#include "WPWBuildCommandlet.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/TextBlock.h"
#include "EdGraphSchema_K2.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "GameWidgets.h"
#include "K2Node_CallFunction.h"
#include "K2Node_DynamicCast.h"
#include "K2Node_Event.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_MacroInstance.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "K2Node_Self.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetStringLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetTextLibrary.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/CompilerResultsLog.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "ModWorldSubsystem.h"
#include "ModWorldSubsystemDataAsset.h"
#include "UObject/SavePackage.h"

DEFINE_LOG_CATEGORY_STATIC(LogWPW, Log, All);

namespace
{
const UEdGraphSchema_K2* K2() { return GetDefault<UEdGraphSchema_K2>(); }

struct FGraphBuilder
{
	UEdGraph* Graph;
	int32 X = 0, Y = 0;
	bool bOk = true;

	template <class T>
	T* Add(TFunctionRef<void(T*)> Setup)
	{
		T* Node = NewObject<T>(Graph);
		Node->CreateNewGuid();
		Node->NodePosX = (X += 300);
		Node->NodePosY = Y;
		Setup(Node);
		Graph->AddNode(Node, false, false);
		Node->AllocateDefaultPins();
		return Node;
	}

	UK2Node_CallFunction* Call(UClass* Class, const TCHAR* Name)
	{
		UFunction* Fn = Class->FindFunctionByName(Name);
		if (!Fn) { UE_LOG(LogWPW, Error, TEXT("function %s::%s not found"), *Class->GetName(), Name); bOk = false; return nullptr; }
		return Add<UK2Node_CallFunction>([Fn](UK2Node_CallFunction* N) { N->SetFromFunction(Fn); });
	}

	UK2Node_DynamicCast* PureCast(UClass* To)
	{
		UK2Node_DynamicCast* N = Add<UK2Node_DynamicCast>([To](UK2Node_DynamicCast* C) { C->TargetType = To; });
		N->SetPurity(true);
		return N;
	}

	static UEdGraphPin* Pin(UEdGraphNode* Node, const TCHAR* Name)
	{
		UEdGraphPin* P = Node ? Node->FindPin(FName(Name)) : nullptr;
		if (!P && Node)
		{
			FString Names;
			for (UEdGraphPin* Each : Node->Pins) Names += Each->PinName.ToString() + TEXT(", ");
			UE_LOG(LogWPW, Error, TEXT("pin '%s' not found on %s (has: %s)"), Name, *Node->GetNodeTitle(ENodeTitleType::ListView).ToString(), *Names);
		}
		return P;
	}

	void Link(UEdGraphNode* A, const TCHAR* APin, UEdGraphNode* B, const TCHAR* BPin)
	{
		UEdGraphPin* PA = Pin(A, APin);
		UEdGraphPin* PB = Pin(B, BPin);
		if (!PA || !PB || !K2()->TryCreateConnection(PA, PB))
		{
			UE_LOG(LogWPW, Error, TEXT("link %s.%s -> %s.%s failed"),
				A ? *A->GetName() : TEXT("null"), APin, B ? *B->GetName() : TEXT("null"), BPin);
			bOk = false;
		}
	}

	void Default(UEdGraphNode* Node, const TCHAR* PinName, const FString& Value)
	{
		if (UEdGraphPin* P = Pin(Node, PinName)) K2()->TrySetDefaultValue(*P, Value);
		else bOk = false;
	}
};
}

