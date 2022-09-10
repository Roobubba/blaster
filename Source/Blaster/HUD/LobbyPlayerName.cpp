// Fill out your copyright notice in the Description page of Project Settings.


#include "LobbyPlayerName.h"
#include "Components/TextBlock.h"

void ULobbyPlayerName::SetPlayerNameText(const FString& PlayerName)
{
    if (PlayerNameText)
    {
        PlayerNameText->SetText(FText::FromString(PlayerName));
    }
}