// Copyright MrQbit. Runtime gameplay helpers (ship in packaged games).
using UnrealBuildTool;

public class DGXGameplay : ModuleRules
{
	public DGXGameplay(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine" });
	}
}
