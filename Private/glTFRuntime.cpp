// Copyright 2020, Roberto De Ioris.

#include "glTFRuntime.h"

#include "glTFRuntimeParser.h"

#define LOCTEXT_NAMESPACE "FglTFRuntimeModule"

void FglTFRuntimeModule::StartupModule()
{
    if(!FModuleManager::Get().IsModuleLoaded("ProceduralMeshComponent"))
    {
        FModuleManager::Get().LoadModule("ProceduralMeshComponent");
    }
	FglTFRuntimeParser::Init();
}

void FglTFRuntimeModule::ShutdownModule()
{
	FglTFRuntimeParser::Deinit();
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FglTFRuntimeModule, glTFRuntime)
