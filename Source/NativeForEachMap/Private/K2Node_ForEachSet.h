// Author: Tom Werner (MajorT), 2025

#pragma once

#include "CoreMinimal.h"
#include "K2Node.h"
#include "K2Node_ForEachSet.generated.h"

/** Even dumber node for a for-each loop over a set */
UCLASS(CollapseCategories)
class NATIVEFOREACHMAP_API UK2Node_ForEachSet : public UK2Node
{
	GENERATED_BODY()

public:
	UK2Node_ForEachSet();

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
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual void PinConnectionListChanged(UEdGraphPin* Pin) override;
	virtual bool ShouldShowNodeProperties() const override { return true; }
	virtual void PostPasteNode() override;
	//~ End UEdGraphNode Interface

	//~ Begin UObject Interface
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	//~ End UObject Interface

	/** Pin Accessors */
	[[nodiscard]] UEdGraphPin* GetInputSetPin() const;
	[[nodiscard]] UEdGraphPin* GetInputBreakPin() const;
	[[nodiscard]] UEdGraphPin* GetLoopBodyPin() const;
	[[nodiscard]] UEdGraphPin* GetValuePin() const;
	[[nodiscard]] UEdGraphPin* GetCompletePin() const;
	[[nodiscard]] UEdGraphPin* GetIndexPin() const;

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
	FEdGraphPinType CachedValueType;

	/** Whether we want to auto-assign pins (for example, when reopening the unreal editor) */
	UPROPERTY()
	bool bAutoAssignPins = false;

private:
	/** A user-editable hook for the display name of the value pin */
	UPROPERTY(EditDefaultsOnly, Category = ForEachSet)
	FString ValueName;

	/** A user-editable hook for the display name of the index pin */
	UPROPERTY(EditDefaultsOnly, Category = ForEachSet)
	FString IndexName;
};
