// Fill out your copyright notice in the Description page of Project Settings.


#include "LobbyOverlay.h"

void ULobbyOverlay::OnLevelRemovedFromWorld(ULevel* InLevel, UWorld* InWorld)
{
    RemoveFromParent();
    
    UWorld* World = GetWorld();
    if (World)
    {
        APlayerController* PlayerController = World->GetFirstPlayerController();
        if (PlayerController)
        {
            FInputModeGameOnly InputModeData;
            PlayerController->SetInputMode(InputModeData);
            PlayerController->SetShowMouseCursor(false);
        }
    }

    Super::OnLevelRemovedFromWorld(InLevel, InWorld);
}