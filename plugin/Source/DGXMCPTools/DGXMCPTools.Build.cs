// Copyright MrQbit. Blueprint-authoring tools exposed via Remote Control for MCP.
using UnrealBuildTool;

public class DGXMCPTools : ModuleRules
{
	public DGXMCPTools(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"UnrealEd",          // FKismetEditorUtilities, editor world
			"BlueprintGraph",    // UK2Node_* , UEdGraphSchema_K2
			"KismetCompiler",    // compile
			"Kismet",            // blueprint editor utils
			"AssetTools",        // asset creation
			"AssetRegistry",     // AssetCreated
			"Slate", "SlateCore",
			"ToolMenus",
		});
	}
}
