// Copyright MrQbit. Implementation of Blueprint-authoring functions for MCP over Remote Control.
#include "DGXBlueprintTools.h"

#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Editor.h"
#include "UObject/SavePackage.h"
#include "UObject/UObjectGlobals.h"
#include "AssetRegistry/AssetRegistryModule.h"

#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/BlueprintEditorUtils.h"

#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_CallFunction.h"
#include "K2Node_Event.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "Engine/MemberReference.h"

DEFINE_LOG_CATEGORY_STATIC(LogDGXMCP, Log, All);

// ---------------------------------------------------------------------------- helpers

static UClass* DGX_ResolveClass(const FString& Path)
{
	if (Path.IsEmpty()) { return nullptr; }
	// Native ("/Script/Engine.Actor") or an already-"_C" class path.
	if (UClass* C = LoadObject<UClass>(nullptr, *Path)) { return C; }
	// Blueprint generated-class path.
	if (UClass* C = LoadObject<UClass>(nullptr, *(Path + TEXT("_C")))) { return C; }
	// Blueprint asset path -> generated class.
	if (UBlueprint* BP = LoadObject<UBlueprint>(nullptr, *Path))
	{
		return BP->GeneratedClass;
	}
	return nullptr;
}

static UBlueprint* DGX_LoadBlueprint(const FString& BlueprintPath)
{
	return LoadObject<UBlueprint>(nullptr, *BlueprintPath);
}

// GraphName "" -> event graph (first ubergraph page); otherwise a named function graph.
static UEdGraph* DGX_GetGraph(UBlueprint* BP, const FString& GraphName)
{
	if (!BP) { return nullptr; }
	if (GraphName.IsEmpty())
	{
		return BP->UbergraphPages.Num() > 0 ? BP->UbergraphPages[0] : nullptr;
	}
	for (UEdGraph* G : BP->FunctionGraphs)
	{
		if (G && G->GetName() == GraphName) { return G; }
	}
	for (UEdGraph* G : BP->UbergraphPages)
	{
		if (G && G->GetName() == GraphName) { return G; }
	}
	return nullptr;
}

static UEdGraphNode* DGX_FindNode(UEdGraph* Graph, const FString& NodeGuid)
{
	if (!Graph) { return nullptr; }
	for (UEdGraphNode* N : Graph->Nodes)
	{
		if (N && N->NodeGuid.ToString() == NodeGuid) { return N; }
	}
	return nullptr;
}

static FEdGraphPinType DGX_MakePinType(const FString& VarType)
{
	FEdGraphPinType T;
	const FString V = VarType.ToLower();
	if (V == TEXT("bool"))        { T.PinCategory = UEdGraphSchema_K2::PC_Boolean; }
	else if (V == TEXT("int"))    { T.PinCategory = UEdGraphSchema_K2::PC_Int; }
	else if (V == TEXT("int64"))  { T.PinCategory = UEdGraphSchema_K2::PC_Int64; }
	else if (V == TEXT("float") || V == TEXT("double") || V == TEXT("real"))
	{
		// A Blueprint "float" is a double in UE5; use PC_Real + PC_Double.
		T.PinCategory = UEdGraphSchema_K2::PC_Real;
		T.PinSubCategory = (V == TEXT("float32")) ? UEdGraphSchema_K2::PC_Float : UEdGraphSchema_K2::PC_Double;
	}
	else if (V == TEXT("string"))  { T.PinCategory = UEdGraphSchema_K2::PC_String; }
	else if (V == TEXT("name"))    { T.PinCategory = UEdGraphSchema_K2::PC_Name; }
	else if (V == TEXT("text"))    { T.PinCategory = UEdGraphSchema_K2::PC_Text; }
	else if (V == TEXT("vector"))  { T.PinCategory = UEdGraphSchema_K2::PC_Struct; T.PinSubCategoryObject = TBaseStructure<FVector>::Get(); }
	else if (V == TEXT("rotator")) { T.PinCategory = UEdGraphSchema_K2::PC_Struct; T.PinSubCategoryObject = TBaseStructure<FRotator>::Get(); }
	else if (V == TEXT("transform")){ T.PinCategory = UEdGraphSchema_K2::PC_Struct; T.PinSubCategoryObject = TBaseStructure<FTransform>::Get(); }
	else
	{
		// Treat as an object reference to the given class path.
		if (UClass* C = DGX_ResolveClass(VarType))
		{
			T.PinCategory = UEdGraphSchema_K2::PC_Object;
			T.PinSubCategoryObject = C;
		}
		else { T.PinCategory = UEdGraphSchema_K2::PC_String; }
	}
	return T;
}

