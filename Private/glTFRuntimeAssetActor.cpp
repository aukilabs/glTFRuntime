// Copyright 2020, Roberto De Ioris.

#include "glTFRuntimeAssetActor.h"

#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/StaticMeshSocket.h"
#include "Animation/AnimSequence.h"
#include "Async/Async.h"

// Sets default values
AglTFRuntimeAssetActor::AglTFRuntimeAssetActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	AssetRoot = CreateDefaultSubobject<USceneComponent>(TEXT("AssetRoot"));
	RootComponent = AssetRoot;
}

// Called when the game starts or when spawned
void AglTFRuntimeAssetActor::BeginPlay()
{
	Super::BeginPlay();

	if (!Asset)
	{
		return;
	}

	TArray<FglTFRuntimeScene> Scenes = Asset->GetScenes();
	for (FglTFRuntimeScene& Scene : Scenes)
	{
		USceneComponent* SceneComponent = NewObject<USceneComponent>(this, *FString::Printf(TEXT("Scene %d"), Scene.Index));
		SceneComponent->SetupAttachment(RootComponent);
		SceneComponent->RegisterComponent();
		AddInstanceComponent(SceneComponent);
		for (int32 NodeIndex : Scene.RootNodesIndices)
		{
			FglTFRuntimeNode Node;
			if (!Asset->GetNode(NodeIndex, Node))
			{
				return;
			}
			ProcessNode(SceneComponent, Node);
		}
	}
}

