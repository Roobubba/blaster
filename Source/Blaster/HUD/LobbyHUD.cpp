// Fill out your copyright notice in the Description page of Project Settings.


#include "LobbyHUD.h"
#include "LobbyOverlay.h"
#include "LobbyPlayerName.h"
#include "Components/Button.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "MultiplayerSessionsSubsystem.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "Blaster/PlayerController/LobbyPlayerController.h"

void ALobbyHUD::BeginPlay()
{
    Super::BeginPlay();
    AddLobbyOverlay();

    UGameInstance* GameInstance = GetGameInstance();
    if (GameInstance)
    {
        MultiplayerSessionsSubsystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();

        if (MultiplayerSessionsSubsystem)
        {
            MultiplayerSessionsSubsystem->MultiplayerOnDestroySessionComplete.AddDynamic(this, &ALobbyHUD::OnDestroySession);
        }
    }

    UpdatePlayerNameList();
}

void ALobbyHUD::Destroyed()
{
    if (LobbyOverlay)
    {
        LobbyOverlay->RemoveFromViewport();
    }

    Super::Destroyed();
}

void ALobbyHUD::AddLobbyOverlay()
{
    UE_LOG(LogTemp, Warning, TEXT("AddingLobbyOverlay"));
    OwningPlayerController = OwningPlayerController == nullptr ? GetOwningPlayerController() : OwningPlayerController;
    if (OwningPlayerController && LobbyOverlayClass && !LobbyOverlay)
    {
        LobbyOverlay = CreateWidget<ULobbyOverlay>(OwningPlayerController, LobbyOverlayClass);
        LobbyOverlay->AddToViewport();
        LobbyOverlay->SetVisibility(ESlateVisibility::Visible);
        LobbyOverlay->bIsFocusable = true;
        FInputModeUIOnly InputModeData;
        InputModeData.SetWidgetToFocus(LobbyOverlay->TakeWidget());
        OwningPlayerController->SetInputMode(InputModeData);
        OwningPlayerController->SetShowMouseCursor(true);

        if (LobbyOverlay->QuitButton && !LobbyOverlay->QuitButton->OnClicked.IsBound())
        {
            LobbyOverlay->QuitButton->OnClicked.AddDynamic(this, &ALobbyHUD::QuitButtonPressed);
        }
        
        UpdateLobbyDetails();
    }
}

void ALobbyHUD::QuitButtonPressed()
{
    if (MultiplayerSessionsSubsystem)
    {
        MultiplayerSessionsSubsystem->DestroySession();
    }
}

void ALobbyHUD::OnDestroySession(bool bWasSuccessful)
{
    if (!LobbyOverlay && !LobbyOverlay->QuitButton)
    {
        return;
    }

    if (!bWasSuccessful)
    {
        LobbyOverlay->QuitButton->SetIsEnabled(true);
        return;
    }

    LobbyOverlay->QuitButton->OnClicked.RemoveDynamic(this, &ALobbyHUD::QuitButtonPressed);

    UWorld* World = GetWorld();
    if (World)
    {
        AGameModeBase* GameMode = World->GetAuthGameMode<AGameModeBase>();
        if (GameMode)
        {
            GameMode->ReturnToMainMenuHost();
        }
        else
        {
            APlayerController* PlayerController = World->GetFirstPlayerController();
            if (PlayerController)
            {
                PlayerController->ClientReturnToMainMenuWithTextReason(FText());
            }
        }
    }
}

void ALobbyHUD::UpdatePlayerNameList()
{
    UE_LOG(LogTemp, Warning, TEXT("UpdatePlayerNameList called"));

    OwningPlayerController = OwningPlayerController == nullptr ? GetOwningPlayerController() : OwningPlayerController;
    if (!OwningPlayerController || !LobbyPlayerNameClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("OwningPlayerController or LobbyNamePlayerClass is null"));
        return;
    }

    if (!LobbyOverlay || !LobbyOverlay->PlayerNameScrollBox)
    {
        UE_LOG(LogTemp, Warning, TEXT("LobbyOverlay or LobbyOverlay->PlayerNameScrollBox is null"));
        return;
    }   

    OwningLobbyPlayerController = OwningLobbyPlayerController == nullptr ? Cast<ALobbyPlayerController>(OwningPlayerController) : OwningLobbyPlayerController;
    if (!OwningLobbyPlayerController)
    {
        UE_LOG(LogTemp, Warning, TEXT("OwningLobbyPlayerController is null"));
        return;
    }

    for (ULobbyPlayerName* LobbyPlayerName : LobbyPlayerNames)
    {
        LobbyPlayerName->RemoveFromViewport();
    }

    LobbyPlayerNames.Empty();

    TArray<FString> PlayerNames = OwningLobbyPlayerController->GetPlayerNames();

    UE_LOG(LogTemp, Warning, TEXT("PlayerNames.Num() = %d"), PlayerNames.Num());

    for (FString PlayerName : PlayerNames)
    {
        ULobbyPlayerName* LobbyPlayerNameWidget = CreateWidget<ULobbyPlayerName>(OwningPlayerController, LobbyPlayerNameClass);
        if (LobbyPlayerNameWidget)
        {
            UE_LOG(LogTemp, Warning, TEXT("Adding %s to the player names list in the HUD"), *PlayerName);
            LobbyPlayerNameWidget->SetPlayerNameText(PlayerName);
            LobbyPlayerNameWidget->AddToViewport();
            LobbyOverlay->PlayerNameScrollBox->AddChild(LobbyPlayerNameWidget);
            LobbyPlayerNames.Add(LobbyPlayerNameWidget);
        }
    }
}

void ALobbyHUD::UpdateLobbyDetails()
{
    UE_LOG(LogTemp, Warning, TEXT("UpdateLobbyDetails called"));
    OwningPlayerController = OwningPlayerController == nullptr ? GetOwningPlayerController() : OwningPlayerController;
    if (!OwningPlayerController)
    {
        return;
    }

    if (!LobbyOverlay)
    {
        UE_LOG(LogTemp, Warning, TEXT("LobbyOverlay is null"));
        return;
    }   

    OwningLobbyPlayerController = OwningLobbyPlayerController == nullptr ? Cast<ALobbyPlayerController>(OwningPlayerController) : OwningLobbyPlayerController;
    if (!OwningLobbyPlayerController)
    {
        UE_LOG(LogTemp, Warning, TEXT("OwningLobbyPlayerController is null"));
        return;
    }

    if (LobbyOverlay->MaxPlayersText && LobbyOverlay->RequiredPlayersText)
    {
        UE_LOG(LogTemp, Warning, TEXT("LobbyOverlay->MaxPlayersText and LobbyOverlay->RequiredPlayersText are both valid"));
        int32 MaxPlayersInt = OwningLobbyPlayerController->GetMaxPublicConnections();
        FString MaxPlayers = FString::FromInt(MaxPlayersInt);
        LobbyOverlay->MaxPlayersText->SetText(FText::FromString(MaxPlayers));
        FString RequiredPlayers = FString::FromInt(OwningLobbyPlayerController->GetRequiredPublicConnections());
        LobbyOverlay->RequiredPlayersText->SetText(FText::FromString(RequiredPlayers));
    }

    if (LobbyOverlay->SelectedGameModeText)
    {
        UE_LOG(LogTemp, Warning, TEXT("LobbyOverlay->SelectedGameModeText is valid"));
        //LobbyOverlay->SelectedGameModeText->SetText(FText::FromString("Wibble"));
        LobbyOverlay->SelectedGameModeText->SetText(FText::FromString(OwningLobbyPlayerController->GetMatchType()));
    }
}