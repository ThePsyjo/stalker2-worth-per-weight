#pragma once

#include "Commandlets/Commandlet.h"
#include "WPWBuildCommandlet.generated.h"

// Builds /ValuePerKilo/WPW_Init (a ModWorldSubsystem Blueprint) from code.
// UnrealEditor-Cmd Stalker2.uproject -run=WPWBuild
UCLASS()
class UWPWBuildCommandlet : public UCommandlet
{
	GENERATED_BODY()
public:
	virtual int32 Main(const FString& Params) override;
};
