// Copyright © 2025 MajorT. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "K2Node.h"
#include "K2Node_InternalIterate.generated.h"


UCLASS()
class UK2Node_InternalIterate : public UK2Node
{
	GENERATED_BODY()

public:
	// Pin Accessors
	UEdGraphPin* GetArrayPin() const;
	UEdGraphPin* GetBreakPin() const;

	UEdGraphPin* GetForEachPin() const;
	UEdGraphPin* GetElementPin() const;
	UEdGraphPin* GetArrayIndexPin() const;
	UEdGraphPin* GetCompletedPin() const;

	// K2Node API
	virtual bool IsNodeSafeToIgnore( ) const override { return true; }
	virtual void GetMenuActions( FBlueprintActionDatabaseRegistrar& ActionRegistrar ) const override;
	virtual FText GetMenuCategory( ) const override;

	// EdGraphNode API
	virtual void AllocateDefaultPins( ) override;
	virtual void ExpandNode( FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph ) override;
	virtual FText GetNodeTitle( ENodeTitleType::Type TitleType ) const override;
	virtual FText GetTooltipText( ) const override;
	virtual FSlateIcon GetIconAndTint( FLinearColor& OutColor ) const override;
	virtual void PinConnectionListChanged( UEdGraphPin* Pin ) override;
	virtual void PostPasteNode( ) override;

private:
	// Determine if there is any configuration options that shouldn't be allowed
	bool CheckForErrors( const FKismetCompilerContext& CompilerContext );

	UPROPERTY()
	FEdGraphPinType RealWildcardType;

	UPROPERTY()
	FEdGraphPinType CurrentInputType;
};