void AglTFRuntimeAssetActor::ProcessNode(USceneComponent* NodeParentComponent, FglTFRuntimeNode& Node)
{
	// skip bones/joints
	if (Asset->NodeIsBone(Node.Index))
	{
		return;
	}

	USceneComponent* NewComponent = nullptr;
	if (Node.CameraIndex != INDEX_NONE)
	{
		UCameraComponent* NewCameraComponent = NewObject<UCameraComponent>(this, GetSafeNodeName<UCameraComponent>(Node));
		NewCameraComponent->SetupAttachment(NodeParentComponent);
		NewCameraComponent->RegisterComponent();
		NewCameraComponent->SetRelativeTransform(Node.Transform);
		AddInstanceComponent(NewCameraComponent);
		Asset->LoadCamera(Node.CameraIndex, NewCameraComponent);
		NewComponent = NewCameraComponent;
	}
	else if (Node.MeshIndex < 0)
	{
		NewComponent = NewObject<USceneComponent>(this, GetSafeNodeName<USceneComponent>(Node));
		NewComponent->SetupAttachment(NodeParentComponent);
		NewComponent->RegisterComponent();
		NewComponent->SetRelativeTransform(Node.Transform);
		AddInstanceComponent(NewComponent);
	}
	else
	{
		if (Node.SkinIndex < 0)
		{
			UStaticMeshComponent* StaticMeshComponent = NewObject<UStaticMeshComponent>(this, GetSafeNodeName<UStaticMeshComponent>(Node));
			StaticMeshComponent->SetupAttachment(NodeParentComponent);
			StaticMeshComponent->RegisterComponent();
			StaticMeshComponent->SetRelativeTransform(Node.Transform);
			AddInstanceComponent(StaticMeshComponent);
			if (StaticMeshConfig.Outer == nullptr)
			{
				StaticMeshConfig.Outer = StaticMeshComponent;
			}
			UStaticMesh* StaticMesh = Asset->LoadStaticMesh(Node.MeshIndex, StaticMeshConfig);
			if (StaticMesh && !StaticMeshConfig.ExportOriginalPivotToSocket.IsEmpty())
			{
				UStaticMeshSocket* DeltaSocket = StaticMesh->FindSocket(FName(StaticMeshConfig.ExportOriginalPivotToSocket));
				if (DeltaSocket)
				{
					FTransform NewTransform = StaticMeshComponent->GetRelativeTransform();
					FVector DeltaLocation = -DeltaSocket->RelativeLocation * NewTransform.GetScale3D();
					DeltaLocation = NewTransform.GetRotation().RotateVector(DeltaLocation);
					NewTransform.AddToTranslation(DeltaLocation);
					StaticMeshComponent->SetRelativeTransform(NewTransform);
				}
			}
			StaticMeshComponent->SetStaticMesh(StaticMesh);
			ReceiveOnStaticMeshComponentCreated(StaticMeshComponent, Node);
			NewComponent = StaticMeshComponent;
		}
		else
		{
			USkeletalMeshComponent* SkeletalMeshComponent = NewObject<USkeletalMeshComponent>(this, GetSafeNodeName<USkeletalMeshComponent>(Node));
			SkeletalMeshComponent->SetupAttachment(NodeParentComponent);
			SkeletalMeshComponent->RegisterComponent();
			SkeletalMeshComponent->SetRelativeTransform(Node.Transform);
			AddInstanceComponent(SkeletalMeshComponent);
			USkeletalMesh* SkeletalMesh = Asset->LoadSkeletalMesh(Node.MeshIndex, Node.SkinIndex, SkeletalMeshConfig);
			SkeletalMeshComponent->SetSkeletalMesh(SkeletalMesh);
			ReceiveOnSkeletalMeshComponentCreated(SkeletalMeshComponent, Node);
			NewComponent = SkeletalMeshComponent;
		}
	}

	if (!NewComponent)
	{
		return;
	}

	// check for audio emitters
	for (const int32 EmitterIndex : Node.EmitterIndices)
	{
		FglTFRuntimeAudioEmitter AudioEmitter;
		if (Asset->LoadAudioEmitter(EmitterIndex, AudioEmitter))
		{
			UAudioComponent* AudioComponent = NewObject<UAudioComponent>(this, *AudioEmitter.Name);
			AudioComponent->SetupAttachment(NewComponent);
			AudioComponent->RegisterComponent();
			AudioComponent->SetRelativeTransform(Node.Transform);
			AddInstanceComponent(AudioComponent);
			Asset->LoadEmitterIntoAudioComponent(AudioEmitter, AudioComponent);
			AudioComponent->Play();
		}
	}

	// check for animations
	if (!NewComponent->IsA<USkeletalMeshComponent>())
	{
		TArray<UglTFRuntimeAnimationCurve*> ComponentAnimationCurves = Asset->LoadAllNodeAnimationCurves(Node.Index);
		TMap<FString, UglTFRuntimeAnimationCurve*> ComponentAnimationCurvesMap;
		for (UglTFRuntimeAnimationCurve* ComponentAnimationCurve : ComponentAnimationCurves)
		{
			if (!CurveBasedAnimations.Contains(NewComponent))
			{
				CurveBasedAnimations.Add(NewComponent, ComponentAnimationCurve);
				CurveBasedAnimationsTimeTracker.Add(NewComponent, 0);
			}
			DiscoveredCurveAnimationsNames.Add(ComponentAnimationCurve->glTFCurveAnimationName);
			ComponentAnimationCurvesMap.Add(ComponentAnimationCurve->glTFCurveAnimationName, ComponentAnimationCurve);
		}
		DiscoveredCurveAnimations.Add(NewComponent, ComponentAnimationCurvesMap);
	}
	else
	{
		USkeletalMeshComponent* SkeletalMeshComponent = Cast<USkeletalMeshComponent>(NewComponent);
		int NodeIndex = Node.Index;

		// TMap<FString, UAnimSequence*> Sequences = Asset->LoadMeshNodeAllSkeletalAnimations(SkeletalMeshComponent->SkeletalMesh, NodeIndex,
		//                                                                             SkeletalAnimationConfig);
		// OnSkeletalMeshAnimationsLoaded(SkeletalMeshComponent->SkeletalMesh, Sequences);

		Asset->LoadMeshNodeAllSkeletalAnimationsAsync(SkeletalMeshComponent->SkeletalMesh, NodeIndex, SkeletalAnimationConfig,
			[this](USkeletalMesh* Mesh, TMap<FString, UAnimSequence*> Sequences)
		{
			// Because animations were created on a non-game thread, they have the Async flag set -> clear it.
			for (const auto& AnimationTuple : Sequences)
			{
				AnimationTuple.Value->ClearInternalFlags(EInternalObjectFlags::Async);
			}
			this->OnSkeletalMeshAnimationsLoaded(Mesh, Sequences);
		});
	    
	}

	for (int32 ChildIndex : Node.ChildrenIndices)
	{
		FglTFRuntimeNode Child;
		if (!Asset->GetNode(ChildIndex, Child))
		{
			return;
		}
		ProcessNode(NewComponent, Child);
	}
}

void AglTFRuntimeAssetActor::SetCurveAnimationByName(const FString& CurveAnimationName)
{
	if (!DiscoveredCurveAnimationsNames.Contains(CurveAnimationName))
	{
		return;
	}

	for (TPair<USceneComponent*, UglTFRuntimeAnimationCurve*>& Pair : CurveBasedAnimations)
	{
		TMap<FString, UglTFRuntimeAnimationCurve*> WantedCurveAnimationsMap = DiscoveredCurveAnimations[Pair.Key];
		if (WantedCurveAnimationsMap.Contains(CurveAnimationName))
		{
			Pair.Value = WantedCurveAnimationsMap[CurveAnimationName];
			CurveBasedAnimationsTimeTracker[Pair.Key] = 0;
		}
		else
		{
			Pair.Value = nullptr;
		}
	}
}

