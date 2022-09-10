// Fill out your copyright notice in the Description page of Project Settings.


#include "LobbyPlayerController.h"
#include "Blaster/HUD/LobbyHUD.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/GameState.h"

void ALobbyPlayerController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    PollInit();
}

void ALobbyPlayerController::PollInit()
{
    if (!LobbyHUD)
    {
        UpdateLobbyHUD();
    }
}

void ALobbyPlayerController::PostSeamlessTravel()
{
    Super::PostSeamlessTravel();
    
    if (LobbyHUD)
    {
        LobbyHUD->Destroy();
    }

    Destroy();
}

void ALobbyPlayerController::MulticastSetLobbyDetails_Implementation(const int32& DesiredMaxPlayerCount, const FString& DesiredMatchType)
{
    UE_LOG(LogTemp, Warning, TEXT("MulticastSetLobbyDetails called with inputs: %d, %s"), DesiredMaxPlayerCount, *DesiredMatchType);
    MaxPublicConnections = DesiredMaxPlayerCount;
    MatchType = DesiredMatchType;
    UpdateLobbyHUD();
}

void ALobbyPlayerController::MulticastSetPlayerList_Implementation(const TArray<FString>& LobbyPlayerNames)
{
    UE_LOG(LogTemp, Warning, TEXT("MulticastSetPlayerList called"));
    PlayerNames.Empty();
    PlayerNames = LobbyPlayerNames;
    UpdateLobbyHUD();
}

void ALobbyPlayerController::UpdateLobbyHUD()
{
    LobbyHUD = LobbyHUD == nullptr ? Cast<ALobbyHUD>(GetHUD()) : LobbyHUD;
    
    if (LobbyHUD && IsLocalController())
    {
        LobbyHUD->UpdatePlayerNameList();
        LobbyHUD->UpdateLobbyDetails();
    }
}