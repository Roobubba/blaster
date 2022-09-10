// Fill out your copyright notice in the Description page of Project Settings.

#include "BlasterHUD.h"
#include "GameFramework/PlayerController.h"
#include "CharacterOverlay.h"
#include "Components/TextBlock.h"
#include "TimerManager.h"
#include "Announcement.h"
#include "EliminationAnnouncement.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/HorizontalBox.h"
#include "Components/CanvasPanelSlot.h"
#include "ChatMessage.h"
#include "ChatInput.h"
#include "Components/EditableText.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "PickupText.h"

void ABlasterHUD::BeginPlay()
{
    Super::BeginPlay();

    APlayerController* PlayerController = GetOwningPlayerController();
    if (PlayerController)
    {
        FInputModeGameOnly InputModeData;
        PlayerController->SetInputMode(InputModeData);
        PlayerController->SetShowMouseCursor(false);
    }

    AddCharacterOverlay();
    AddAnnouncement();
    AddChatInput();
}

void ABlasterHUD::AddCharacterOverlay()
{
    APlayerController* PlayerController = GetOwningPlayerController();
    if (PlayerController && CharacterOverlayClass && !CharacterOverlay)
    {
        //if (CharacterOverlay)
        //{
        //    CharacterOverlay->RemoveFromViewport();
        //}

        CharacterOverlay = CreateWidget<UCharacterOverlay>(PlayerController, CharacterOverlayClass);
        CharacterOverlay->AddToViewport();
        CharacterOverlay->SetVisibility(ESlateVisibility::Hidden);
    }
}

void ABlasterHUD::AddAnnouncement()
{
    APlayerController* PlayerController = GetOwningPlayerController();

    if (PlayerController && AnnouncementClass && !Announcement)
    {
        Announcement = CreateWidget<UAnnouncement>(PlayerController, AnnouncementClass);
        Announcement->AddToViewport();
        Announcement->SetVisibility(ESlateVisibility::Hidden);
    }
}

void ABlasterHUD::AddChatInput()
{
    APlayerController* PlayerController = GetOwningPlayerController();

    if (PlayerController && ChatInputClass && !ChatInput)
    {
        ChatInput = CreateWidget<UChatInput>(PlayerController, ChatInputClass);
        ChatInput->AddToViewport();
        ChatInput->SetVisibility(ESlateVisibility::Visible);
        if (ChatInput->ChatInputEditableText)
        {
            ChatInput->ChatInputEditableText->SetVisibility(ESlateVisibility::Hidden);
            ChatInput->ChatInputEditableText->SetClearKeyboardFocusOnCommit(false);
        }
    }
}

void ABlasterHUD::DrawHUD()
{
    Super::DrawHUD();

    FVector2D ViewportSize;
    
    if (GEngine)
    {
        GEngine->GameViewport->GetViewportSize(ViewportSize);
        const FVector2D ViewportCentre(ViewportSize.X / 2.f, ViewportSize.Y / 2.f);

        float SpreadScaled = CrosshairSpreadMax * HUDPackage.CrosshairSpread;

        if (HUDPackage.CrosshairsCentre)
        {
            FVector2D Spread(0.f, 0.f);
            DrawCrosshair(HUDPackage.CrosshairsCentre, ViewportCentre, Spread, HUDPackage.CrosshairsColour);
        }
        if (HUDPackage.CrosshairsLeft)
        {
            FVector2D Spread(-SpreadScaled, 0.f);
            DrawCrosshair(HUDPackage.CrosshairsLeft, ViewportCentre, Spread, HUDPackage.CrosshairsColour);
        }
        if (HUDPackage.CrosshairsRight)
        {
            FVector2D Spread(SpreadScaled, 0.f);
            DrawCrosshair(HUDPackage.CrosshairsRight, ViewportCentre, Spread, HUDPackage.CrosshairsColour);
        }
        if (HUDPackage.CrosshairsTop)
        {
            FVector2D Spread(0.f, -SpreadScaled);
            DrawCrosshair(HUDPackage.CrosshairsTop, ViewportCentre, Spread, HUDPackage.CrosshairsColour);
        }
        if (HUDPackage.CrosshairsBottom)
        {
            FVector2D Spread(0.f, SpreadScaled);
            DrawCrosshair(HUDPackage.CrosshairsBottom, ViewportCentre, Spread, HUDPackage.CrosshairsColour);
        }
    }
}

