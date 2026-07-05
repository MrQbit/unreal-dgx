using UnrealBuildTool;
public class PongGameEditorTarget : TargetRules
{
	public PongGameEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("PongGame");
	}
}
