// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LobbyPlayerName.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API ULobbyPlayerName : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(meta = (BindWidget))
	class UHorizontalBox* PlayerNameBox;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* PlayerNameText;

	void SetPlayerNameText(const FString& PlayerName);
};
