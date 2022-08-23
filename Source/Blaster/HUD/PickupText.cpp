// Fill out your copyright notice in the Description page of Project Settings.

#include "PickupText.h"
#include "Components/TextBlock.h"
#include "BlasterHUD.h"
#include "TimerManager.h"

void UPickupText::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    AnnouncementTimeline.TickTimeline(InDeltaTime);
    RemainingTime = FMath::Max(0.f, RemainingTime - InDeltaTime);
}

void UPickupText::SetPickupText(FString PickupTextString, float DisplayTime, FColor Colour, ABlasterHUD* ParentBlasterHUD)
{
    AnnouncementTextTrack.BindDynamic(this, &UPickupText::UpdateAnnouncementTextAlpha);
    
    if (AnnouncementTextCurve && DisplayTime > 0.f)
    {
        RemainingTime = DisplayTime;
        AnnouncementTimeline.AddInterpFloat(AnnouncementTextCurve, AnnouncementTextTrack);
        float NewRate = AnnouncementTimeline.GetTimelineLength() / DisplayTime;

        AnnouncementTimeline.SetPlayRate(NewRate);   
            
        if (PickupText)
        {
            PickupText->SetText(FText::FromString(PickupTextString));
            PickupText->SetColorAndOpacity(FSlateColor(Colour));
            PickupText->SetRenderOpacity(0.f);
            PickupText->SetVisibility(ESlateVisibility::Visible);
        }

        AnnouncementTimeline.PlayFromStart();

        BlasterHUD = ParentBlasterHUD;

        UWorld* World = GetWorld();
        World->GetTimerManager().SetTimer(
            AnnouncementTextTimerHandle,
            this,
            &UPickupText::PickupTextTimerFinished,
            DisplayTime
        );
    }
}

void UPickupText::UpdateAnnouncementTextAlpha(float AnnouncementTextAlpha)
{
    if (PickupText)
    {
        PickupText->SetRenderOpacity(AnnouncementTextAlpha);
    }
}

void UPickupText::PickupTextTimerFinished()
{
    if (BlasterHUD)
    {
        BlasterHUD->PickupTextTimerFinished(this);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Could not find BlasterHUD, calling RemoveFromParent directly"));
        RemoveFromParent();
    }
}