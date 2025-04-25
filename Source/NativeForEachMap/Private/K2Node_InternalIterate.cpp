// Copyright © 2025 MajorT. All Rights Reserved.


#include "K2Node_InternalIterate.h"

#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "K2Node_AssignmentStatement.h"
#include "K2Node_CallFunction.h"
#include "K2Node_ExecutionSequence.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_TemporaryVariable.h"
#include "KismetCompiler.h"
#include "Kismet/KismetArrayLibrary.h"
#include "Kismet/KismetMathLibrary.h"

#define LOCTEXT_NAMESPACE "K2Node_NativeForEach"

namespace PinNames
{
	static const FName ArrayPin(TEXT("ArrayPin"));
	static const FName BreakPin(TEXT("BreakPin"));
	static const FName ElementPin(TEXT("ElementPin"));
	static const FName ArrayIndexPin(TEXT("ArrayIndexPin"));
	static const FName CompletedPin(TEXT("CompletedPin"));
}

UEdGraphPin* UK2Node_InternalIterate::GetArrayPin() const
{
	return FindPinChecked(PinNames::ArrayPin);
}

UEdGraphPin* UK2Node_InternalIterate::GetBreakPin() const
{
	return FindPinChecked(PinNames::BreakPin);
}

UEdGraphPin* UK2Node_InternalIterate::GetForEachPin() const
{
	return FindPinChecked(UEdGraphSchema_K2::PN_Then);
}

UEdGraphPin* UK2Node_InternalIterate::GetElementPin() const
{
	return FindPinChecked(PinNames::ElementPin);
}

UEdGraphPin* UK2Node_InternalIterate::GetArrayIndexPin() const
{
	return FindPinChecked(PinNames::ArrayIndexPin);
}

UEdGraphPin* UK2Node_InternalIterate::GetCompletedPin() const
{
	return FindPinChecked(PinNames::CompletedPin);
}

void UK2Node_InternalIterate::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
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

FText UK2Node_InternalIterate::GetMenuCategory() const
{
	return LOCTEXT( "NodeMenu", "Core Utilities" );
}

void UK2Node_InternalIterate::AllocateDefaultPins()
{
	Super::AllocateDefaultPins( );

	// Create default pins here
	// INPUT: Exec
	UEdGraphPin* ExecPin =
		CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute);
	if (ensure(ExecPin))
	{
		ExecPin->PinFriendlyName = LOCTEXT( "ExecPin_FriendlyName", "Execute" );
	}

	// INPUT: Array Type
	UEdGraphPin* ArrayPin =
		CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Wildcard, PinNames::ArrayPin);
	if (ensure(ArrayPin))
	{
		ArrayPin->PinType.ContainerType = EPinContainerType::Array;
		ArrayPin->PinType.bIsConst = true;
		ArrayPin->PinType.bIsReference = true;
		ArrayPin->PinFriendlyName = LOCTEXT("ArrayPin_FriendlyName", "Array");	
	}
	RealWildcardType = ArrayPin->PinType;

	// INPUT: Break pin
	UEdGraphPin* BreakPin =
		CreatePin( EGPD_Input, UEdGraphSchema_K2::PC_Exec, PinNames::BreakPin);
	if (ensure(BreakPin))
	{
		BreakPin->PinFriendlyName = LOCTEXT("BreakPin_FriendlyName", "Break");
	}

	// OUTPUT: Loop Body
	UEdGraphPin* LoopBodyPin =
		CreatePin( EGPD_Output, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Then);
	if (ensure(LoopBodyPin))
	{
		LoopBodyPin->PinFriendlyName = LOCTEXT( "ForEachPin_FriendlyName", "Loop Body" );	
	}

	// OUTPUT: Array Element
	UEdGraphPin* ElementPin =
		CreatePin( EGPD_Output, UEdGraphSchema_K2::PC_Wildcard, PinNames::ElementPin);
	if (ensure(ElementPin))
	{
		ElementPin->PinFriendlyName = LOCTEXT( "ElementPin_FriendlyName", "Array Element" );	
	}

	// OUTPUT: Array Index
	UEdGraphPin* IndexPin =
		CreatePin( EGPD_Output, UEdGraphSchema_K2::PC_Int, PinNames::ArrayIndexPin);
	if (ensure(IndexPin))
	{
		IndexPin->PinFriendlyName = LOCTEXT( "IndexPin_FriendlyName", "Array Index");
	}

	// OUTPUT: Completed Exec
	UEdGraphPin* CompletedPin =
		CreatePin( EGPD_Output, UEdGraphSchema_K2::PC_Exec, PinNames::CompletedPin);
	if (ensure(CompletedPin))
	{
		CompletedPin->PinFriendlyName = LOCTEXT( "CompletedPin_FriendlyName", "Completed");
	}

	if (CurrentInputType.PinCategory == NAME_None)
	{
		CurrentInputType = RealWildcardType;
	}
	else if (CurrentInputType.PinCategory != UEdGraphSchema_K2::PC_Wildcard)
	{
		ArrayPin->PinType = CurrentInputType;
		ElementPin->PinType = CurrentInputType;
		ElementPin->PinType.ContainerType = EPinContainerType::None;
	}

	if (AdvancedPinDisplay == ENodeAdvancedPins::NoPins)
	{
		AdvancedPinDisplay = ENodeAdvancedPins::Hidden;
	}
}

