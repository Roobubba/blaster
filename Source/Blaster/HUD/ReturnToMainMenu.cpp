// Fill out your copyright notice in the Description page of Project Settings.

#include "ReturnToMainMenu.h"
#include "GameFramework/PlayerController.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Components/Button.h"
#include "MultiplayerSessionsSubsystem.h"
#include "GameFramework/GameModeBase.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Components/Slider.h"
#include "Components/CheckBox.h"
#include "GameFramework/InputSettings.h"
#include "Blaster/GameMode/BlasterGameMode.h"

void UReturnToMainMenu::MenuSetup()
{
    UWorld* World = GetWorld();
    if (World)
    {
        APlayerController* PlayerController = World->GetFirstPlayerController();
        if (PlayerController)
        {
            ABlasterPlayerController* BlasterPlayerController = Cast<ABlasterPlayerController>(PlayerController);      
    
            if (BlasterPlayerController == nullptr || BlasterPlayerController->GetMatchState() != MatchState::InProgress)
            {
                return;
            }
            
            if (ReturnButton)
            {
                if (!ReturnButton->OnClicked.IsBound())
                {
                    ReturnButton->OnClicked.AddDynamic(this, &UReturnToMainMenu::ReturnButtonClicked);
                }
                else
                {
                    return;
                }
            }

            AddToViewport();
            SetVisibility(ESlateVisibility::Visible);
            bIsFocusable = true;

            FInputModeGameAndUI InputModeData;
            InputModeData.SetWidgetToFocus(TakeWidget());
            PlayerController->SetInputMode(InputModeData);
            PlayerController->SetShowMouseCursor(true);

            UGameInstance* GameInstance = GetGameInstance();
            if (GameInstance)
            {
                MultiplayerSessionsSubsystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();

                if (MultiplayerSessionsSubsystem)
                {
                    MultiplayerSessionsSubsystem->MultiplayerOnDestroySessionComplete.AddDynamic(this, &UReturnToMainMenu::OnDestroySession);
                }
            }  
            
            BlasterPlayerController->LoadInputSettings();

            FInputAxisProperties MouseYProperties;
            FKey YAxisKey = FKey(FName("MouseY"));
            if (!BlasterPlayerController->PlayerInput->GetAxisProperties(YAxisKey, MouseYProperties))
            {
                return;
            }

            float Sensitivity = MouseYProperties.Sensitivity;
            bool bIsInverted = MouseYProperties.bInvert;
        
            if (SensitivitySlider && !SensitivitySlider->OnValueChanged.IsBound())
            {
                SensitivitySlider->SetValue(Sensitivity);
                SensitivitySlider->OnValueChanged.AddDynamic(this, &UReturnToMainMenu::SensitivitySliderChanged);
            }

            if (InvertMouseCheckbox && !InvertMouseCheckbox->OnCheckStateChanged.IsBound())
            {
                InvertMouseCheckbox->SetCheckedState(bIsInverted ? ECheckBoxState::Checked : ECheckBoxState::Unchecked);
                InvertMouseCheckbox->OnCheckStateChanged.AddDynamic(this, &UReturnToMainMenu::InvertMouseCheckboxStateChanged);
            }
        }
    }
}

void UReturnToMainMenu::SensitivitySliderChanged(float Value)
{
    UWorld* World = GetWorld();
    if (World)
    {
        APlayerController* PlayerController = World->GetFirstPlayerController();
        if (PlayerController)
        {
            ABlasterPlayerController* BlasterPlayerController = Cast<ABlasterPlayerController>(PlayerController);      
    
            if (BlasterPlayerController == nullptr)
            {
                return;
            }
            
            Value = FMath::Clamp(Value, 0.01f, 1.f); //Should get from slider or set slider to these instead of using magic numbers. Lazy toad.

            BlasterPlayerController->PlayerInput->SetMouseSensitivity(Value, Value);

            BlasterPlayerController->SaveInputSettings();
        }
    }
}

void UReturnToMainMenu::InvertMouseCheckboxStateChanged(bool bIsChecked)
{
    UWorld* World = GetWorld();
    if (World)
    {
        APlayerController* PlayerController = World->GetFirstPlayerController();
        if (PlayerController)
        {
            ABlasterPlayerController* BlasterPlayerController = Cast<ABlasterPlayerController>(PlayerController);      
    
            if (BlasterPlayerController == nullptr)
            {
                return;
            }

            FInputAxisProperties MouseYProperties;
            FKey YAxisKey = FKey(FName("MouseY"));
            if (BlasterPlayerController->PlayerInput->GetAxisProperties(YAxisKey, MouseYProperties))
            {
                MouseYProperties.bInvert = bIsChecked;
                BlasterPlayerController->PlayerInput->SetAxisProperties(YAxisKey, MouseYProperties);
                BlasterPlayerController->SaveInputSettings();
            }
        }
    }
}

void UReturnToMainMenu::OnDestroySession(bool bWasSuccessful)
{
    if (!bWasSuccessful)
    {
        ReturnButton->SetIsEnabled(true);
        return;
    }

    MenuTearDown();
    
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

void UReturnToMainMenu::MenuTearDown()
{
    if (!ReturnButton || !ReturnButton->GetIsEnabled())
    {
        return;
    }
    
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

            ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(PlayerController->GetPawn());
            if (BlasterCharacter && BlasterCharacter->OnLeftGame.IsBound())
            {
                BlasterCharacter->OnLeftGame.RemoveDynamic(this, &UReturnToMainMenu::OnPlayerLeftGame);
            }
        }
    }

    if (ReturnButton && ReturnButton->OnClicked.IsBound())
    {
        ReturnButton->OnClicked.RemoveDynamic(this, &UReturnToMainMenu::ReturnButtonClicked);
    }

    if (SensitivitySlider && SensitivitySlider->OnValueChanged.IsBound())
    {
        SensitivitySlider->OnValueChanged.RemoveDynamic(this, &UReturnToMainMenu::SensitivitySliderChanged);
    }
    
    if (InvertMouseCheckbox && InvertMouseCheckbox->OnCheckStateChanged.IsBound())
    {
        InvertMouseCheckbox->OnCheckStateChanged.RemoveDynamic(this, &UReturnToMainMenu::InvertMouseCheckboxStateChanged);
    }
}

void UReturnToMainMenu::ReturnButtonClicked()
{
    ReturnButton->SetIsEnabled(false);

    UWorld* World = GetWorld();
    if (World)
    {
        APlayerController* FirstPlayerController = World->GetFirstPlayerController();
        if (FirstPlayerController)
        {
            ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(FirstPlayerController->GetPawn());
            if (BlasterCharacter)
            {
                BlasterCharacter->ServerLeaveGame();
                if (!BlasterCharacter->OnLeftGame.IsBound())
                {
                    BlasterCharacter->OnLeftGame.AddDynamic(this, &UReturnToMainMenu::OnPlayerLeftGame);
                }
            }
            else
            {
                ReturnButton->SetIsEnabled(true);
            }
        }
    }

}

void UReturnToMainMenu::OnPlayerLeftGame()
{
    if (MultiplayerSessionsSubsystem)
    {
        MultiplayerSessionsSubsystem->DestroySession();
    }

    MenuTearDown();
}