void ABlasterHUD::DrawCrosshair(UTexture2D* Texture, FVector2D ViewportCentre, FVector2D Spread, FLinearColor CrosshairColour)
{
    const float TextureWidth = Texture->GetSizeX();
    const float TextureHeight = Texture->GetSizeY();
    const FVector2D TextureDrawPoint(
        ViewportCentre.X - (TextureWidth / 2.f) + Spread.X,
        ViewportCentre.Y - (TextureHeight / 2.f) + Spread.Y
    );

    DrawTexture(
        Texture,
        TextureDrawPoint.X,
        TextureDrawPoint.Y,
        TextureWidth,
        TextureHeight,
        0.f,
        0.f,
        1.f,
        1.f,
        CrosshairColour
    );
}

void ABlasterHUD::AddPickupText(FString PickupAnnouncement, float DisplayTime, FColor Colour)
{
    OwningPlayerController = OwningPlayerController == nullptr ? GetOwningPlayerController() : OwningPlayerController;

    if (OwningPlayerController && PickupTextClass)
    {
        UPickupText* PickupTextWidget = CreateWidget<UPickupText>(OwningPlayerController, PickupTextClass);
        if (PickupTextWidget)
        {
            PickupTextWidget->AddToViewport();
            PickupTextWidget->SetVisibility(ESlateVisibility::Visible);
            PickupTextWidget->SetRenderOpacity(1.f);
            PickupTextWidget->SetPickupText(PickupAnnouncement, DisplayTime, Colour, this);

            PickupTexts.Add(PickupTextWidget);

            PickupTexts.Sort([] (const UPickupText& Val1, const UPickupText& Val2)
            {
                return Val1.GetRemainingTime() > Val2.GetRemainingTime();

            });

            for (int32 i = 0; i < PickupTexts.Num(); i++)
            {
                UPickupText* PickupText = PickupTexts[i];
                if (PickupText && PickupText->PickupTextBox)
                {
                    UCanvasPanelSlot* CanvasSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(PickupText->PickupTextBox);
                    if (CanvasSlot)
                    {
                        FVector2D Position = FVector2D(36.f, PickupTextBaseYPosition + i * PickupTextBaseHeight);
                        CanvasSlot->SetPosition(Position);
                    }
                }
            }
           
        }
    }
}

void ABlasterHUD::PickupTextTimerFinished(UPickupText* MessageToRemove)
{
    if (PickupTexts.Contains(MessageToRemove))
    {
        PickupTexts.Remove(MessageToRemove);
    }

    MessageToRemove->RemoveFromParent();
}

void ABlasterHUD::RemoveAllPickupTexts()
{
    for (auto PickupText : PickupTexts)
    {
        PickupText->RemoveFromParent();
    }

    PickupTexts.Empty();
}

void ABlasterHUD::AddEliminationAnnouncement(FString Attacker, FString Victim)
{
    OwningPlayerController = OwningPlayerController == nullptr ? GetOwningPlayerController() : OwningPlayerController;

    if (OwningPlayerController && EliminationAnnouncementClass)
    {
        UEliminationAnnouncement* EliminationAnnouncementWidget = CreateWidget<UEliminationAnnouncement>(OwningPlayerController, EliminationAnnouncementClass);
        if (EliminationAnnouncementWidget)
        {
            EliminationAnnouncementWidget->SetEliminationAnnouncementText(Attacker, Victim);
            EliminationAnnouncementWidget->AddToViewport();

            for (UEliminationAnnouncement* Message : EliminationMessages)
            {
                if (Message && Message->AnnouncementBox)
                {
                    UCanvasPanelSlot* CanvasSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(Message->AnnouncementBox);
                    if (CanvasSlot)
                    {
                        FVector2D Position = CanvasSlot->GetPosition();
                        FVector2D NewPosition(Position.X, Position.Y + CanvasSlot->GetSize().Y);
                        CanvasSlot->SetPosition(NewPosition);
                    }
                }
            }

            EliminationMessages.Add(EliminationAnnouncementWidget);

            FTimerHandle EliminationMessageTimer;
            FTimerDelegate EliminationMessageDelegate;

            EliminationMessageDelegate.BindUFunction(this, FName("EliminationAnnouncementTimerFinished"), EliminationAnnouncementWidget);
            GetWorldTimerManager().SetTimer
            (
                EliminationMessageTimer,
                EliminationMessageDelegate,
                EliminationAnnouncementTime,
                false
            );
        }
    }
}

void ABlasterHUD::EliminationAnnouncementTimerFinished(UEliminationAnnouncement* MessageToRemove)
{
    if (MessageToRemove)
    {
        MessageToRemove->RemoveFromParent();
    }
}

