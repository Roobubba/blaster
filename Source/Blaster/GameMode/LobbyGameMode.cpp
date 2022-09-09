// Fill out your copyright notice in the Description page of Project Settings.

#include "LobbyGameMode.h"
#include "GameFramework/GameStateBase.h"
#include "Blaster/PlayerController/LobbyPlayerController.h"
#include "MultiplayerSessionsSubsystem.h"

void ALobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);

    int32 NumberOfPlayers = GameState.Get()->PlayerArray.Num();

    UGameInstance* GameInstance = GetGameInstance();
    if (GameInstance)
    {
        UMultiplayerSessionsSubsystem* Subsystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();
        check(Subsystem);

        for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
        {
            ALobbyPlayerController* LobbyPlayerController = Cast<ALobbyPlayerController>(*Iterator);
            if (LobbyPlayerController)
            {
                LobbyPlayerController->SetLobbyDetails(Subsystem->GetDesiredNumPublicConnections(), Subsystem->GetDesiredMatchType());
                LobbyPlayerController->AddPlayer(NewPlayer);
            }
        }

        if (NumberOfPlayers >= FMath::Max(2, (Subsystem->GetDesiredNumPublicConnections() / 2)))
        {
            UWorld* World = GetWorld();
            if (World)
            {
                bUseSeamlessTravel = true;

                FString MatchType = Subsystem->GetDesiredMatchType();

                if (MatchType == "Teams")
                {
                    World->ServerTravel(FString("/Game/Maps/TeamsMap?listen"));
                }
                else if (MatchType == "CaptureTheFlagon")
                {
                    World->ServerTravel(FString("/Game/Maps/CaptureTheFlagonMap?listen"));
                }
                else
                {
                    World->ServerTravel(FString("/Game/Maps/BlasterMap?listen"));
                }
            }
        }
    }
}

void ALobbyGameMode::Logout(AController* Exiting)
{
    APlayerController* ExitingPlayerController = Cast<APlayerController>(Exiting);
    if (ExitingPlayerController)
    {
        for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
        {
            ALobbyPlayerController* LobbyPlayerController = Cast<ALobbyPlayerController>(*Iterator);
            if (LobbyPlayerController)
            {
                LobbyPlayerController->RemovePlayer(ExitingPlayerController);
            }
        }
    }
}