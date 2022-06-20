// Fill out your copyright notice in the Description page of Project Settings.


#include "OverheadWidget.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerState.h"

void UOverheadWidget::OnLevelRemovedFromWorld(ULevel* InLevel, UWorld* InWorld)
{
    RemoveFromParent();
    Super::OnLevelRemovedFromWorld(InLevel, InWorld);
}

void UOverheadWidget::SetDisplayText(FString TextToDisplay)
{
    if (DisplayText)
    {
        DisplayText->SetText(FText::FromString(TextToDisplay));
    }
}

void UOverheadWidget::ShowPlayerName(APawn* InPawn)
{
    //APlayerState* PlayerState = InPawn->GetPlayerState();
    //if (PlayerState)
    //{
    //    FString PlayerName = PlayerState->GetPlayerName();
    //    FString NameString = FString::Printf(TEXT("Name: %s"), *PlayerName);
    //    SetDisplayText(NameString);
    //}
    //ENetRole RemoteRole = InPawn->GetRemoteRole();
    //FString Role;
    //switch (RemoteRole)
    //{
    //    case ENetRole::ROLE_Authority:
    //        Role = FString("Authority");
    //        break;
    //    case ENetRole::ROLE_AutonomousProxy:
    //        Role = FString("AutonomousProxy");
    //        break;
    //    case ENetRole::ROLE_SimulatedProxy:
    //        Role = FString("SimulatedProxy");
    //        break;
    //    case ENetRole::ROLE_None:
    //    default:
    //        Role = FString("None");
    //        break;
    //}
//
    //FString RemoteRoleString = FString::Printf(TEXT("Remote Role: %s"), *Role);
    //SetDisplayText(RemoteRoleString);
    
}