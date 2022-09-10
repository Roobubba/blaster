// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LobbyOverlay.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API ULobbyOverlay : public UUserWidget
{
	GENERATED_BODY()

public:

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* RequiredPlayersText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* MaxPlayersText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* SelectedGameModeText;

	UPROPERTY(meta = (BindWidget))
	class UScrollBox* PlayerNameScrollBox;

	UPROPERTY(meta = (BindWidget))
	class UButton* QuitButton;

protected:
	virtual void OnLevelRemovedFromWorld(ULevel* InLevel, UWorld* InWorld) override;

};
