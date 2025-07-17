// Author: Tom Werner (MajorT), 2025


#include "K2Node_ForEachSet.h"

#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "K2Node_CallFunction.h"
#include "K2Node_InternalIterate.h"
#include "KismetCompiler.h"
#include "Kismet/BlueprintMapLibrary.h"
#include "Kismet/BlueprintSetLibrary.h"
#include "Kismet2/BlueprintEditorUtils.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(K2Node_ForEachSet)

#define LOCTEXT_NAMESPACE "K2Node_ForEachSet"

namespace ForEachSet_PinNames
{
	static const FName SetPin(TEXT("SetPin"));
	static const FName BreakPin(TEXT("BreakPin"));
	static const FName ValuePin(TEXT("ValuePin"));
	static const FName CompletePin(TEXT("CompletePin"));
	static const FName IndexPin(TEXT("IndexPin"));
}

UK2Node_ForEachSet::UK2Node_ForEachSet()
{
	ValueName = LOCTEXT("ValuePin_FriendlyName", "Map Value").ToString();
	IndexName = LOCTEXT("IndexPin_FriendlyName", "Map Index").ToString();
}

void UK2Node_ForEachSet::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
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

FText UK2Node_ForEachSet::GetMenuCategory() const
{
	return LOCTEXT("NodeMenuCategory", "Utilities|Array");
}

void UK2Node_ForEachSet::AllocateDefaultPins()
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
	_params.ContainerType = EPinContainerType::Set;
	_params.ValueTerminalType.TerminalCategory = UEdGraphSchema_K2::PC_Wildcard;

	// INPUT: Set Type
	UEdGraphPin* SetPin =
		CreatePin( EGPD_Input, UEdGraphSchema_K2::PC_Wildcard, ForEachSet_PinNames::SetPin, _params);
	if (ensure(SetPin))
	{
		SetPin->PinType.bIsConst = true;
		SetPin->PinType.bIsReference = true;
		SetPin->PinFriendlyName = LOCTEXT( "SetPin_FriendlyName", "Set" );	
	}

	// INPUT: Break pin
	UEdGraphPin* BreakPin =
		CreatePin( EGPD_Input, UEdGraphSchema_K2::PC_Exec, ForEachSet_PinNames::BreakPin);
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

	// OUTPUT: Value
	UEdGraphPin* ValuePin =
		CreatePin( EGPD_Output, UEdGraphSchema_K2::PC_Wildcard, ForEachSet_PinNames::ValuePin);
	if (ensure(ValuePin))
	{
		ValuePin->PinFriendlyName = FText::FromString(ValueName);
	}

	// OUTPUT: Index
	UEdGraphPin* IndexPin =
		CreatePin( EGPD_Output, UEdGraphSchema_K2::PC_Int, ForEachSet_PinNames::IndexPin);
	if (ensure(IndexPin))
	{
		IndexPin->PinFriendlyName = FText::FromString(IndexName);
	}

	// OUTPUT: Completed Exec
	UEdGraphPin* CompletedPin =
		CreatePin( EGPD_Output, UEdGraphSchema_K2::PC_Exec, ForEachSet_PinNames::CompletePin);
	if (ensure(CompletedPin))
	{
		CompletedPin->PinFriendlyName = LOCTEXT( "CompletedPin_FriendlyName", "Completed" );
	}

	if (bAutoAssignPins)
	{
		SetPin->PinType = CachedInputType;
		ValuePin->PinType = CachedValueType;
	}
	else
	{
		CachedInputWildcardType = SetPin->PinType;
		CachedWildcardType = ValuePin->PinType;

		CachedInputType = SetPin->PinType;
		CachedValueType = ValuePin->PinType;

		bAutoAssignPins = true; // We've assigned the pins, so next time we load auto assign them
	}
}

void UK2Node_ForEachSet::ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);

	if (CheckForErrors( CompilerContext ))
	{
		BreakAllNodeLinks( );
		return;
	}

	const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>( );

	// This node
	UEdGraphPin* ForEach_Exec = GetExecPin();
	UEdGraphPin* ForEach_Set = GetInputSetPin();
	UEdGraphPin* ForEach_Break = GetInputBreakPin();
	UEdGraphPin* ForEach_ForEach = GetLoopBodyPin();
	UEdGraphPin* ForEach_Value = GetValuePin();
	UEdGraphPin* ForEach_Completed = GetCompletePin();
	UEdGraphPin* ForEach_Index = GetIndexPin();

	// Create the getter node
	UK2Node_CallFunction* CallFunc_ToArray = CompilerContext.SpawnIntermediateNode< UK2Node_CallFunction >( this, SourceGraph );
	CallFunc_ToArray->FunctionReference.SetExternalMember( GET_FUNCTION_NAME_CHECKED( UBlueprintSetLibrary, Set_ToArray ), UBlueprintMapLibrary::StaticClass( ) );
	CallFunc_ToArray->AllocateDefaultPins( );
	UEdGraphPin* ToArray_Exec = CallFunc_ToArray->GetExecPin();
	UEdGraphPin* ToArray_Set = CallFunc_ToArray->FindPinChecked(TEXT("A"));
	UEdGraphPin* ToArray_Return = CallFunc_ToArray->FindPinChecked(TEXT("Result"));
	UEdGraphPin* ToArray_Then = CallFunc_ToArray->GetThenPin();

	CompilerContext.CopyPinLinksToIntermediate( *ForEach_Set, *ToArray_Set );
	CallFunc_ToArray->PinConnectionListChanged(ToArray_Set);

	CompilerContext.MovePinLinksToIntermediate(*ForEach_Exec, *ToArray_Exec);


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

	ToArray_Then->MakeLinkTo(Internal_Exec);
	Schema->TryCreateConnection(ToArray_Return, Internal_Array);

	// No more intermediate nodes, just wire up directly
	CompilerContext.MovePinLinksToIntermediate(*ForEach_Value, *Internal_Element);
	CompilerContext.MovePinLinksToIntermediate(*ForEach_Index, *Internal_Index);

	// Break the links as our internal iterator node will handle the rest
	BreakAllNodeLinks();
}

