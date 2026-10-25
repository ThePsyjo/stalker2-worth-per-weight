// Stub of the game's /Script/Stalker2.ModWorldSubsystem (layout from the UE4SS dump of 2.0.6).
// Only the reflected interface matters: cooked Blueprints bind to these names at runtime.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "StructUtils/InstancedStruct.h"
#include "ModWorldSubsystem.generated.h"

UCLASS(Abstract, Blueprintable)
class STALKER2_API UModWorldSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	using UTickableWorldSubsystem::OnWorldBeginPlay;  // the game's Blueprint event shares the engine virtual's name

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	bool bIsTickEnabled = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	bool bShouldSaveData = false;

	UFUNCTION(BlueprintCallable)
	void SetTickEnabled(bool bIsEnabled) { bIsTickEnabled = bIsEnabled; }

	UFUNCTION(BlueprintCallable)
	bool SetDataForSave(UPARAM(ref) FInstancedStruct& SaveData) { return false; }

	UFUNCTION(BlueprintImplementableEvent)
	void PreDeinitialize();

	UFUNCTION(BlueprintImplementableEvent)
	void OnWorldBeginPlay(UWorld* World);

	UFUNCTION(BlueprintImplementableEvent)
	void OnTick(float DeltaTime);

	UFUNCTION(BlueprintImplementableEvent)
	void OnSaveDataRequested();

	UFUNCTION(BlueprintImplementableEvent)
	void OnSaveDataLoaded(const FInstancedStruct& SaveData);

	UFUNCTION(BlueprintImplementableEvent)
	void OnInitialized();

	UFUNCTION(BlueprintImplementableEvent)
	void LevelActorsPreInit(ULevel* Level, const TArray<AActor*>& Actors);

	UFUNCTION(BlueprintPure)
	UWorld* GetTickableWorld() const { return GetWorld(); }

	// FTickableGameObject
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UModWorldSubsystem, STATGROUP_Tickables); }
	virtual void Tick(float DeltaTime) override { if (bIsTickEnabled) OnTick(DeltaTime); }
};
