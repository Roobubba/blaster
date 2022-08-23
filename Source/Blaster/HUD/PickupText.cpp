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

void UPickupText::SetPickupText(FString PickupTextString, float DisplayTime, ABlasterHUD* ParentBlasterHUD)
{
    AnnouncementTextTrack.BindDynamic(this, &UPickupText::UpdateAnnouncementTextAlpha);
    UE_LOG(LogTemp, Display, TEXT("SetPickupText called on new UPickupText: %s, DisplayTime =  %f"), *PickupTextString, DisplayTime);
    
    if (AnnouncementTextCurve && DisplayTime > 0.f)
    {
        RemainingTime = DisplayTime;
        AnnouncementTimeline.AddInterpFloat(AnnouncementTextCurve, AnnouncementTextTrack);
        UE_LOG(LogTemp, Display, TEXT("(AnnouncementTextCurve && DisplayTime > 0.f)"));
        float NewRate = AnnouncementTimeline.GetTimelineLength() / DisplayTime;
        UE_LOG(LogTemp, Display, TEXT("NewRate =  %f"), NewRate);

        AnnouncementTimeline.SetPlayRate(NewRate);   
            
        if (PickupText)
        {
            UE_LOG(LogTemp, Display, TEXT("(PickupText)"));
            PickupText->SetText(FText::FromString(PickupTextString));
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
            DisplayTime //CurveTime
        );
    }
}

void UPickupText::UpdateAnnouncementTextAlpha(float AnnouncementTextAlpha)
{
    if (PickupText)
    {
        UE_LOG(LogTemp, Display, TEXT("AnnouncementTextAlpha = %f"), AnnouncementTextAlpha);
        PickupText->SetRenderOpacity(AnnouncementTextAlpha);
    }
}

void UPickupText::PickupTextTimerFinished()
{
    UE_LOG(LogTemp, Display, TEXT("PickupTextTimerFinished()"));
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