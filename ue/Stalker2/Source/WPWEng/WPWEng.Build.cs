using UnrealBuildTool;

// Stand-in for classes the game adds to /Script/Engine. "WPWEng" has the same length as "Engine", so the cooked
// import "/Script/WPWEng" is patched to "/Script/Engine" in place (tools/zonekit.py).
public class WPWEng : ModuleRules
{
	public WPWEng(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine" });
	}
}
