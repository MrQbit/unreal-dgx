// Copyright MrQbit. Blueprint-authoring functions callable over Remote Control for MCP-driven
// game building. All functions are static + BlueprintCallable so the Remote Control HTTP API can
// call them via  PUT /remote/object/call  on this class's CDO:
//   objectPath = "/Script/DGXMCPTools.Default__DGXBlueprintTools"
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "DGXBlueprintTools.generated.h"

UCLASS()
class UDGXBlueprintTools : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Create a new Blueprint asset. ParentClassPath e.g. "/Script/Engine.Actor" or "/Script/Engine.Pawn".
	 *  Returns the created Blueprint's object path (e.g. "/Game/BP/BP_Foo.BP_Foo") or "" on failure. */
	UFUNCTION(BlueprintCallable, Category = "DGX|Blueprint")
	static FString CreateBlueprint(const FString& PackagePath, const FString& AssetName, const FString& ParentClassPath);

	/** Add a component to the Blueprint's construction script. ComponentClassPath e.g.
	 *  "/Script/Engine.StaticMeshComponent". Returns true on success. */
	UFUNCTION(BlueprintCallable, Category = "DGX|Blueprint")
	static bool AddComponent(const FString& BlueprintPath, const FString& ComponentClassPath, const FString& ComponentName);

	/** Set a default property on a component previously added to the Blueprint. Value is a string the
	 *  property can import (numbers, "(X=1,Y=2,Z=3)" for vectors, asset paths for object refs). */
	UFUNCTION(BlueprintCallable, Category = "DGX|Blueprint")
	static bool SetComponentProperty(const FString& BlueprintPath, const FString& ComponentName,
		const FString& PropertyName, const FString& Value);

	/** Add a member variable. VarType is one of: bool, int, int64, float, double, string, name, text,
	 *  vector, rotator, transform, or an object class path like "/Script/Engine.Actor". */
	UFUNCTION(BlueprintCallable, Category = "DGX|Blueprint")
	static bool AddVariable(const FString& BlueprintPath, const FString& VarName, const FString& VarType);

	/** Set the default value of a member variable (string the property can import). */
	UFUNCTION(BlueprintCallable, Category = "DGX|Blueprint")
	static bool SetVariableDefault(const FString& BlueprintPath, const FString& VarName, const FString& Value);

	/** Add a new (empty) function graph to the Blueprint. Returns true on success. */
	UFUNCTION(BlueprintCallable, Category = "DGX|Blueprint")
	static bool AddFunctionGraph(const FString& BlueprintPath, const FString& FunctionName);

	/** Add a "call function" node to a graph (GraphName "" = event graph). TargetClassPath is the class
	 *  owning the function ("/Script/Engine.KismetSystemLibrary" for statics, "" = self/parent class).
	 *  Returns the new node's GUID string (use it with ConnectNodes) or "" on failure. */
	UFUNCTION(BlueprintCallable, Category = "DGX|Blueprint")
	static FString AddCallFunctionNode(const FString& BlueprintPath, const FString& GraphName,
		const FString& TargetClassPath, const FString& FunctionName, float NodePosX, float NodePosY);

	/** Add an overridable event node (e.g. "ReceiveBeginPlay", "ReceiveTick") to the event graph.
	 *  Returns the node's GUID string or "" on failure. */
	UFUNCTION(BlueprintCallable, Category = "DGX|Blueprint")
	static FString AddEventNode(const FString& BlueprintPath, const FString& EventName, float NodePosX, float NodePosY);

	/** Add an input-axis event node (legacy input mapping, e.g. "MoveRight") to the event graph.
	 *  Its "then" exec + float "Axis Value" pin drive player-controlled behaviours. Returns the GUID. */
	UFUNCTION(BlueprintCallable, Category = "DGX|Blueprint")
	static FString AddInputAxisEvent(const FString& BlueprintPath, const FString& AxisName, float NodePosX, float NodePosY);

	/** Set a literal default value on a node pin (for constants like a rotation amount or a string).
	 *  Value is the import string ("(Pitch=0,Yaw=90,Roll=0)", "5.0", "Hello"). GraphName "" = event graph. */
	UFUNCTION(BlueprintCallable, Category = "DGX|Blueprint")
	static bool SetPinDefault(const FString& BlueprintPath, const FString& GraphName,
		const FString& NodeGuid, const FString& PinName, const FString& Value);

	/** Add a variable get/set node for a member variable. bIsSetter=true adds a Set node.
	 *  Returns the node's GUID string or "" on failure. */
	UFUNCTION(BlueprintCallable, Category = "DGX|Blueprint")
	static FString AddVariableNode(const FString& BlueprintPath, const FString& GraphName,
		const FString& VarName, bool bIsSetter, float NodePosX, float NodePosY);

	/** Connect two node pins. Pin names: exec-out is usually "then", exec-in is "execute"; data pins
	 *  use their parameter name. GraphName "" = event graph. Returns true on success. */
	UFUNCTION(BlueprintCallable, Category = "DGX|Blueprint")
	static bool ConnectNodes(const FString& BlueprintPath, const FString& GraphName,
		const FString& NodeAGuid, const FString& PinAName,
		const FString& NodeBGuid, const FString& PinBName);

	/** Compile the Blueprint. Returns true if it compiled without errors. */
	UFUNCTION(BlueprintCallable, Category = "DGX|Blueprint")
	static bool CompileBlueprint(const FString& BlueprintPath);

	/** Save an asset by package path, e.g. "/Game/BP/BP_Foo". */
	UFUNCTION(BlueprintCallable, Category = "DGX|Blueprint")
	static bool SaveAsset(const FString& AssetPath);

	/** Spawn an actor from a Blueprint (or native class path) into the current editor level.
	 *  Returns the spawned actor's path name or "" on failure. */
	UFUNCTION(BlueprintCallable, Category = "DGX|Blueprint")
	static FString SpawnActor(const FString& ClassPath, FVector Location, FRotator Rotation);

	/** List the node GUIDs + titles in a graph (JSON), to inspect what was built. */
	UFUNCTION(BlueprintCallable, Category = "DGX|Blueprint")
	static FString DescribeGraph(const FString& BlueprintPath, const FString& GraphName);

	// ---- asset import (the generation pipeline: Blender glTF, textures, audio -> UE assets) ----

	/** Import any source asset file into the project via the appropriate factory/translator:
	 *  .glb/.gltf (mesh+material+anim via Interchange), .png/.jpg/.tga (texture), .wav (sound), etc.
	 *  DestPath is a content path like "/Game/Meshes". Returns the imported object's path or "". */
	UFUNCTION(BlueprintCallable, Category = "DGX|Assets")
	static FString ImportAsset(const FString& SourceFile, const FString& DestPath);

	/** Set a Static Mesh (by asset path) on a StaticMeshActor previously spawned into the level. */
	UFUNCTION(BlueprintCallable, Category = "DGX|Assets")
	static bool SetActorStaticMesh(const FString& ActorPath, const FString& StaticMeshPath);
};
