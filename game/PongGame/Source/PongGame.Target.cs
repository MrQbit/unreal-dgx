using UnrealBuildTool;
public class PongGameTarget : TargetRules
{
	public PongGameTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("PongGame");
	}
}
