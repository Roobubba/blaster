// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "LobbyHUD.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API ALobbyHUD : public AHUD
{
	GENERATED_BODY()

public:

	void UpdatePlayerNameList();
	void UpdateLobbyDetails();
	virtual void Destroyed() override;
	
protected:

	virtual void BeginPlay() override;
	
	UFUNCTION()
	void OnDestroySession(bool bWasSuccessful);

private:

	UPROPERTY()
	class APlayerController* OwningPlayerController;

	UPROPERTY()
	class ALobbyPlayerController* OwningLobbyPlayerController;

	void AddLobbyOverlay();
	
	UFUNCTION()
	void QuitButtonPressed();
	
	UPROPERTY()
	class UMultiplayerSessionsSubsystem* MultiplayerSessionsSubsystem;

	UPROPERTY(EditAnywhere)
	TSubclassOf<class UUserWidget> LobbyOverlayClass;

	UPROPERTY()
	class ULobbyOverlay* LobbyOverlay = nullptr;

	UPROPERTY(EditAnywhere)
	TSubclassOf<UUserWidget> LobbyPlayerNameClass;

	UPROPERTY()
	TArray<class ULobbyPlayerName*> LobbyPlayerNames;
};
