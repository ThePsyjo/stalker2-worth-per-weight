using UnrealBuildTool;

public class Stalker2 : ModuleRules
{
	public Stalker2(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "UMG", "CommonUI" });
	}
}
