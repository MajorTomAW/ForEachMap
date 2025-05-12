// Copyright © 2025 MajorT. All Rights Reserved.


#include "K2Node_ForEachMap.h"

#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "K2Node_CallFunction.h"
#include "K2Node_InternalIterate.h"
#include "KismetCompiler.h"
#include "Kismet/BlueprintMapLibrary.h"
#include "Kismet2/BlueprintEditorUtils.h"

#define LOCTEXT_NAMESPACE "K2Node_ForEachMap"

namespace ForEachMap_PinNames
{
	static const FName MapPin(TEXT("MapPin"));
	static const FName BreakPin(TEXT("BreakPin"));
	static const FName KeyPin(TEXT("KeyPin"));
	static const FName ValuePin(TEXT("ValuePin"));
	static const FName CompletePin(TEXT("CompletePin"));
	static const FName IndexPin(TEXT("IndexPin"));
}

UK2Node_ForEachMap::UK2Node_ForEachMap()
{
}

void UK2Node_ForEachMap::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	Super::GetMenuActions(ActionRegistrar);

	UClass* Action = GetClass();
	if (ActionRegistrar.IsOpenForRegistration(Action))
	{
		UBlueprintNodeSpawner* GetNodeSpawner = UBlueprintNodeSpawner::Create(Action);
		check(GetNodeSpawner != nullptr);

		ActionRegistrar.AddBlueprintAction(Action, GetNodeSpawner);
	}
}

FText UK2Node_ForEachMap::GetMenuCategory() const
{
	return LOCTEXT("NodeMenuCategory", "Utilities|Array");
}

void UK2Node_ForEachMap::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();

	// Add default pins here
	// INPUT: Exec
	UEdGraphPin* ExecPin =
		CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute);
	if (ensure(ExecPin))
	{
		ExecPin->PinFriendlyName = LOCTEXT("ExecPin_FriendlyName", "Execute");
	}

	FCreatePinParams _params;
	_params.ContainerType = EPinContainerType::Map;
	_params.ValueTerminalType.TerminalCategory = UEdGraphSchema_K2::PC_Wildcard;

	// INPUT: Map Type
	UEdGraphPin* MapPin =
		CreatePin( EGPD_Input, UEdGraphSchema_K2::PC_Wildcard, ForEachMap_PinNames::MapPin, _params);
	if (ensure(MapPin))
	{
		MapPin->PinType.bIsConst = true;
		MapPin->PinType.bIsReference = true;
		MapPin->PinFriendlyName = LOCTEXT( "MapPin_FriendlyName", "Map" );	
	}

	// INPUT: Break pin
	UEdGraphPin* BreakPin =
		CreatePin( EGPD_Input, UEdGraphSchema_K2::PC_Exec, ForEachMap_PinNames::BreakPin);
	if (ensure(BreakPin))
	{
		BreakPin->PinFriendlyName = LOCTEXT( "BreakPin_FriendlyName", "Break" );
	}

	// OUTPUT: Loop Body
	UEdGraphPin* LoopBodyPin =
		CreatePin( EGPD_Output, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Then);
	if (ensure(LoopBodyPin))
	{
		LoopBodyPin->PinFriendlyName = LOCTEXT( "ForEachPin_FriendlyName", "Loop Body" );	
	}

	// OUTPUT: Key
	UEdGraphPin* KeyPin =
		CreatePin( EGPD_Output, UEdGraphSchema_K2::PC_Wildcard, ForEachMap_PinNames::KeyPin);
	if (ensure(KeyPin))
	{
		KeyPin->PinFriendlyName = LOCTEXT("KeyPin_FriendlyName", "Map Key");	
	}

	// OUTPUT: Value
	UEdGraphPin* ValuePin =
		CreatePin( EGPD_Output, UEdGraphSchema_K2::PC_Wildcard, ForEachMap_PinNames::ValuePin);
	if (ensure(ValuePin))
	{
		ValuePin->PinFriendlyName = LOCTEXT("ValuePin_FriendlyName", "Map Value");
	}

	// OUTPUT: Index
	UEdGraphPin* IndexPin =
		CreatePin( EGPD_Output, UEdGraphSchema_K2::PC_Int, ForEachMap_PinNames::IndexPin);
	if (ensure(IndexPin))
	{
		IndexPin->PinFriendlyName = LOCTEXT("IndexPin_FriendlyName", "Map Index");
	}

	// OUTPUT: Completed Exec
	UEdGraphPin* CompletedPin =
		CreatePin( EGPD_Output, UEdGraphSchema_K2::PC_Exec, ForEachMap_PinNames::CompletePin);
	if (ensure(CompletedPin))
	{
		CompletedPin->PinFriendlyName = LOCTEXT( "CompletedPin_FriendlyName", "Completed" );
	}

	if (bAutoAssignPins)
	{
		MapPin->PinType = CachedInputType;
		KeyPin->PinType = CachedKeyType;
		ValuePin->PinType = CachedValueType;
	}
	else
	{
		CachedInputWildcardType = MapPin->PinType;
		CachedWildcardType = ValuePin->PinType;

		CachedInputType = MapPin->PinType;
		CachedKeyType = KeyPin->PinType;
		CachedValueType = ValuePin->PinType;

		bAutoAssignPins = true; // We've assigned the pins, so next time we load auto assign them
	}
}

