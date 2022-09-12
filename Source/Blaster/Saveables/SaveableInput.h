// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
//#include "UObject/NoExportTypes.h"
#include "GameFramework/PlayerInput.h"
#include "SaveableInput.generated.h"

struct FInputAxisConfigEntry;
struct FInputActionKeyMapping;
struct FInputAxisKeyMapping;


UCLASS(Config=Input)
class BLASTER_API USaveableInput : public UObject
{
	GENERATED_BODY()

public:

	/** This player's version of the Axis Properties */
	UPROPERTY(Config)
	TArray<struct FInputAxisConfigEntry> AxisConfig;

	/** This player's version of the Action Mappings */
	UPROPERTY(Config)
	TArray<struct FInputActionKeyMapping> ActionMappings;

	/** This player's version of Axis Mappings */
	UPROPERTY(Config)
	TArray<struct FInputAxisKeyMapping> AxisMappings;

	/** List of Axis Mappings that have been inverted */
	UPROPERTY(Config)
	TArray<FName> InvertedAxis;
};