int32 UWPWBuildCommandlet::Main(const FString& Params)
{
	const FString PackageName = TEXT("/ValuePerKilo/WPW_Init");
	UPackage* Package = CreatePackage(*PackageName);
	UBlueprint* BP = FKismetEditorUtilities::CreateBlueprint(UModWorldSubsystem::StaticClass(), Package, TEXT("WPW_Init"),
		BPTYPE_Normal, UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass());

	FEdGraphPinType DoubleType;
	DoubleType.PinCategory = UEdGraphSchema_K2::PC_Real;
	DoubleType.PinSubCategory = UEdGraphSchema_K2::PC_Double;
	FBlueprintEditorUtils::AddMemberVariable(BP, TEXT("Accum"), DoubleType);

	UEdGraph* Graph = BP->UbergraphPages[0];
	FGraphBuilder G{Graph};
	UClass* Math = UKismetMathLibrary::StaticClass();
	UClass* Str = UKismetStringLibrary::StaticClass();
	UClass* Txt = UKismetTextLibrary::StaticClass();

	// OnWorldBeginPlay -> SetTickEnabled(true)
	int32 PosY = 0;
	UK2Node_Event* Begin = FKismetEditorUtilities::AddDefaultEventNode(BP, Graph, TEXT("OnWorldBeginPlay"), UModWorldSubsystem::StaticClass(), PosY);
	UK2Node_CallFunction* EnableTick = G.Call(UModWorldSubsystem::StaticClass(), TEXT("SetTickEnabled"));
	G.Default(EnableTick, TEXT("bIsEnabled"), TEXT("true"));
	G.Link(Begin, TEXT("then"), EnableTick, TEXT("execute"));

	// OnTick: Accum += DeltaTime; if Accum >= 0.1 { Accum = 0; process tooltips }
	G.X = 0; G.Y = 400; PosY = 400;
	UK2Node_Event* Tick = FKismetEditorUtilities::AddDefaultEventNode(BP, Graph, TEXT("OnTick"), UModWorldSubsystem::StaticClass(), PosY);
	auto GetAccum = [&] { return G.Add<UK2Node_VariableGet>([](UK2Node_VariableGet* N) { N->VariableReference.SetSelfMember(TEXT("Accum")); }); };
	auto SetAccum = [&] { return G.Add<UK2Node_VariableSet>([](UK2Node_VariableSet* N) { N->VariableReference.SetSelfMember(TEXT("Accum")); }); };

	UK2Node_CallFunction* Sum = G.Call(Math, TEXT("Add_DoubleDouble"));
	G.Link(GetAccum(), TEXT("Accum"), Sum, TEXT("A"));
	G.Link(Tick, TEXT("DeltaTime"), Sum, TEXT("B"));
	UK2Node_VariableSet* StoreSum = SetAccum();
	G.Link(Tick, TEXT("then"), StoreSum, TEXT("execute"));
	G.Link(Sum, TEXT("ReturnValue"), StoreSum, TEXT("Accum"));

	UK2Node_CallFunction* Due = G.Call(Math, TEXT("GreaterEqual_DoubleDouble"));
	G.Link(GetAccum(), TEXT("Accum"), Due, TEXT("A"));
	G.Default(Due, TEXT("B"), TEXT("0.1"));
	UK2Node_IfThenElse* IfDue = G.Add<UK2Node_IfThenElse>([](UK2Node_IfThenElse*) {});
	G.Link(StoreSum, TEXT("then"), IfDue, TEXT("execute"));
	G.Link(Due, TEXT("ReturnValue"), IfDue, TEXT("Condition"));

	UK2Node_VariableSet* Reset = SetAccum();
	G.Default(Reset, TEXT("Accum"), TEXT("0.0"));
	G.Link(IfDue, TEXT("then"), Reset, TEXT("execute"));

	UK2Node_CallFunction* FindAll = G.Call(UWidgetBlueprintLibrary::StaticClass(), TEXT("GetAllWidgetsOfClass"));
	if (FindAll)
	{
		if (UEdGraphPin* Ctx = FindAll->FindPin(TEXT("WorldContextObject"))) Ctx->bHidden = false;
		UEdGraphPin* ClassPin = G.Pin(FindAll, TEXT("WidgetClass"));
		if (ClassPin) K2()->TrySetDefaultObject(*ClassPin, UItemInfo::StaticClass());
		FindAll->ReconstructNode();  // resolve FoundWidgets to TArray<ItemInfo>
		G.Default(FindAll, TEXT("TopLevelOnly"), TEXT("false"));
		UK2Node_Self* Self = G.Add<UK2Node_Self>([](UK2Node_Self*) {});
		G.Link(Self, TEXT("self"), FindAll, TEXT("WorldContextObject"));
	}
	G.Link(Reset, TEXT("then"), FindAll, TEXT("execute"));

	UBlueprint* Macros = LoadObject<UBlueprint>(nullptr, TEXT("/Engine/EditorBlueprintResources/StandardMacros.StandardMacros"));
	UEdGraph* ForEachGraph = nullptr;
	if (Macros) for (UEdGraph* M : Macros->MacroGraphs) if (M->GetFName() == TEXT("ForEachLoop")) ForEachGraph = M;
	if (!ForEachGraph) { UE_LOG(LogWPW, Error, TEXT("ForEachLoop macro not found")); return 1; }
	UK2Node_MacroInstance* Loop = G.Add<UK2Node_MacroInstance>([ForEachGraph](UK2Node_MacroInstance* N) { N->SetMacroGraph(ForEachGraph); });
	G.Link(FindAll, TEXT("then"), Loop, TEXT("Exec"));
	G.Link(FindAll, TEXT("FoundWidgets"), Loop, TEXT("Array"));
	UEdGraphNode* Elem = Loop;
	const TCHAR* ElemPin = TEXT("Array Element");

	// field text block: Info.<Field>.CommonTextObj (stub properties, resolved by name at runtime)
	auto FieldWidget = [&](const TCHAR* Field) -> UK2Node_VariableGet*
	{
		UK2Node_VariableGet* Get = G.Add<UK2Node_VariableGet>([Field](UK2Node_VariableGet* N) {
			N->VariableReference.SetExternalMember(FName(Field), UItemInfo::StaticClass());
		});
		G.Link(Elem, ElemPin, Get, TEXT("self"));
		return Get;
	};
	auto FieldBlock = [&](const TCHAR* Field) -> UK2Node_VariableGet*
	{
		UK2Node_VariableGet* Outer = FieldWidget(Field);
		UK2Node_VariableGet* Inner = G.Add<UK2Node_VariableGet>([](UK2Node_VariableGet* N) {
			N->VariableReference.SetExternalMember(TEXT("CommonTextObj"), UTextWidget::StaticClass());
		});
		G.Link(Outer, Field, Inner, TEXT("self"));
		return Inner;
	};
	auto TextOf = [&](UK2Node_VariableGet* Block) -> UK2Node_CallFunction*
	{
		UK2Node_CallFunction* Get = G.Call(UTextBlock::StaticClass(), TEXT("GetText"));
		G.Link(Block, TEXT("CommonTextObj"), Get, TEXT("self"));
		UK2Node_CallFunction* ToStr = G.Call(Txt, TEXT("Conv_TextToString"));
		G.Link(Get, TEXT("ReturnValue"), ToStr, TEXT("InText"));
		return ToStr;
	};

	G.X = 600; G.Y = 900;
	UK2Node_VariableGet* PriceBlock = FieldBlock(TEXT("Price"));
	UK2Node_CallFunction* PriceStr = TextOf(PriceBlock);
	G.Y = 1200;
	UK2Node_CallFunction* WeightStr = TextOf(FieldBlock(TEXT("Weight")));
	G.Y = 1500;
	UK2Node_CallFunction* UnitStr = TextOf(FieldBlock(TEXT("OneItemWeight")));
	UK2Node_VariableGet* UnitWidget = FieldWidget(TEXT("OneItemWeight"));
	UK2Node_CallFunction* UnitShown = G.Call(UWidget::StaticClass(), TEXT("IsVisible"));
	G.Link(UnitWidget, TEXT("OneItemWeight"), UnitShown, TEXT("self"));

	// per-unit weight if shown (stacks), else total weight; "(0,35 kg)" -> "0.35 kg"
	G.Y = 1800;
	UK2Node_CallFunction* PickWeight = G.Call(Math, TEXT("SelectString"));
	G.Link(UnitStr, TEXT("ReturnValue"), PickWeight, TEXT("A"));
	G.Link(WeightStr, TEXT("ReturnValue"), PickWeight, TEXT("B"));
	G.Link(UnitShown, TEXT("ReturnValue"), PickWeight, TEXT("bPickA"));
	UK2Node_CallFunction* NoParen = G.Call(Str, TEXT("Replace"));
	G.Link(PickWeight, TEXT("ReturnValue"), NoParen, TEXT("SourceString"));
	G.Default(NoParen, TEXT("From"), TEXT("("));
	G.Default(NoParen, TEXT("To"), TEXT(""));
	UK2Node_CallFunction* Dot = G.Call(Str, TEXT("Replace"));
	G.Link(NoParen, TEXT("ReturnValue"), Dot, TEXT("SourceString"));
	G.Default(Dot, TEXT("From"), TEXT(","));
	G.Default(Dot, TEXT("To"), TEXT("."));
	UK2Node_CallFunction* Kg = G.Call(Str, TEXT("Conv_StringToDouble"));
	G.Link(Dot, TEXT("ReturnValue"), Kg, TEXT("InString"));
	UK2Node_CallFunction* Price = G.Call(Str, TEXT("Conv_StringToDouble"));
	G.Link(PriceStr, TEXT("ReturnValue"), Price, TEXT("InString"));

	// go = visible && price > 0 && kg > 0 && !price text already contains "/kg"
	G.Y = 2100;
	UK2Node_CallFunction* Shown = G.Call(UWidget::StaticClass(), TEXT("IsVisible"));
	G.Link(Elem, ElemPin, Shown, TEXT("self"));
	UK2Node_CallFunction* HasPrice = G.Call(Math, TEXT("Greater_DoubleDouble"));
	G.Link(Price, TEXT("ReturnValue"), HasPrice, TEXT("A"));
	UK2Node_CallFunction* HasKg = G.Call(Math, TEXT("Greater_DoubleDouble"));
	G.Link(Kg, TEXT("ReturnValue"), HasKg, TEXT("A"));
	UK2Node_CallFunction* Done = G.Call(Str, TEXT("Contains"));
	G.Link(PriceStr, TEXT("ReturnValue"), Done, TEXT("SearchIn"));
	G.Default(Done, TEXT("Substring"), TEXT("/kg"));
	UK2Node_CallFunction* NotDone = G.Call(Math, TEXT("Not_PreBool"));
	G.Link(Done, TEXT("ReturnValue"), NotDone, TEXT("A"));
	UK2Node_CallFunction* And1 = G.Call(Math, TEXT("BooleanAND"));
	G.Link(Shown, TEXT("ReturnValue"), And1, TEXT("A"));
	G.Link(HasPrice, TEXT("ReturnValue"), And1, TEXT("B"));
	UK2Node_CallFunction* And2 = G.Call(Math, TEXT("BooleanAND"));
	G.Link(HasKg, TEXT("ReturnValue"), And2, TEXT("A"));
	G.Link(NotDone, TEXT("ReturnValue"), And2, TEXT("B"));
	UK2Node_CallFunction* Go = G.Call(Math, TEXT("BooleanAND"));
	G.Link(And1, TEXT("ReturnValue"), Go, TEXT("A"));
	G.Link(And2, TEXT("ReturnValue"), Go, TEXT("B"));
	UK2Node_IfThenElse* IfGo = G.Add<UK2Node_IfThenElse>([](UK2Node_IfThenElse*) {});
	G.Link(Loop, TEXT("LoopBody"), IfGo, TEXT("execute"));
	G.Link(Go, TEXT("ReturnValue"), IfGo, TEXT("Condition"));

	// label = v < 500 ? "<1k/kg" : round(v / 1000) + "k/kg";  text = label + " · " + price
	G.Y = 2400;
	UK2Node_CallFunction* PerKg = G.Call(Math, TEXT("Divide_DoubleDouble"));
	G.Link(Price, TEXT("ReturnValue"), PerKg, TEXT("A"));
	G.Link(Kg, TEXT("ReturnValue"), PerKg, TEXT("B"));
	UK2Node_CallFunction* InK = G.Call(Math, TEXT("Divide_DoubleDouble"));
	G.Link(PerKg, TEXT("ReturnValue"), InK, TEXT("A"));
	G.Default(InK, TEXT("B"), TEXT("1000.0"));
	UK2Node_CallFunction* Rounded = G.Call(Math, TEXT("Round"));
	G.Link(InK, TEXT("ReturnValue"), Rounded, TEXT("A"));
	UK2Node_CallFunction* Num = G.Call(Str, TEXT("Conv_IntToString"));
	G.Link(Rounded, TEXT("ReturnValue"), Num, TEXT("InInt"));
	UK2Node_CallFunction* WithUnit = G.Call(Str, TEXT("Concat_StrStr"));
	G.Link(Num, TEXT("ReturnValue"), WithUnit, TEXT("A"));
	G.Default(WithUnit, TEXT("B"), TEXT("k/kg"));
	UK2Node_CallFunction* Small = G.Call(Math, TEXT("Less_DoubleDouble"));
	G.Link(PerKg, TEXT("ReturnValue"), Small, TEXT("A"));
	G.Default(Small, TEXT("B"), TEXT("500.0"));
	UK2Node_CallFunction* Label = G.Call(Math, TEXT("SelectString"));
	G.Default(Label, TEXT("A"), TEXT("<1k/kg"));
	G.Link(WithUnit, TEXT("ReturnValue"), Label, TEXT("B"));
	G.Link(Small, TEXT("ReturnValue"), Label, TEXT("bPickA"));
	UK2Node_CallFunction* WithDot = G.Call(Str, TEXT("Concat_StrStr"));
	G.Link(Label, TEXT("ReturnValue"), WithDot, TEXT("A"));
	G.Default(WithDot, TEXT("B"), FString(TEXT(" ")) + TCHAR(0x00B7) + TEXT(" "));
	UK2Node_CallFunction* Full = G.Call(Str, TEXT("Concat_StrStr"));
	G.Link(WithDot, TEXT("ReturnValue"), Full, TEXT("A"));
	G.Link(PriceStr, TEXT("ReturnValue"), Full, TEXT("B"));
	UK2Node_CallFunction* AsText = G.Call(Txt, TEXT("Conv_StringToText"));
	G.Link(Full, TEXT("ReturnValue"), AsText, TEXT("InString"));
	UK2Node_CallFunction* Write = G.Call(UTextBlock::StaticClass(), TEXT("SetText"));
	G.Link(IfGo, TEXT("then"), Write, TEXT("execute"));
	G.Link(PriceBlock, TEXT("CommonTextObj"), Write, TEXT("self"));
	G.Link(AsText, TEXT("ReturnValue"), Write, TEXT("InText"));

	if (!G.bOk) { UE_LOG(LogWPW, Error, TEXT("graph construction failed")); return 1; }

	FCompilerResultsLog Results;
	FKismetEditorUtilities::CompileBlueprint(BP, EBlueprintCompileOptions::None, &Results);
	for (const TSharedRef<FTokenizedMessage>& Msg : Results.Messages)
		UE_LOG(LogWPW, Display, TEXT("compile: %s"), *Msg->ToText().ToString());
	if (Results.NumErrors > 0 || BP->Status == BS_Error) { UE_LOG(LogWPW, Error, TEXT("compile failed (%d errors)"), Results.NumErrors); return 1; }

	const FString File = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs Args;
	Args.TopLevelFlags = RF_Public | RF_Standalone;
	if (!UPackage::SavePackage(Package, BP, *File, Args)) { UE_LOG(LogWPW, Error, TEXT("save failed: %s"), *File); return 1; }
	UE_LOG(LogWPW, Display, TEXT("saved %s"), *File);

	// registration: the game instantiates the subsystems listed in *_WorldSubsystemData assets
	const FString RegName = TEXT("Autogenerated_1792500777_WorldSubsystemData");
	UPackage* RegPackage = CreatePackage(*(TEXT("/ValuePerKilo/") + RegName));
	UModWorldSubsystemDataAsset* Reg = NewObject<UModWorldSubsystemDataAsset>(RegPackage, *RegName, RF_Public | RF_Standalone);
	Reg->ModWorldSubsystems.Add(BP->GeneratedClass->GetPathName());
	const FString RegFile = FPackageName::LongPackageNameToFilename(RegPackage->GetName(), FPackageName::GetAssetPackageExtension());
	if (!UPackage::SavePackage(RegPackage, Reg, *RegFile, Args)) { UE_LOG(LogWPW, Error, TEXT("save failed: %s"), *RegFile); return 1; }
	UE_LOG(LogWPW, Display, TEXT("saved %s (%s)"), *RegFile, *Reg->ModWorldSubsystems[0]);
	return 0;
}