void UK2Node_ForEachMap::ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode( CompilerContext, SourceGraph );

	if (CheckForErrors( CompilerContext ))
	{
		BreakAllNodeLinks( );
		return;
	}

	const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>( );
	
	UEdGraphPin* ForEach_Exec = GetExecPin();
	UEdGraphPin* ForEach_Map = GetInputMapPin();
	UEdGraphPin* ForEach_Break = GetInputBreakPin();
	UEdGraphPin* ForEach_ForEach = GetLoopBodyPin();
	UEdGraphPin* ForEach_Key = GetKeyPin();
	UEdGraphPin* ForEach_Value = GetValuePin();
	UEdGraphPin* ForEach_Completed = GetCompletePin();
	UEdGraphPin* ForEach_Index = GetIndexPin();

	
	// Create the getter node
	UK2Node_CallFunction* CallFunc_GetKeys = CompilerContext.SpawnIntermediateNode< UK2Node_CallFunction >( this, SourceGraph );
	CallFunc_GetKeys->FunctionReference.SetExternalMember( GET_FUNCTION_NAME_CHECKED( UBlueprintMapLibrary, Map_Keys ), UBlueprintMapLibrary::StaticClass( ) );
	CallFunc_GetKeys->AllocateDefaultPins( );

	UEdGraphPin* Get_Exec = CallFunc_GetKeys->GetExecPin();
	UEdGraphPin* Get_Map = CallFunc_GetKeys->FindPinChecked(TEXT("TargetMap"));
	UEdGraphPin* Get_Return = CallFunc_GetKeys->FindPinChecked(TEXT("Keys"));
	UEdGraphPin* Get_Then = CallFunc_GetKeys->GetThenPin();

	CompilerContext.CopyPinLinksToIntermediate( *ForEach_Map, *Get_Map );
	CallFunc_GetKeys->PinConnectionListChanged(Get_Map);

	CompilerContext.MovePinLinksToIntermediate(*ForEach_Exec, *Get_Exec);

	
	// Create the internal iterator node
	UK2Node_InternalIterate* InternalIterate = CompilerContext.SpawnIntermediateNode<UK2Node_InternalIterate>( this, SourceGraph );
	InternalIterate->AllocateDefaultPins();

	UEdGraphPin* Internal_Exec = InternalIterate->GetExecPin();
	UEdGraphPin* Internal_Array = InternalIterate->GetArrayPin();
	UEdGraphPin* Internal_Index = InternalIterate->GetArrayIndexPin();
	UEdGraphPin* Internal_Break = InternalIterate->GetBreakPin();
	UEdGraphPin* Internal_ForEach = InternalIterate->GetForEachPin();
	UEdGraphPin* Internal_Element = InternalIterate->GetElementPin();
	UEdGraphPin* Internal_Completed = InternalIterate->GetCompletedPin();

	// All the exec pins wire up directly
	CompilerContext.MovePinLinksToIntermediate(*ForEach_ForEach, *Internal_ForEach);
	CompilerContext.MovePinLinksToIntermediate(*ForEach_Break, *Internal_Break);
	CompilerContext.MovePinLinksToIntermediate(*ForEach_Completed, *Internal_Completed);

	Get_Then->MakeLinkTo(Internal_Exec);
	Schema->TryCreateConnection(Get_Return, Internal_Array);

	// For each element is the key, wire up directly
	CompilerContext.MovePinLinksToIntermediate( *ForEach_Key, *Internal_Element);
	CompilerContext.MovePinLinksToIntermediate(*ForEach_Index, *Internal_Index);

	// Create the find node
	UK2Node_CallFunction* CallFunc_Find = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	CallFunc_Find->FunctionReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(UBlueprintMapLibrary, Map_Find), UBlueprintMapLibrary::StaticClass());
	CallFunc_Find->AllocateDefaultPins();

	UEdGraphPin* Find_Map = CallFunc_Find->FindPinChecked(TEXT("TargetMap"));
	UEdGraphPin* Find_Key = CallFunc_Find->FindPinChecked(TEXT("Key"));
	UEdGraphPin* Find_Return = CallFunc_Find->FindPinChecked(TEXT("Value"));

	CompilerContext.MovePinLinksToIntermediate(*ForEach_Map,*Find_Map);
	CallFunc_Find->PinConnectionListChanged(Find_Map);

	Schema->TryCreateConnection(Internal_Element,Find_Key);
	
	CompilerContext.MovePinLinksToIntermediate(*ForEach_Value,*Find_Return);

	// Break the links as our internal iterator node will handle the rest
	BreakAllNodeLinks();
}

