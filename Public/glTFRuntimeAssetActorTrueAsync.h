// Copyright 2020, Roberto De Ioris.

#pragma once

#include <functional>

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "glTFRuntimeAsset.h"
#include "glTFRuntimeAssetActorTrueAsync.generated.h"

UCLASS()
class GLTFRUNTIME_API AglTFRuntimeAssetActorTrueAsync : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AglTFRuntimeAssetActorTrueAsync();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void ProcessRootNodes();

	virtual void SetAsset(UglTFRuntimeAsset* InAsset);

	virtual void ClearAsset();

	virtual void SetAssetAsync(UglTFRuntimeAsset* InAsset, const std::function<void()>& OnFinished);
	
	virtual void ProcessNode(USceneComponent* NodeParentComponent, FglTFRuntimeNode& Node);

	TMap<USceneComponent*, float> CurveBasedAnimationsTimeTracker;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "glTFRuntime")
	TSet<FString> DiscoveredCurveAnimationsNames;

	TMap<USceneComponent*, TMap<FString, UglTFRuntimeAnimationCurve*>> DiscoveredCurveAnimations;

	template<typename T>
	FName GetSafeNodeName(const FglTFRuntimeNode& Node)
	{
		return MakeUniqueObjectName(this, T::StaticClass(), *Node.Name);
	}

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (ExposeOnSpawn = true), Category = "glTFRuntime")
	UglTFRuntimeAsset* Asset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (ExposeOnSpawn = true), Category = "glTFRuntime")
	FglTFRuntimeStaticMeshConfig StaticMeshConfig;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Meta = (ExposeOnSpawn = true), Category = "glTFRuntime")
	FglTFRuntimeSkeletalMeshConfig SkeletalMeshConfig;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "glTFRuntime")
	TMap<USceneComponent*, UglTFRuntimeAnimationCurve*> CurveBasedAnimations;

	UFUNCTION(BlueprintNativeEvent, Category = "glTFRuntime", meta = (DisplayName = "On StaticMeshComponent Created"))
	void ReceiveOnStaticMeshComponentCreated(UStaticMeshComponent* StaticMeshComponent, const FglTFRuntimeNode& Node);

	UFUNCTION(BlueprintNativeEvent, Category = "glTFRuntime", meta = (DisplayName = "On SkeletalMeshComponent Created"))
	void ReceiveOnSkeletalMeshComponentCreated(USkeletalMeshComponent* SkeletalMeshComponent, const FglTFRuntimeNode& Node);

	UFUNCTION(BlueprintCallable, Category = "glTFRuntime")
	void SetCurveAnimationByName(const FString& CurveAnimationName);
	
};
