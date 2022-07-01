// Fill out your copyright notice in the Description page of Project Settings.

#include "BlasterPlayerController.h"
#include "Blaster/HUD/BlasterHUD.h"
#include "Blaster/HUD/CharacterOverlay.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/BlasterTypes/WeaponType.h"
#include "Net/UnrealNetwork.h"
#include "Blaster/GameMode/BlasterGameMode.h"
#include "Blaster/HUD/Announcement.h"
#include "Kismet/GameplayStatics.h"

void ABlasterPlayerController::BeginPlay()
{
    Super::BeginPlay();

    BlasterHUD = Cast<ABlasterHUD>(GetHUD());
    ServerGetMatchState();
}

void ABlasterPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABlasterPlayerController, MatchState);
}

void ABlasterPlayerController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    SetHUDTime();
    CheckTimeSync(DeltaTime);

    PollInit();
}

void ABlasterPlayerController::CheckTimeSync(float DeltaTime)
{
    TimeSinceLastSync += DeltaTime;
    if (IsLocalController())
    {
        if (TimeSinceLastSync > TimeSyncFrequency)
        {
            TimeSinceLastSync = 0.f;
            ServerRequestServerTime(GetWorld()->GetTimeSeconds());
        }
    }
}

void ABlasterPlayerController::ServerGetMatchState_Implementation()
{
    ABlasterGameMode* GameMode = Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this));
    if (GameMode)
    {
        WarmupTime = GameMode->WarmupTime;
        MatchTime = GameMode->MatchTime;
        LevelStartingTime = GameMode->LevelStartingTime;
        MatchState = GameMode->GetMatchState();
        ClientJoinMidgame(MatchState, WarmupTime, MatchTime, LevelStartingTime);
    }
}

void ABlasterPlayerController::ClientJoinMidgame_Implementation(FName StateOfMatch, float Warmup, float Match, float StartingTime)
{
    WarmupTime = Warmup;
    MatchTime = Match;
    LevelStartingTime = StartingTime;
    MatchState = StateOfMatch;
    OnMatchStateSet(MatchState);

    if (BlasterHUD && MatchState == MatchState::WaitingToStart)
    {
        BlasterHUD->AddAnnouncement();
    }
}

void ABlasterPlayerController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(InPawn);
    if (BlasterCharacter)
    {
        SetHUDHealth(BlasterCharacter->GetHealth(), BlasterCharacter->GetMaxHealth());
    }
}

void ABlasterPlayerController::SetHUDHealth(float Health, float MaxHealth)
{
    BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

    bool bHUDValid = BlasterHUD && 
        BlasterHUD->CharacterOverlay &&
        BlasterHUD->CharacterOverlay->HealthBar &&
        BlasterHUD->CharacterOverlay->HealthText;

    if (bHUDValid)
    {
        const float HealthPercent = Health / MaxHealth;
        BlasterHUD->CharacterOverlay->HealthBar->SetPercent(HealthPercent);
        
        FString HealthString = FString::Printf(TEXT("%d/%d"), FMath::CeilToInt(Health), FMath::CeilToInt(MaxHealth));
        BlasterHUD->CharacterOverlay->HealthText->SetText(FText::FromString(HealthString));
    }
    else
    {
        bInitializeCharacterOverlay = true;
        HUDHealth = Health;
        HUDMaxHealth = HUDMaxHealth;
    }
}

void ABlasterPlayerController::SetHUDScore(float Score)
{
    BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

    bool bHUDValid = BlasterHUD && 
        BlasterHUD->CharacterOverlay &&
        BlasterHUD->CharacterOverlay->ScoreAmount;

    if (bHUDValid)
    {
        FString ScoreString = FString::Printf(TEXT("%d"), FMath::FloorToInt(Score));
        BlasterHUD->CharacterOverlay->ScoreAmount->SetText(FText::FromString(ScoreString));
    }
    else
    {
        bInitializeCharacterOverlay = true;
        HUDScore = Score;
    }
}

void ABlasterPlayerController::SetHUDDefeats(int32 Defeats)
{
    BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
    
    bool bHUDValid = BlasterHUD && 
    BlasterHUD->CharacterOverlay &&
    BlasterHUD->CharacterOverlay->DefeatsAmount;

    if (bHUDValid)
    {
        FString DefeatsString = FString::Printf(TEXT("%d"), Defeats);
        BlasterHUD->CharacterOverlay->DefeatsAmount->SetText(FText::FromString(DefeatsString));
    }
    else
    {
        bInitializeCharacterOverlay = true;
        HUDDefeats = Defeats;
    }
}

void ABlasterPlayerController::AnnounceElim()
{
    BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

    if (BlasterHUD)
    {
        BlasterHUD->ShowElimMessage();
    }
}

void ABlasterPlayerController::SetHUDWeaponAmmo(int32 Ammo)
{
    BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
    
    bool bHUDValid = BlasterHUD && 
    BlasterHUD->CharacterOverlay &&
    BlasterHUD->CharacterOverlay->WeaponAmmoAmount;

    if (bHUDValid)
    {
        FString AmmoString = FString::Printf(TEXT("%d"), Ammo);
        BlasterHUD->CharacterOverlay->WeaponAmmoAmount->SetText(FText::FromString(AmmoString));
    }
    else
    {
        bInitializeCharacterOverlay = true;
        HUDAmmo = Ammo;
    }
}