FText UK2Node_ForEachMap::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("NodeTitle", "For Each Map");
}

FText UK2Node_ForEachMap::GetTooltipText() const
{
	return LOCTEXT("NodeTooltip", "Fuck it, just give up.");
}

FText UK2Node_ForEachMap::GetKeywords() const
{
	return FText::FromString(TEXT("For,Each,Loop,Map"));
}

FSlateIcon UK2Node_ForEachMap::GetIconAndTint(FLinearColor& OutColor) const
{
	static const FSlateIcon Icon = FSlateIcon(FAppStyle::GetAppStyleSetName(), "GraphEditor.Macro.ForEach_16x");
	return Icon;
}

void UK2Node_ForEachMap::PinConnectionListChanged(UEdGraphPin* Pin)
{
	Super::PinConnectionListChanged( Pin );

	if (Pin == nullptr)
		return;

	if (Pin->PinName == ForEachMap_PinNames::MapPin)
	{
		UEdGraphPin* ValuePin = GetValuePin( );
		UEdGraphPin* KeyPin = GetKeyPin( );

		if (Pin->LinkedTo.Num() > 0)
		{
			const UEdGraphPin* FirstPin = Pin->LinkedTo[0];

			Pin->PinType = FirstPin->PinType;
			KeyPin->PinType = FEdGraphPinType::GetTerminalTypeForContainer(FirstPin->PinType);
			ValuePin->PinType = FEdGraphPinType::GetPinTypeForTerminalType(FirstPin->PinType.PinValueType);
		}
		else
		{
			Pin->PinType = CachedInputWildcardType;
			KeyPin->PinType = ValuePin->PinType = CachedWildcardType;
		}

		CachedInputType = Pin->PinType;
		CachedKeyType = KeyPin->PinType;
		CachedValueType = ValuePin->PinType;

		auto ReconnectPins = [this](UEdGraphPin* Pin)
		{
			TArray<UEdGraphPin*>& LinkedPins = Pin->LinkedTo;
			Pin->BreakAllPinLinks(true);

			const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();
			for (UEdGraphPin* Connection : LinkedPins)
			{
				Schema->TryCreateConnection( Pin, Connection);
			}

			GetGraph()->NotifyGraphChanged();
			FBlueprintEditorUtils::MarkBlueprintAsModified(GetBlueprint());
		};

		ReconnectPins(KeyPin);
		ReconnectPins(ValuePin);
	}
}

void UK2Node_ForEachMap::PostPasteNode()
{
	Super::PostPasteNode( );

	if (const UEdGraphPin* MapPin = GetInputMapPin())
	{
		if (!MapPin->LinkedTo.Num())
		{
			bAutoAssignPins = false;
		}
	}
	else
	{
		bAutoAssignPins = false;
	}
}


UEdGraphPin* UK2Node_ForEachMap::GetInputMapPin() const
{
	return FindPinChecked(ForEachMap_PinNames::MapPin);
}

UEdGraphPin* UK2Node_ForEachMap::GetInputBreakPin() const
{
	return FindPinChecked(ForEachMap_PinNames::BreakPin);
}

UEdGraphPin* UK2Node_ForEachMap::GetLoopBodyPin() const
{
	return FindPinChecked(UEdGraphSchema_K2::PN_Then);
}

UEdGraphPin* UK2Node_ForEachMap::GetKeyPin() const
{
	return FindPinChecked(ForEachMap_PinNames::KeyPin);
}

UEdGraphPin* UK2Node_ForEachMap::GetValuePin() const
{
	return FindPinChecked(ForEachMap_PinNames::ValuePin);
}

UEdGraphPin* UK2Node_ForEachMap::GetCompletePin() const
{
	return FindPinChecked(ForEachMap_PinNames::CompletePin);
}

UEdGraphPin* UK2Node_ForEachMap::GetIndexPin() const
{
	return FindPinChecked(ForEachMap_PinNames::IndexPin);
}

bool UK2Node_ForEachMap::CheckForErrors(const FKismetCompilerContext& CompilerContext)
{
	if (GetInputMapPin()->LinkedTo.Num() == 0)
	{
		CompilerContext.MessageLog.Error(
			*LOCTEXT( "NoMapEntry", "For Each Map node @@ requires a map input.").ToString(),
			this);
		return true;
	}

	return false;
}

#undef LOCTEXT_NAMESPACE
