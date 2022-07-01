// Fill out your copyright notice in the Description page of Project Settings.

#include "BlasterHUD.h"
#include "GameFramework/PlayerController.h"
#include "CharacterOverlay.h"
#include "Components/TextBlock.h"
#include "TimerManager.h"

ABlasterHUD::ABlasterHUD()
{
    ElimTextTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("TimelineComponent"));
    ElimTextTrack.BindDynamic(this, &ABlasterHUD::UpdateElimTextAlpha);
}

void ABlasterHUD::BeginPlay()
{
    Super::BeginPlay();
}

void ABlasterHUD::AddCharacterOverlay()
{
    APlayerController* PlayerController = GetOwningPlayerController();
    if (PlayerController)
    {
        CharacterOverlay = CreateWidget<UCharacterOverlay>(PlayerController, CharacterOverlayClass);
        CharacterOverlay->AddToViewport();
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

void ABlasterHUD::ShowElimMessage()
{
    if (ElimTextTimeline && ElimTextCurve)
    {
        ElimTextTimeline->AddInterpFloat(ElimTextCurve, ElimTextTrack);
        ElimTextTimeline->PlayFromStart();
            
        if (CharacterOverlay && CharacterOverlay->ElimText)
        {
            //CharacterOverlay->ElimText->SetVisibility(ESlateVisibility::Visible);
            CharacterOverlay->ElimText->SetRenderOpacity(0.f);
        }

        float CurveTime = ElimTextTimeline->GetTimelineLength();
        GetWorldTimerManager().SetTimer(
            ElimTextTimerHandle,
            this,
            &ABlasterHUD::ElimTextTimerFinished,
            CurveTime
        );
    }
}

void ABlasterHUD::UpdateElimTextAlpha(float ElimTextAlpha)
{
    if (CharacterOverlay && CharacterOverlay->ElimText)
    {
        CharacterOverlay->ElimText->SetRenderOpacity(ElimTextAlpha);
    }
}

void ABlasterHUD::ElimTextTimerFinished()
{
    if (CharacterOverlay && CharacterOverlay->ElimText)
    {
        //CharacterOverlay->ElimText->SetVisibility(ESlateVisibility::Hidden);
        CharacterOverlay->ElimText->SetRenderOpacity(0.f);
    }
}