void UK2Node_InternalIterate::ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);

	if (CheckForErrors(CompilerContext))
	{
		BreakAllNodeLinks();
		return;
	}

	const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();
	
	UEdGraphPin* ExecPin = GetExecPin();
	UEdGraphPin* ArrayPin = GetArrayPin();
	UEdGraphPin* ForEachPin = GetForEachPin();
	UEdGraphPin* ArrayElementPin = GetElementPin();
	UEdGraphPin* ArrayIndexPin = GetArrayIndexPin();
	UEdGraphPin* CompletedPin = GetCompletedPin();

	// Create a local temporary variable
	UK2Node_TemporaryVariable* TempVar = CompilerContext.SpawnIntermediateNode<UK2Node_TemporaryVariable>(this, SourceGraph);
	TempVar->VariableType.PinCategory = UEdGraphSchema_K2::PC_Int;
	TempVar->AllocateDefaultPins();

	UEdGraphPin* TempVar_Pin = TempVar->GetVariablePin();
	CompilerContext.MovePinLinksToIntermediate( *ArrayIndexPin, *TempVar_Pin);

	// Default value to 0
	UK2Node_AssignmentStatement* Init_TempVar = CompilerContext.SpawnIntermediateNode<UK2Node_AssignmentStatement>(this, SourceGraph);
	Init_TempVar->AllocateDefaultPins();

	UEdGraphPin* Init_Exec = Init_TempVar->GetExecPin();
	UEdGraphPin* Init_Variable = Init_TempVar->GetVariablePin();
	UEdGraphPin* Init_Value = Init_TempVar->GetValuePin();
	UEdGraphPin* Init_Then = Init_TempVar->GetThenPin();

	CompilerContext.MovePinLinksToIntermediate(*ExecPin, *Init_Exec);
	Schema->TryCreateConnection(Init_Variable, TempVar_Pin);
	Init_Value->DefaultValue = TEXT("0"); // <-- Nice, initializing to 0 by using a string

	// Create a loop condition branch
	UK2Node_IfThenElse* BranchCond = CompilerContext.SpawnIntermediateNode<UK2Node_IfThenElse>(this, SourceGraph);
	BranchCond->AllocateDefaultPins();

	UEdGraphPin* Branch_Exec = BranchCond->GetExecPin();
	UEdGraphPin* Branch_Input = BranchCond->GetConditionPin();
	UEdGraphPin* Branch_Then = BranchCond->GetThenPin();
	UEdGraphPin* Branch_Else = BranchCond->GetElsePin();

	Init_Then->MakeLinkTo(Branch_Exec);
	CompilerContext.MovePinLinksToIntermediate(*CompletedPin, *Branch_Else);

	// Create the linker node to the loop condition branch
	UK2Node_CallFunction* LessThanFunc = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	LessThanFunc->FunctionReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Less_IntInt), UKismetMathLibrary::StaticClass());
	LessThanFunc->AllocateDefaultPins();

	UEdGraphPin* Compare_A = LessThanFunc->FindPinChecked(TEXT("A"));
	UEdGraphPin* Compare_B = LessThanFunc->FindPinChecked(TEXT("B"));
	UEdGraphPin* Compare_Return = LessThanFunc->GetReturnValuePin();

	Branch_Input->MakeLinkTo(Compare_Return);
	TempVar_Pin->MakeLinkTo(Compare_A);

	// Create another linker node to get the length of the array
	UK2Node_CallFunction* ArrayLenFunc = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	ArrayLenFunc->FunctionReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(UKismetArrayLibrary, Array_Length), UKismetArrayLibrary::StaticClass());
	ArrayLenFunc->AllocateDefaultPins();

	UEdGraphPin* ArrayLength_Array = ArrayLenFunc->FindPinChecked(TEXT("TargetArray"));
	UEdGraphPin* ArrayLength_Return = ArrayLenFunc->GetReturnValuePin();
	
	ArrayLength_Array->PinType = ArrayPin->PinType;

	// And connect
	Compare_B->MakeLinkTo(ArrayLength_Return);
	CompilerContext.CopyPinLinksToIntermediate(*ArrayPin,*ArrayLength_Array);

	// Incrementer node
	UK2Node_ExecutionSequence* SequenceFunc = CompilerContext.SpawnIntermediateNode<UK2Node_ExecutionSequence>(this, SourceGraph);
	SequenceFunc->AllocateDefaultPins();

	UEdGraphPin* Sequence_Exec = SequenceFunc->GetExecPin();
	UEdGraphPin* Sequence_One = SequenceFunc->GetThenPinGivenIndex(0);
	UEdGraphPin* Sequence_Two = SequenceFunc->GetThenPinGivenIndex(1);

	Branch_Then->MakeLinkTo(Sequence_Exec);
	CompilerContext.MovePinLinksToIntermediate(*ForEachPin, *Sequence_One);

	UK2Node_CallFunction* GetArrayElemFunc = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	GetArrayElemFunc->FunctionReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(UKismetArrayLibrary, Array_Get), UKismetArrayLibrary::StaticClass());
	GetArrayElemFunc->AllocateDefaultPins();

	UEdGraphPin* GetElement_Array = GetArrayElemFunc->FindPinChecked(TEXT("TargetArray"));
	UEdGraphPin* GetElement_Index = GetArrayElemFunc->FindPinChecked(TEXT("Index"));
	UEdGraphPin* GetElement_Return = GetArrayElemFunc->FindPinChecked(TEXT("Item"));

	// Connect here too
	GetElement_Array->PinType = ArrayPin->PinType;
	GetElement_Return->PinType = ArrayElementPin->PinType;

	CompilerContext.CopyPinLinksToIntermediate(*ArrayPin,*GetElement_Array);
	GetElement_Index->MakeLinkTo(TempVar_Pin);
	CompilerContext.MovePinLinksToIntermediate(*ArrayElementPin,*GetElement_Return);

	// Increment the loop counter
	UK2Node_AssignmentStatement* IncrVarFunc = CompilerContext.SpawnIntermediateNode<UK2Node_AssignmentStatement>(this, SourceGraph);
	IncrVarFunc->AllocateDefaultPins();

	UEdGraphPin* Inc_Exec = IncrVarFunc->GetExecPin();
	UEdGraphPin* Inc_Variable = IncrVarFunc->GetVariablePin();
	UEdGraphPin* Inc_Value = IncrVarFunc->GetValuePin();
	UEdGraphPin* Inc_Then = IncrVarFunc->GetThenPin();

	// And connect again.
	Sequence_Two->MakeLinkTo(Inc_Exec);
	Branch_Exec->MakeLinkTo(Inc_Then);
	Schema->TryCreateConnection(TempVar_Pin, Inc_Variable);

	UK2Node_CallFunction* AddOneFunc = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	AddOneFunc->FunctionReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, Add_IntInt), UKismetMathLibrary::StaticClass());
	AddOneFunc->AllocateDefaultPins();

	UEdGraphPin* Add_A = AddOneFunc->FindPinChecked(TEXT("A"));
	UEdGraphPin* Add_B = AddOneFunc->FindPinChecked(TEXT("B"));
	UEdGraphPin* Add_Return = AddOneFunc->GetReturnValuePin();

	// Guess what? Connect again.
	TempVar_Pin->MakeLinkTo(Add_A);
	Add_B->DefaultValue = TEXT("1"); // <-- D: again initializing an integer using a char
	Add_Return->MakeLinkTo(Inc_Value);


	// Break login nodes
	UEdGraphPin* BreakPin = GetBreakPin();

	UK2Node_AssignmentStatement* SetVarFunc = CompilerContext.SpawnIntermediateNode<UK2Node_AssignmentStatement>(this, SourceGraph);
	SetVarFunc->AllocateDefaultPins();

	UEdGraphPin* Set_Exec = SetVarFunc->GetExecPin();
	UEdGraphPin* Set_Variable = SetVarFunc->GetVariablePin();
	UEdGraphPin* Set_Value = SetVarFunc->GetValuePin();

	CompilerContext.MovePinLinksToIntermediate(*BreakPin, *Set_Exec);
	Schema->TryCreateConnection(TempVar_Pin,Set_Variable);

	UK2Node_CallFunction* GetArrLastIndexFunc = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this,SourceGraph);
	GetArrLastIndexFunc->FunctionReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(UKismetArrayLibrary, Array_LastIndex), UKismetArrayLibrary::StaticClass());
	GetArrLastIndexFunc->AllocateDefaultPins();

	UEdGraphPin* GetIndex_Array = GetArrLastIndexFunc->FindPinChecked(TEXT("TargetArray"));
	UEdGraphPin* GetIndex_Return = GetArrLastIndexFunc->GetReturnValuePin();

	// Troll user by pretending to pretend that we would connect the nodes :P
	GetIndex_Array->PinType = ArrayPin->PinType;
	CompilerContext.CopyPinLinksToIntermediate(*ArrayPin,*GetIndex_Array);

	GetIndex_Return->MakeLinkTo(Set_Value);

	// Finally done!
	BreakAllNodeLinks();
}