// Called every frame
void AglTFRuntimeAssetActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	for (TPair<USceneComponent*, UglTFRuntimeAnimationCurve*>& Pair : CurveBasedAnimations)
	{
		// the curve could be null
		if (!Pair.Value)
		{
			continue;
		}
		float MinTime;
		float MaxTime;
		Pair.Value->GetTimeRange(MinTime, MaxTime);

		float CurrentTime = CurveBasedAnimationsTimeTracker[Pair.Key];
		if (CurrentTime > Pair.Value->glTFCurveAnimationDuration)
		{
			CurveBasedAnimationsTimeTracker[Pair.Key] = 0;
			CurrentTime = 0;
		}

		if (CurrentTime >= MinTime)
		{
			FTransform FrameTransform = Pair.Value->GetTransformValue(CurveBasedAnimationsTimeTracker[Pair.Key]);
			Pair.Key->SetRelativeTransform(FrameTransform);
		}
		CurveBasedAnimationsTimeTracker[Pair.Key] += DeltaTime;
	}
}


void AglTFRuntimeAssetActor::OnSkeletalMeshAnimationsLoaded(USkeletalMesh* SkeletalMesh, TMap<FString, UAnimSequence*>& AnimationSequences)
{
	if (AnimationSequences.Num())
	{
		AnimSequences = AnimationSequences;
		AnimSequences.GenerateKeyArray(AnimationNames);
		
		AnimatedSkeletalMeshComponent = nullptr;

		TArray<UActorComponent*> Components;
		GetComponents(USkeletalMeshComponent::StaticClass(), Components);
		for (UActorComponent* Component : Components)
		{
			USkeletalMeshComponent* CandidateComponent = Cast<USkeletalMeshComponent>(Component);
			if (CandidateComponent && CandidateComponent->SkeletalMesh == SkeletalMesh)
			{
				AnimatedSkeletalMeshComponent = CandidateComponent;
				break;
			}
		}

		if (!AnimatedSkeletalMeshComponent)
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to find component with matching skeletal mesh!"));
			return;
		}

		UAnimMontage* NewMontage = UAnimMontage::CreateSlotAnimationAsDynamicMontage(AnimSequences[AnimationNames[CurrentAnimSequence]], FName("Default"));
		AnimatedSkeletalMeshComponent->AnimationData.AnimToPlay = NewMontage;
		AnimatedSkeletalMeshComponent->AnimationData.bSavedLooping = false;
		AnimatedSkeletalMeshComponent->AnimationData.bSavedPlaying = true;
		AnimatedSkeletalMeshComponent->SetAnimationMode(EAnimationMode::AnimationSingleNode);

		UAnimInstance* Instance = AnimatedSkeletalMeshComponent->GetAnimInstance();
		if (!Instance)
		{
			AnimatedSkeletalMeshComponent->InitializeAnimScriptInstance();
			Instance = AnimatedSkeletalMeshComponent->GetAnimInstance();
		}
		if (!Instance)
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to initialize or get animation instance!"));
			return;
		}
		Instance->OnMontageBlendingOut.AddDynamic(this, &AglTFRuntimeAssetActor::OnAnimationBlendingOut);
	}
}

void AglTFRuntimeAssetActor::OnAnimationBlendingOut(UAnimMontage* Montage, bool bInterrupted)
{
	PlayNextAnimation(AnimatedSkeletalMeshComponent);
}

void AglTFRuntimeAssetActor::PlayNextAnimation(USkeletalMeshComponent* SkeletalMeshComponent)
{
	UAnimInstance* Instance = SkeletalMeshComponent->GetAnimInstance();
	if (CurrentAnimSequence >= AnimationNames.Num())
	{
		CurrentAnimSequence = 0;
	}
	const FString CurrentAnimationName = AnimationNames[CurrentAnimSequence];
	UAnimSequence* CurrentAnimation = AnimSequences[CurrentAnimationName];
	
	UE_LOG(LogTemp, Warning, TEXT("Playing animation %s!"), *CurrentAnimationName);
	UAnimMontage* NewMontage = UAnimMontage::CreateSlotAnimationAsDynamicMontage(CurrentAnimation, FName("Default"));
	Instance->Montage_Play(NewMontage);
	CurrentAnimSequence++;
}


void AglTFRuntimeAssetActor::ReceiveOnStaticMeshComponentCreated_Implementation(UStaticMeshComponent* StaticMeshComponent,
                                                                                const FglTFRuntimeNode& Node)
{
}

void AglTFRuntimeAssetActor::ReceiveOnSkeletalMeshComponentCreated_Implementation(USkeletalMeshComponent* SkeletalMeshComponent,
                                                                                  const FglTFRuntimeNode& Node)
{
}
