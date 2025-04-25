// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class NativeForEachMap : ModuleRules
{
	public NativeForEachMap(ReadOnlyTargetRules Target) : base(Target)
	{
		PublicDependencyModuleNames.AddRange(new []
		{ 
			"Core", 
			"BlueprintGraph", 
			"GraphEditor"
		});

		PrivateDependencyModuleNames.AddRange(new []
		{ 
			"CoreUObject", 
			"Engine", 
			"Slate", 
			"SlateCore", 
			"KismetCompiler", 
			"UnrealEd"
		});
	}
}