FText UK2Node_ForEachSet::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("NodeTitle", "For Each Set");
}

FText UK2Node_ForEachSet::GetTooltipText() const
{
	return LOCTEXT("NodeTooltip", "Fuck it, just give up.");
}

FText UK2Node_ForEachSet::GetKeywords() const
{
	return FText::FromString(TEXT("For,Each,Loop,Set"));
}

FSlateIcon UK2Node_ForEachSet::GetIconAndTint(FLinearColor& OutColor) const
{
	static const FSlateIcon Icon = FSlateIcon(FAppStyle::GetAppStyleSetName(), "GraphEditor.Macro.ForEach_16x");
	OutColor = FLinearColor::White;
	return Icon;
}

FLinearColor UK2Node_ForEachSet::GetNodeTitleColor() const
{
	return FLinearColor::White;
}

void UK2Node_ForEachSet::PinConnectionListChanged(UEdGraphPin* Pin)
{
	Super::PinConnectionListChanged(Pin);

	if (Pin == nullptr)
	{
		return;
	}

	if (Pin->PinName == ForEachSet_PinNames::ValuePin)
	{
		UEdGraphPin* ValuePin = GetValuePin();
		UEdGraphPin* MapPin = GetInputSetPin();

		// If we're connected to nothing, reset all those pin types
		if ( (ValuePin->LinkedTo.Num() + MapPin->LinkedTo.Num()) <= 0)
		{
			MapPin->PinType = CachedInputWildcardType;
			ValuePin->PinType = CachedWildcardType;
		}
	}

	if (Pin->PinName == ForEachSet_PinNames::SetPin)
	{
		UEdGraphPin* ValuePin = GetValuePin();

		bool bShouldReconnect = false;
		if (Pin->LinkedTo.Num() > 0)
		{
			const UEdGraphPin* FirstPin = Pin->LinkedTo[0];

			// Only reconnect if the pin type has actually changed
			bShouldReconnect = Pin->PinType != FirstPin->PinType;

			Pin->PinType = FirstPin->PinType;
			ValuePin->PinType = FEdGraphPinType::GetTerminalTypeForContainer(FirstPin->PinType);
		}
		else
		{
			// If we have no connections anymore, reset pin types
			if (ValuePin->LinkedTo.Num() <= 0)
			{
				Pin->PinType = CachedInputWildcardType;
				ValuePin->PinType = CachedWildcardType;
				bShouldReconnect = true;
			}
		}

		CachedInputType = Pin->PinType;
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

		if (bShouldReconnect)
		{
			ReconnectPins(ValuePin);	
		}
	}
}

void UK2Node_ForEachSet::PostPasteNode()
{
	Super::PostPasteNode();

	if (const UEdGraphPin* SetPin = GetInputSetPin())
	{
		if (!SetPin->LinkedTo.Num())
		{
			bAutoAssignPins = false;
		}
	}
	else
	{
		bAutoAssignPins = false;
	}
}

void UK2Node_ForEachSet::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	bool bRefresh = false;

	if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(ThisClass, ValueName))
	{
		GetValuePin()->PinFriendlyName = FText::FromString(ValueName);
		bRefresh = true;
	}
	else if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(ThisClass, IndexName))
	{
		GetIndexPin()->PinFriendlyName = FText::FromString(IndexName);
		bRefresh = true;
	}

	if (bRefresh)
	{
		// Poke the graph to update the visuals based on the above changes
		GetGraph()->NotifyGraphChanged();
		FBlueprintEditorUtils::MarkBlueprintAsModified(GetBlueprint());
	}
}

UEdGraphPin* UK2Node_ForEachSet::GetInputSetPin() const
{
	return FindPinChecked(ForEachSet_PinNames::SetPin);
}

UEdGraphPin* UK2Node_ForEachSet::GetInputBreakPin() const
{
	return FindPinChecked(ForEachSet_PinNames::BreakPin);
}

UEdGraphPin* UK2Node_ForEachSet::GetLoopBodyPin() const
{
	return FindPinChecked(UEdGraphSchema_K2::PN_Then);
}

UEdGraphPin* UK2Node_ForEachSet::GetValuePin() const
{
	return FindPinChecked(ForEachSet_PinNames::ValuePin);
}

UEdGraphPin* UK2Node_ForEachSet::GetCompletePin() const
{
	return FindPinChecked(ForEachSet_PinNames::CompletePin);
}

UEdGraphPin* UK2Node_ForEachSet::GetIndexPin() const
{
	return FindPinChecked(ForEachSet_PinNames::IndexPin);
}

bool UK2Node_ForEachSet::CheckForErrors(const FKismetCompilerContext& CompilerContext)
{
	if (GetInputSetPin()->LinkedTo.Num() == 0)
	{
		CompilerContext.MessageLog.Error(
			*LOCTEXT( "NoSetEntry", "For Each Set node @@ requires a set input.").ToString(),
			this);
		return true;
	}

	return false;
}

#undef LOCTEXT_NAMESPACE