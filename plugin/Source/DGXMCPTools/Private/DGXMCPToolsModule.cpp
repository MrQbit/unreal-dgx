#include "DGXMCPToolsModule.h"
#include "Modules/ModuleManager.h"

#define LOCTEXT_NAMESPACE "FDGXMCPToolsModule"

void FDGXMCPToolsModule::StartupModule()
{
	UE_LOG(LogTemp, Display, TEXT("DGXMCPTools: Blueprint-authoring tools loaded (callable via Remote Control)."));
}

void FDGXMCPToolsModule::ShutdownModule() {}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FDGXMCPToolsModule, DGXMCPTools)
