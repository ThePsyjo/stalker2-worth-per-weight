// Stub of the game's /Script/Engine.ModWorldSubsystemDataAsset: lists Blueprint ModWorldSubsystems to instantiate.
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ModWorldSubsystemDataAsset.generated.h"

UCLASS()
class WPWENG_API UModWorldSubsystemDataAsset : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere)
	TArray<FString> ModWorldSubsystems;
};
