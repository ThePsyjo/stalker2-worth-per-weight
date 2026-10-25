using UnrealBuildTool;

public class Stalker2EditorTarget : TargetRules
{
	public Stalker2EditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_5;
		ExtraModuleNames.AddRange(new[] { "Stalker2", "WPWEng", "WPWTools" });
	}
}
