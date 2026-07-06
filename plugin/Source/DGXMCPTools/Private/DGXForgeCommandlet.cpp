// Copyright MrQbit. Batch commandlet that runs the GameForge assembly in-process (survives headless):
//   UnrealEditor-Cmd <Project> -run=DGXForge
// Imports the locally-generated assets (~/gameforge/assets) and builds+wires the entity Blueprints,
// exercising the exact ImportAsset + Blueprint-graph path the MCP/orchestrator uses — but as one
// batch run rather than a persistent Remote Control server.
#include "DGXForgeCommandlet.h"
#include "DGXBlueprintTools.h"
#include "Misc/Paths.h"
#include "HAL/PlatformMisc.h"

DEFINE_LOG_CATEGORY_STATIC(LogDGXForge, Log, All);

int32 UDGXForgeCommandlet::Main(const FString& Params)
{
	const FString Home = FPlatformMisc::GetEnvironmentVariable(TEXT("HOME"));
	const FString Assets = FPaths::Combine(Home, TEXT("gameforge"), TEXT("assets"));
	UE_LOG(LogDGXForge, Display, TEXT("=== GameForge assembly (batch) === assets: %s"), *Assets);

	// 1. import the locally-generated assets (glTF mesh, texture, audio)
	auto Import = [&](const TCHAR* File, const TCHAR* Dest)
	{
		const FString Src = FPaths::Combine(Assets, File);
		const FString R = UDGXBlueprintTools::ImportAsset(Src, Dest);
		UE_LOG(LogDGXForge, Display, TEXT("  import %s -> %s"), File, R.IsEmpty() ? TEXT("(FAILED)") : *R);
		return R;
	};
	Import(TEXT("crate.glb"),         TEXT("/Game/Meshes"));
	Import(TEXT("stone_texture.png"), TEXT("/Game/Textures"));
	Import(TEXT("bgm.wav"),           TEXT("/Game/Audio"));

	// 2. BP_Ball (Actor) + "spin" recipe: Tick -> AddActorLocalRotation(yaw)
	{
		const FString BP = UDGXBlueprintTools::CreateBlueprint(TEXT("/Game/Blueprints"), TEXT("BP_Ball"), TEXT("/Script/Engine.Actor"));
		const FString Ev  = UDGXBlueprintTools::AddEventNode(BP, TEXT("ReceiveTick"), -400, 0);
		const FString Rot = UDGXBlueprintTools::AddCallFunctionNode(BP, TEXT(""), TEXT(""), TEXT("K2_AddActorLocalRotation"), 0, 0);
		UDGXBlueprintTools::SetPinDefault(BP, TEXT(""), Rot, TEXT("DeltaRotation"), TEXT("(Pitch=0.0,Yaw=2.0,Roll=0.0)"));
		UDGXBlueprintTools::ConnectNodes(BP, TEXT(""), Ev, TEXT("then"), Rot, TEXT("execute"));
		const bool Ok = UDGXBlueprintTools::CompileBlueprint(BP);
		UDGXBlueprintTools::SaveAsset(TEXT("/Game/Blueprints/BP_Ball"));
		UE_LOG(LogDGXForge, Display, TEXT("  BP_Ball spin recipe -> %s (compiled=%d)"), *BP, Ok ? 1 : 0);
	}

	// 3. BP_Paddle (Pawn) + "hello" recipe: BeginPlay -> PrintString  (input recipes need mappings)
	{
		const FString BP = UDGXBlueprintTools::CreateBlueprint(TEXT("/Game/Blueprints"), TEXT("BP_Paddle"), TEXT("/Script/Engine.Pawn"));
		const FString Ev = UDGXBlueprintTools::AddEventNode(BP, TEXT("ReceiveBeginPlay"), -400, 0);
		const FString Ps = UDGXBlueprintTools::AddCallFunctionNode(BP, TEXT(""), TEXT("/Script/Engine.KismetSystemLibrary"), TEXT("PrintString"), 0, 0);
		UDGXBlueprintTools::SetPinDefault(BP, TEXT(""), Ps, TEXT("InString"), TEXT("Paddle ready"));
		UDGXBlueprintTools::ConnectNodes(BP, TEXT(""), Ev, TEXT("then"), Ps, TEXT("execute"));
		const bool Ok = UDGXBlueprintTools::CompileBlueprint(BP);
		UDGXBlueprintTools::SaveAsset(TEXT("/Game/Blueprints/BP_Paddle"));
		UE_LOG(LogDGXForge, Display, TEXT("  BP_Paddle hello recipe -> %s (compiled=%d)"), *BP, Ok ? 1 : 0);
	}

	UE_LOG(LogDGXForge, Display, TEXT("=== GameForge assembly done ==="));
	return 0;
}
