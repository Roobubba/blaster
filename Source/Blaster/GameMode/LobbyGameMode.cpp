// Fill out your copyright notice in the Description page of Project Settings.

#include "LobbyGameMode.h"
#include "GameFramework/GameStateBase.h"
#include "Blaster/PlayerController/LobbyPlayerController.h"
#include "GameFramework/PlayerState.h"
#include "MultiplayerSessionsSubsystem.h"

void ALobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);

    if (NewPlayer->IsLocalController())
    {
        UGameInstance* GameInstance = GetGameInstance();
        if (GameInstance)
        {
            UMultiplayerSessionsSubsystem* Subsystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();
            check(Subsystem);

            MaxNumberOfPlayers = Subsystem->GetDesiredNumPublicConnections();
            MatchType = Subsystem->GetDesiredMatchType();
        }
    }

    int32 NumberOfPlayers = GameState.Get()->PlayerArray.Num();

    LobbyPlayerNames.Empty();

    for (APlayerState* LobbyPlayerState : GameState.Get()->PlayerArray)
    {
        FString NewName = LobbyPlayerState->GetPlayerName();
        LobbyPlayerNames.Add(NewName);
    }

    ALobbyPlayerController* NewLobbyPlayerController = Cast<ALobbyPlayerController>(NewPlayer);
    if (NewLobbyPlayerController)
    {
        UE_LOG(LogTemp, Warning, TEXT("Sending MulticastSetLobbyDetails"));
        NewLobbyPlayerController->MulticastSetLobbyDetails(MaxNumberOfPlayers, MatchType);

        SendPlayerListToControllers();
    }

    if (NumberOfPlayers >= FMath::Max(2, (MaxNumberOfPlayers / 2)))
    {
        UWorld* World = GetWorld();
        if (World)
        {
            bUseSeamlessTravel = true;

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

void ALobbyGameMode::Logout(AController* Exiting)
{
    LobbyPlayerNames.Empty();

    for (APlayerState* LobbyPlayerState : GameState.Get()->PlayerArray)
    {
        FString NewName = LobbyPlayerState->GetPlayerName();
        LobbyPlayerNames.Add(NewName);
    }

    SendPlayerListToControllers();

    Super::Logout(Exiting);
}

void ALobbyGameMode::SendPlayerListToControllers()
{
    for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
    {
        ALobbyPlayerController* LobbyPlayerController = Cast<ALobbyPlayerController>(*Iterator);
        if (LobbyPlayerController)
        {
            LobbyPlayerController->MulticastSetPlayerList(LobbyPlayerNames);
        }
    }
}
