using UnrealBuildTool;

public class WPWTools : ModuleRules
{
	public WPWTools(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PrivateDependencyModuleNames.AddRange(new[] {
			"Core", "CoreUObject", "Engine", "UMG", "UnrealEd", "BlueprintGraph", "Kismet", "KismetCompiler", "Stalker2", "WPWEng"
		});
	}
}