//void ABlasterHUD::EnableChatInput()
//{
/*     if (!ChatInput || !ChatInput->ChatInputEditableText)
    {
        UE_LOG(LogTemp, Warning, TEXT("ChatInput or the ChatInputEditableText were nullptr"));
        AddChatInput();
        return;
    }

    if (ChatInput->ChatInputEditableText->GetVisibility() == ESlateVisibility::Visible)
    {
        UE_LOG(LogTemp, Warning, TEXT("Hiding Chat Input"));
        ChatInput->ChatInputEditableText->SetVisibility(ESlateVisibility::Hidden);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Showing Chat Input"));
        ChatInput->ChatInputEditableText->SetVisibility(ESlateVisibility::Visible);
        ChatInput->ChatInputEditableText->SetKeyboardFocus();

        if (!ChatInput->ChatInputEditableText->IsValidLowLevel() && !ChatInput->ChatInputEditableText->OnTextCommitted.IsBound())
        {
            UE_LOG(LogTemp, Warning, TEXT("Chat Input was not bound, binding to delegate"));
            ChatInput->ChatInputEditableText->OnTextCommitted.AddDynamic(this, &ABlasterHUD::ChatInputCommitted);
        }
    } */
//}

void ABlasterHUD::ChatInputCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
    UE_LOG(LogTemp, Warning, TEXT("ChatInputCommitted called: Text = %s"), *FString(Text.ToString()));
    if (ChatInput && ChatInput->ChatInputEditableText)
    {
        if (ChatInput->ChatInputEditableText->OnTextCommitted.IsBound())
        {
            ChatInput->ChatInputEditableText->OnTextCommitted.RemoveDynamic(this, &ABlasterHUD::ChatInputCommitted);
        }

        switch (CommitMethod)
        {
            case ETextCommit::OnEnter:
                OwningPlayerController = OwningPlayerController == nullptr ? GetOwningPlayerController() : OwningPlayerController;
                if (OwningPlayerController)
                {
                    OwningBlasterPlayerController = OwningBlasterPlayerController == nullptr ? Cast<ABlasterPlayerController>(OwningPlayerController) : OwningBlasterPlayerController;
                    if (OwningBlasterPlayerController)
                    {
                        FString Message = FString(Text.ToString());
                        UE_LOG(LogTemp, Display, TEXT("Message To Send = '%s'; Calling ServerBroadcastChatMessage()"), *Message);
                        OwningBlasterPlayerController->ServerBroadcastChatMessage(OwningPlayerController, Message);  
                    }
                }
                break;
            case ETextCommit::Default:
            case ETextCommit::OnCleared:
            case ETextCommit::OnUserMovedFocus:
            default:

                break;
        }
        
        ChatInput->ChatInputEditableText->SetText(FText());
        ChatInput->ChatInputEditableText->SetVisibility(ESlateVisibility::Hidden);
    }
}

void ABlasterHUD::AddChatMessage(const FString& Message)
{
    OwningPlayerController = OwningPlayerController == nullptr ? GetOwningPlayerController() : OwningPlayerController;

    if (OwningPlayerController && ChatMessageClass)
    {
        UChatMessage* ChatMessageWidget = CreateWidget<UChatMessage>(OwningPlayerController, ChatMessageClass);
        if (ChatMessageWidget)
        {
            ChatMessageWidget->SetChatMessageText(Message);
            ChatMessageWidget->AddToViewport();

            for (UChatMessage* ChatMessage : ChatMessages)
            {
                if (ChatMessage && ChatMessage->ChatMessageBox)
                {
                    UCanvasPanelSlot* CanvasSlot = UWidgetLayoutLibrary::SlotAsCanvasSlot(ChatMessage->ChatMessageBox);
                    if (CanvasSlot)
                    {
                        FVector2D Position = CanvasSlot->GetPosition();
                        FVector2D NewPosition(Position.X, Position.Y - CanvasSlot->GetSize().Y);
                        CanvasSlot->SetPosition(NewPosition);
                    }
                }
            }

            ChatMessages.Add(ChatMessageWidget);

            FTimerHandle ChatMessageTimer;
            FTimerDelegate ChatMessageDelegate;

            ChatMessageDelegate.BindUFunction(this, FName("ChatMessageTimerFinished"), ChatMessageWidget);
            GetWorldTimerManager().SetTimer
            (
                ChatMessageTimer,
                ChatMessageDelegate,
                ChatMessageTime,
                false
            );
        }
    }
}

void ABlasterHUD::ChatMessageTimerFinished(UChatMessage* MessageToRemove)
{
    if (MessageToRemove)
    {
        MessageToRemove->RemoveFromParent();
    }
}

void ABlasterHUD::ResetChatSystem()
{
    for (auto MessageToRemove : ChatMessages)
    {
        if (MessageToRemove)
        {
            MessageToRemove->RemoveFromParent();
        }
    }

    ChatMessages.Empty();
    if (ChatInput && ChatInput->ChatInputEditableText)
    {
        ChatInput->ChatInputEditableText->SetText(FText());
        ChatInput->ChatInputEditableText->SetVisibility(ESlateVisibility::Hidden);
    }
}