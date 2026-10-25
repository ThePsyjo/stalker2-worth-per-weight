using UnrealBuildTool;

public class Stalker2Target : TargetRules
{
	public Stalker2Target(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_5;
		ExtraModuleNames.AddRange(new[] { "Stalker2", "WPWEng" });
	}
}