FText UK2Node_InternalIterate::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT( "NodeTitle", "Internal Iterate" );
}

FText UK2Node_InternalIterate::GetTooltipText() const
{
	return Super::GetTooltipText();
}

FSlateIcon UK2Node_InternalIterate::GetIconAndTint(FLinearColor& OutColor) const
{
	static const FSlateIcon Icon = FSlateIcon(FAppStyle::GetAppStyleSetName(), "GraphEditor.Macro.ForEach_16x");
	return Icon;
}

void UK2Node_InternalIterate::PinConnectionListChanged(UEdGraphPin* Pin)
{
	Super::PinConnectionListChanged(Pin);

	if (Pin == nullptr)
	{
		return;
	};

	if (Pin->PinName == PinNames::ArrayPin)
	{
		if (Pin->LinkedTo.Num() > 0)
		{
			CurrentInputType = Pin->LinkedTo[ 0 ]->PinType;	
		}
		else
		{
			CurrentInputType = RealWildcardType;
		}

		Pin->PinType = CurrentInputType;

		UEdGraphPin* ElementPin = GetElementPin();
		ElementPin->PinType = CurrentInputType;
		ElementPin->PinType.ContainerType = EPinContainerType::None;
	}
}

void UK2Node_InternalIterate::PostPasteNode()
{
	Super::PostPasteNode();

	CurrentInputType.PinCategory = NAME_None;
	if (UEdGraphPin* ArrayPin = GetArrayPin())
	{
		if ((CurrentInputType.PinCategory == NAME_None) &&
			ArrayPin->LinkedTo.Num())
		{
			PinConnectionListChanged(ArrayPin);
		}
	}
}

bool UK2Node_InternalIterate::CheckForErrors(const FKismetCompilerContext& CompilerContext)
{
	if (GetArrayPin()->LinkedTo.Num() == 0)
	{
		CompilerContext.MessageLog.Error(
			*LOCTEXT( "NoArrayEntry", "Internal Iterate node @@ requires an array input.").ToString(),
			this);
		return true;
	}

	return false;
}
