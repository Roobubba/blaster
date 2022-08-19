// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ChatInput.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API UChatInput : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(meta = (BindWidget))
	class UHorizontalBox* ChatInputBox;

	UPROPERTY(meta = (BindWidget))
	class UEditableText* ChatInputEditableText;
};
