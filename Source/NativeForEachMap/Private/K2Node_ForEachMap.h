// Copyright © 2025 MajorT. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "K2Node.h"
#include "K2Node_ForEachMap.generated.h"

/** Dumb node for a for-each loop over a map */
UCLASS()
class NATIVEFOREACHMAP_API UK2Node_ForEachMap : public UK2Node
{
	GENERATED_BODY()
	
public:
	UK2Node_ForEachMap();
	
	//~ Begin UK2Node Interface
	virtual bool IsNodeSafeToIgnore() const override { return true; }
	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	virtual FText GetMenuCategory() const override;
	//~ End UK2Node Interface

	//~ Begin UEdGraphNode Interface
	virtual void AllocateDefaultPins() override;
	virtual void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetTooltipText() const override;
	virtual FText GetKeywords() const override;
	virtual FSlateIcon GetIconAndTint(FLinearColor& OutColor) const override;
	virtual void PinConnectionListChanged(UEdGraphPin* Pin) override;
	virtual bool ShouldShowNodeProperties() const override { return true; }
	virtual void PostPasteNode() override;
	//~ End UEdGraphNode Interface

	/** Pin Accessors */
	UEdGraphPin* GetInputMapPin() const;
	UEdGraphPin* GetInputBreakPin() const;
	UEdGraphPin* GetLoopBodyPin() const;
	UEdGraphPin* GetKeyPin() const;
	UEdGraphPin* GetValuePin() const;
	UEdGraphPin* GetCompletePin() const;

protected:
	/** Performs a generalized CheckForErrors lookup. */
	virtual bool CheckForErrors(const FKismetCompilerContext& CompilerContext);

	/** Cached off types for the input pins */
	UPROPERTY()
	FEdGraphPinType CachedInputWildcardType;
	UPROPERTY()
	FEdGraphPinType CachedWildcardType;
	UPROPERTY()
	FEdGraphPinType CachedInputType;
	UPROPERTY()
	FEdGraphPinType CachedKeyType;
	UPROPERTY()
	FEdGraphPinType CachedValueType;

	/** Whether we want to auto-assign pins (for example, when reopening the unreal editor) */
	UPROPERTY()
	bool bAutoAssignPins = false;
};
