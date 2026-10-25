// Stubs of game widget classes referenced by mod Blueprints (names/hierarchy from the UE4SS dump of 2.0.6).
// Cooked Blueprints resolve properties by name against the game's real classes at load time; the
// BlueprintReadOnly specifiers below only exist so the editor lets a Blueprint read them.
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CommonTextBlock.h"
#include "GameWidgets.generated.h"

UCLASS()
class STALKER2_API UWidgetBase : public UUserWidget
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable)
	void UpdateWidget() {}
};

UCLASS()
class STALKER2_API ULocalizationWidget : public UWidgetBase
{
	GENERATED_BODY()
};

// W_Text_C: the shown text lives in the child CommonTextBlock CommonTextObj (GetText() returns a stale default).
UCLASS()
class STALKER2_API UTextWidget : public ULocalizationWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UCommonTextBlock> CommonTextObj;
};

// Inventory / trade item tooltip (W_ItemInfo_C).
UCLASS()
class STALKER2_API UItemInfo : public UWidgetBase
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UTextWidget> HeaderText;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UTextWidget> Price;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UTextWidget> Weight;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UTextWidget> OneItemWeight;
};