// ---------------------------------------------------------------------------- functions

FString UDGXBlueprintTools::CreateBlueprint(const FString& PackagePath, const FString& AssetName, const FString& ParentClassPath)
{
	UClass* ParentClass = DGX_ResolveClass(ParentClassPath);
	if (!ParentClass) { UE_LOG(LogDGXMCP, Warning, TEXT("CreateBlueprint: bad parent class '%s'"), *ParentClassPath); return FString(); }

	const FString PackageName = PackagePath / AssetName;
	UPackage* Package = CreatePackage(*PackageName);
	if (!Package) { return FString(); }

	UBlueprint* BP = FKismetEditorUtilities::CreateBlueprint(
		ParentClass, Package, FName(*AssetName), BPTYPE_Normal,
		UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass());
	if (!BP) { return FString(); }

	FAssetRegistryModule::AssetCreated(BP);
	Package->MarkPackageDirty();
	return BP->GetPathName();
}

bool UDGXBlueprintTools::AddComponent(const FString& BlueprintPath, const FString& ComponentClassPath, const FString& ComponentName)
{
	UBlueprint* BP = DGX_LoadBlueprint(BlueprintPath);
	UClass* CompClass = DGX_ResolveClass(ComponentClassPath);
	if (!BP || !BP->SimpleConstructionScript || !CompClass) { return false; }
	if (!CompClass->IsChildOf(UActorComponent::StaticClass())) { return false; }

	USCS_Node* Node = BP->SimpleConstructionScript->CreateNode(CompClass, FName(*ComponentName));
	if (!Node) { return false; }
	BP->SimpleConstructionScript->AddNode(Node);
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
	return true;
}

bool UDGXBlueprintTools::SetComponentProperty(const FString& BlueprintPath, const FString& ComponentName,
	const FString& PropertyName, const FString& Value)
{
	UBlueprint* BP = DGX_LoadBlueprint(BlueprintPath);
	if (!BP || !BP->SimpleConstructionScript) { return false; }
	for (USCS_Node* Node : BP->SimpleConstructionScript->GetAllNodes())
	{
		if (!Node || Node->GetVariableName() != FName(*ComponentName)) { continue; }
		UActorComponent* Template = Node->ComponentTemplate;
		if (!Template) { return false; }
		FProperty* Prop = Template->GetClass()->FindPropertyByName(FName(*PropertyName));
		if (!Prop) { return false; }
		void* Addr = Prop->ContainerPtrToValuePtr<void>(Template);
		const TCHAR* Result = Prop->ImportText_Direct(*Value, Addr, Template, PPF_None);
		FBlueprintEditorUtils::MarkBlueprintAsModified(BP);
		return Result != nullptr;
	}
	return false;
}

bool UDGXBlueprintTools::AddVariable(const FString& BlueprintPath, const FString& VarName, const FString& VarType)
{
	UBlueprint* BP = DGX_LoadBlueprint(BlueprintPath);
	if (!BP) { return false; }
	return FBlueprintEditorUtils::AddMemberVariable(BP, FName(*VarName), DGX_MakePinType(VarType));
}

bool UDGXBlueprintTools::SetVariableDefault(const FString& BlueprintPath, const FString& VarName, const FString& Value)
{
	UBlueprint* BP = DGX_LoadBlueprint(BlueprintPath);
	if (!BP) { return false; }
	for (FBPVariableDescription& Var : BP->NewVariables)
	{
		if (Var.VarName == FName(*VarName))
		{
			Var.DefaultValue = Value;
			FBlueprintEditorUtils::MarkBlueprintAsModified(BP);
			return true;
		}
	}
	return false;
}