void ABlasterPlayerController::SetHUDWeaponType(EWeaponType WeaponType)
{
    BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
    
    bool bHUDValid = BlasterHUD && 
    BlasterHUD->CharacterOverlay &&
    BlasterHUD->CharacterOverlay->WeaponTypeText;

    if (bHUDValid)
    {
        FString WeaponString;
        switch (WeaponType)
        {
            case EWeaponType::EWT_AssaultRifle:
                WeaponString = FString(TEXT("Assault Rifle"));
                break;
            case EWeaponType::EWT_MAX:
            default:
                WeaponString = FString("");
                break;
        }

        BlasterHUD->CharacterOverlay->WeaponTypeText->SetText(FText::FromString(WeaponString));
    }
    else
    {
        bInitializeCharacterOverlay = true;
        HUDWeaponType = WeaponType;
    }
}

void ABlasterPlayerController::SetHUDCarriedAmmo(int32 Ammo)
{
    BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
    
    bool bHUDValid = BlasterHUD && 
    BlasterHUD->CharacterOverlay &&
    BlasterHUD->CharacterOverlay->CarriedAmmoAmount;

    if (bHUDValid)
    {
        FString AmmoString = FString::Printf(TEXT("%d"), Ammo);
        BlasterHUD->CharacterOverlay->CarriedAmmoAmount->SetText(FText::FromString(AmmoString));
    }
    else
    {
        bInitializeCharacterOverlay = true;
        HUDCarriedAmmo = Ammo;
    }
}

void ABlasterPlayerController::SetHUDMatchCountdown(float CountdownTime)
{
    BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
    
    bool bHUDValid = BlasterHUD && 
    BlasterHUD->CharacterOverlay &&
    BlasterHUD->CharacterOverlay->MatchCountdownText;

    if (bHUDValid)
    {
        int32 Minutes = FMath::FloorToInt(CountdownTime / 60.f);
        int32 Seconds = CountdownTime - Minutes * 60;
        FString TimeString = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
        BlasterHUD->CharacterOverlay->MatchCountdownText->SetText(FText::FromString(TimeString));
    }
}

void ABlasterPlayerController::SetHUDAnnouncementCountdown(float CountdownTime)
{
    BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
    
    bool bHUDValid = BlasterHUD && 
    BlasterHUD->Announcement &&
    BlasterHUD->Announcement->WarmupTime;

    if (bHUDValid)
    {
        int32 Minutes = FMath::FloorToInt(CountdownTime / 60.f);
        int32 Seconds = CountdownTime - Minutes * 60;
        FString TimeString = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
        BlasterHUD->Announcement->WarmupTime->SetText(FText::FromString(TimeString));
    }
}

void ABlasterPlayerController::SetHUDTime()
{
    float TimeLeft;

    if (MatchState == MatchState::WaitingToStart)
    {
        TimeLeft = WarmupTime - GetServerTime() + LevelStartingTime;
    }
    else if (MatchState == MatchState::InProgress)
    {
        TimeLeft = WarmupTime + MatchTime - GetServerTime() + LevelStartingTime;
    }

    uint32 SecondsLeft = FMath::CeilToInt(TimeLeft);
    
    if (CountdownInt != SecondsLeft)
    {
        if (MatchState == MatchState::WaitingToStart)
        {
            SetHUDAnnouncementCountdown(TimeLeft);
        }
        else if (MatchState == MatchState::InProgress)
        {
            SetHUDMatchCountdown(TimeLeft);
        }
    }

    CountdownInt = SecondsLeft;
}

void ABlasterPlayerController::PollInit()
{
   if (CharacterOverlay == nullptr)
   {
        if (BlasterHUD && BlasterHUD->CharacterOverlay)
        {
            CharacterOverlay = BlasterHUD->CharacterOverlay;
            if (CharacterOverlay)
            {
                SetHUDHealth(HUDHealth, HUDMaxHealth);
                SetHUDScore(HUDScore);
                SetHUDDefeats(HUDDefeats);
                SetHUDWeaponType(EWeaponType::EWT_MAX);
                SetHUDCarriedAmmo(0);
                SetHUDWeaponAmmo(0);
            }
        }
   } 
}

void ABlasterPlayerController::ServerRequestServerTime_Implementation(float TimeOfClientRequest)
{
    float ServerTimeOfReceipt = GetWorld()->GetTimeSeconds();
    ClientReportServerTime(TimeOfClientRequest, ServerTimeOfReceipt);
}

void ABlasterPlayerController::ClientReportServerTime_Implementation(float TimeOfClientRequest, float TimeServerReceivedClientRequest)
{
    float RoundTripTime = GetWorld()->GetTimeSeconds() - TimeOfClientRequest;
    float CurrentServerTime = TimeServerReceivedClientRequest + 0.5f * RoundTripTime;
    ClientServerTimeDelta = CurrentServerTime - GetWorld()->GetTimeSeconds();
}

float ABlasterPlayerController::GetServerTime()
{
    if (HasAuthority())
    {
        return GetWorld()->GetTimeSeconds();
    }

    return GetWorld()->GetTimeSeconds() + ClientServerTimeDelta;
}

void ABlasterPlayerController::ReceivedPlayer()
{
    Super::ReceivedPlayer();
    
    if (IsLocalController())
    {
        ServerRequestServerTime(GetWorld()->GetTimeSeconds());
    }
}

void ABlasterPlayerController::OnMatchStateSet(FName State)
{
    MatchState = State;

    if (MatchState == MatchState::InProgress)
    {
        HandleMatchHasStarted();
    }
}

void ABlasterPlayerController::OnRep_MatchState()
{
    if (MatchState == MatchState::InProgress)
    {
        HandleMatchHasStarted();
    }
}

void ABlasterPlayerController::HandleMatchHasStarted()
{
    BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

    if (BlasterHUD)
    {
        BlasterHUD->AddCharacterOverlay();
        if (BlasterHUD->Announcement)
        {
            BlasterHUD->Announcement->SetVisibility(ESlateVisibility::Hidden);
        }
    }
}