bool UDGXBlueprintTools::AddFunctionGraph(const FString& BlueprintPath, const FString& FunctionName)
{
	UBlueprint* BP = DGX_LoadBlueprint(BlueprintPath);
	if (!BP) { return false; }
	UEdGraph* NewGraph = FBlueprintEditorUtils::CreateNewGraph(
		BP, FName(*FunctionName), UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
	if (!NewGraph) { return false; }
	FBlueprintEditorUtils::AddFunctionGraph<UClass>(BP, NewGraph, /*bIsUserCreated*/true, nullptr);
	return true;
}

FString UDGXBlueprintTools::AddCallFunctionNode(const FString& BlueprintPath, const FString& GraphName,
	const FString& TargetClassPath, const FString& FunctionName, float NodePosX, float NodePosY)
{
	UBlueprint* BP = DGX_LoadBlueprint(BlueprintPath);
	UEdGraph* Graph = DGX_GetGraph(BP, GraphName);
	if (!BP || !Graph) { return FString(); }

	UClass* TargetClass = nullptr;
	if (TargetClassPath.IsEmpty())
	{
		TargetClass = BP->GeneratedClass ? BP->GeneratedClass.Get() : BP->ParentClass.Get();
	}
	else
	{
		TargetClass = DGX_ResolveClass(TargetClassPath);
	}
	if (!TargetClass) { return FString(); }
	UFunction* Func = TargetClass->FindFunctionByName(FName(*FunctionName));
	if (!Func) { return FString(); }

	FGraphNodeCreator<UK2Node_CallFunction> Creator(*Graph);
	UK2Node_CallFunction* Node = Creator.CreateNode();
	Node->SetFromFunction(Func);
	Node->NodePosX = NodePosX;
	Node->NodePosY = NodePosY;
	Creator.Finalize();
	return Node->NodeGuid.ToString();
}

FString UDGXBlueprintTools::AddEventNode(const FString& BlueprintPath, const FString& EventName, float NodePosX, float NodePosY)
{
	UBlueprint* BP = DGX_LoadBlueprint(BlueprintPath);
	UEdGraph* Graph = DGX_GetGraph(BP, FString());
	if (!BP || !Graph) { return FString(); }

	// Use the engine helper: creates (or finds) the overridable event node correctly.
	UClass* ParentClass = BP->ParentClass ? BP->ParentClass.Get() : AActor::StaticClass();
	int32 PosY = (int32)NodePosY;
	UK2Node_Event* Node = FKismetEditorUtilities::AddDefaultEventNode(BP, Graph, FName(*EventName), ParentClass, PosY);
	if (!Node) { return FString(); }
	Node->NodePosX = (int32)NodePosX;
	return Node->NodeGuid.ToString();
}

FString UDGXBlueprintTools::AddVariableNode(const FString& BlueprintPath, const FString& GraphName,
	const FString& VarName, bool bIsSetter, float NodePosX, float NodePosY)
{
	UBlueprint* BP = DGX_LoadBlueprint(BlueprintPath);
	UEdGraph* Graph = DGX_GetGraph(BP, GraphName);
	if (!BP || !Graph) { return FString(); }

	UEdGraphNode* NodeBase = nullptr;
	if (bIsSetter)
	{
		FGraphNodeCreator<UK2Node_VariableSet> Creator(*Graph);
		UK2Node_VariableSet* Node = Creator.CreateNode();
		Node->VariableReference.SetSelfMember(FName(*VarName));
		Node->NodePosX = NodePosX; Node->NodePosY = NodePosY;
		Creator.Finalize();
		NodeBase = Node;
	}
	else
	{
		FGraphNodeCreator<UK2Node_VariableGet> Creator(*Graph);
		UK2Node_VariableGet* Node = Creator.CreateNode();
		Node->VariableReference.SetSelfMember(FName(*VarName));
		Node->NodePosX = NodePosX; Node->NodePosY = NodePosY;
		Creator.Finalize();
		NodeBase = Node;
	}
	return NodeBase ? NodeBase->NodeGuid.ToString() : FString();
}

bool UDGXBlueprintTools::ConnectNodes(const FString& BlueprintPath, const FString& GraphName,
	const FString& NodeAGuid, const FString& PinAName, const FString& NodeBGuid, const FString& PinBName)
{
	UBlueprint* BP = DGX_LoadBlueprint(BlueprintPath);
	UEdGraph* Graph = DGX_GetGraph(BP, GraphName);
	if (!BP || !Graph) { return false; }

	UEdGraphNode* A = DGX_FindNode(Graph, NodeAGuid);
	UEdGraphNode* B = DGX_FindNode(Graph, NodeBGuid);
	if (!A || !B) { return false; }

	UEdGraphPin* PinA = A->FindPin(FName(*PinAName));
	UEdGraphPin* PinB = B->FindPin(FName(*PinBName));
	if (!PinA || !PinB) { return false; }

	const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();
	const bool bOk = Schema->TryCreateConnection(PinA, PinB);
	if (bOk) { FBlueprintEditorUtils::MarkBlueprintAsModified(BP); }
	return bOk;
}

bool UDGXBlueprintTools::CompileBlueprint(const FString& BlueprintPath)
{
	UBlueprint* BP = DGX_LoadBlueprint(BlueprintPath);
	if (!BP) { return false; }
	FKismetEditorUtilities::CompileBlueprint(BP);
	return BP->Status == BS_UpToDate || BP->Status == BS_UpToDateWithWarnings;
}

bool UDGXBlueprintTools::SaveAsset(const FString& AssetPath)
{
	UObject* Asset = LoadObject<UObject>(nullptr, *AssetPath);
	if (!Asset) { return false; }
	UPackage* Package = Asset->GetOutermost();
	if (!Package) { return false; }

	const FString FileName = FPackageName::LongPackageNameToFilename(
		Package->GetName(), FPackageName::GetAssetPackageExtension());
	FSavePackageArgs Args;
	Args.TopLevelFlags = RF_Public | RF_Standalone;
	Args.SaveFlags = SAVE_NoError;
	return UPackage::SavePackage(Package, nullptr, *FileName, Args);
}

FString UDGXBlueprintTools::SpawnActor(const FString& ClassPath, FVector Location, FRotator Rotation)
{
	if (!GEditor) { return FString(); }
	UWorld* World = GEditor->GetEditorWorldContext().World();
	UClass* Cls = DGX_ResolveClass(ClassPath);
	if (!World || !Cls || !Cls->IsChildOf(AActor::StaticClass())) { return FString(); }

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AActor* Actor = World->SpawnActor(Cls, &Location, &Rotation, Params);
	return Actor ? Actor->GetPathName() : FString();
}

FString UDGXBlueprintTools::DescribeGraph(const FString& BlueprintPath, const FString& GraphName)
{
	UBlueprint* BP = DGX_LoadBlueprint(BlueprintPath);
	UEdGraph* Graph = DGX_GetGraph(BP, GraphName);
	if (!Graph) { return TEXT("{\"error\":\"graph not found\"}"); }

	FString Out = TEXT("{\"nodes\":[");
	bool bFirst = true;
	for (UEdGraphNode* N : Graph->Nodes)
	{
		if (!N) { continue; }
		if (!bFirst) { Out += TEXT(","); }
		bFirst = false;
		FString Pins;
		bool bFirstPin = true;
		for (UEdGraphPin* P : N->Pins)
		{
			if (!P) { continue; }
			if (!bFirstPin) { Pins += TEXT(","); }
			bFirstPin = false;
			Pins += FString::Printf(TEXT("{\"name\":\"%s\",\"dir\":\"%s\"}"),
				*P->PinName.ToString(), P->Direction == EGPD_Input ? TEXT("in") : TEXT("out"));
		}
		Out += FString::Printf(TEXT("{\"guid\":\"%s\",\"title\":\"%s\",\"pins\":[%s]}"),
			*N->NodeGuid.ToString(), *N->GetNodeTitle(ENodeTitleType::ListView).ToString(), *Pins);
	}
	Out += TEXT("]}");
	return Out;
}
