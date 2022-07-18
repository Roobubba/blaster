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
#include "Blaster/BlasterComponents/CombatComponent.h"
#include "Blaster/BlasterComponents/BuffComponent.h"
#include "Blaster/GameState/BlasterGameState.h"
#include "Blaster/PlayerState/BlasterPlayerState.h"
#include "Styling/SlateColor.h"
#include "Camera/CameraComponent.h"

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
        ClientJoinMidgame(GameMode->GetMatchState(), GameMode->WarmupTime, GameMode->MatchTime, GameMode->CooldownTime, GameMode->LevelStartingTime);
    }
}

void ABlasterPlayerController::ClientJoinMidgame_Implementation(FName StateOfMatch, float Warmup, float Match, float Cooldown, float StartingTime)
{
    WarmupTime = Warmup;
    MatchTime = Match;
    LevelStartingTime = StartingTime;
    MatchState = StateOfMatch;
    CooldownTime = Cooldown;
    OnMatchStateSet(MatchState);
    
    UWorld* World = GetWorld();
    if (World)
    {
        if (GetCharacter())
        {
            ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(GetCharacter());
            if (BlasterCharacter)
            {
                BlasterCharacter->UpdateHUDHealth();
                BlasterCharacter->UpdateHUDShield();
                //BlasterCharacter->UpdateHUDAmmo();

                if (BlasterCharacter->GetBuffComponent())
                {
                    BlasterCharacter->GetBuffComponent()->UpdateHUDHealing();
                    BlasterCharacter->GetBuffComponent()->UpdateHUDShieldRegen();
                }
            }
        }
    }  
}

void ABlasterPlayerController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    // Ensure server HUD updates at start

    ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(InPawn);
    if (BlasterCharacter)
    {
        SetHUDHealth(BlasterCharacter->GetHealth(), BlasterCharacter->GetMaxHealth());
        SetHUDShield(BlasterCharacter->GetShield(), BlasterCharacter->GetMaxShield());

        BlasterCharacter->UpdateHUDAmmo();

        if (BlasterCharacter->GetBuffComponent())
        {
            BlasterCharacter->GetBuffComponent()->UpdateHUDHealing();
            BlasterCharacter->GetBuffComponent()->UpdateHUDShieldRegen();
        }
    }
}

void ABlasterPlayerController::SetHUDHealthExtraHealing(float HealingPercent)
{
    BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

    if (BlasterHUD)
    {
        CharacterOverlay = CharacterOverlay == nullptr ? BlasterHUD->GetCharacterOverlay() : CharacterOverlay;

        bool bHUDValid = CharacterOverlay &&
            CharacterOverlay->HealthBarHealing;

        if (bHUDValid)
        {
            CharacterOverlay->HealthBarHealing->SetPercent(HealingPercent);
        }
    }
}

void ABlasterPlayerController::SetHUDHealth(float Health, float MaxHealth)
{
    BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

    if (BlasterHUD)
    {
        CharacterOverlay = CharacterOverlay == nullptr ? BlasterHUD->GetCharacterOverlay() : CharacterOverlay;

        bool bHUDValid = CharacterOverlay &&
            CharacterOverlay->HealthBar &&
            CharacterOverlay->HealthText;

        if (bHUDValid)
        {
            const float HealthPercent = Health / MaxHealth;
            CharacterOverlay->HealthBar->SetPercent(HealthPercent);
            
            FString HealthString = FString::Printf(TEXT("%d/%d"), FMath::CeilToInt(Health), FMath::CeilToInt(MaxHealth));
            CharacterOverlay->HealthText->SetText(FText::FromString(HealthString));
            HUDHealth = 100.f;
            HUDMaxHealth = 100.f;
        }
        else
        {
            bInitializeHUDHealth = true;
            HUDHealth = Health;
            HUDMaxHealth = MaxHealth;
        }
    }
}

void ABlasterPlayerController::SetHUDShield(float Shield, float MaxShield)
{
    BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

    if (BlasterHUD)
    {
        CharacterOverlay = CharacterOverlay == nullptr ? BlasterHUD->GetCharacterOverlay() : CharacterOverlay;

        bool bHUDValid = CharacterOverlay &&
            CharacterOverlay->ShieldBar &&
            CharacterOverlay->ShieldText;

        if (bHUDValid)
        {
            const float ShieldPercent = Shield / MaxShield;
            CharacterOverlay->ShieldBar->SetPercent(ShieldPercent);
            
            FString ShieldString = FString::Printf(TEXT("%d/%d"), FMath::CeilToInt(Shield), FMath::CeilToInt(MaxShield));
            CharacterOverlay->ShieldText->SetText(FText::FromString(ShieldString));
            HUDShield = 100.f;
            HUDMaxShield = 100.f;
        }
        else
        {
            bInitializeHUDShield = true;
            HUDShield = Shield;
            HUDMaxShield = Shield;
        }
    }
}

void ABlasterPlayerController::SetHUDShieldExtraRegen(float ShieldRegenPercent)
{
    BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

    if (BlasterHUD)
    {
        CharacterOverlay = CharacterOverlay == nullptr ? BlasterHUD->GetCharacterOverlay() : CharacterOverlay;

        bool bHUDValid = CharacterOverlay &&
            CharacterOverlay->ShieldBarRegen;

        if (bHUDValid)
        {
            CharacterOverlay->ShieldBarRegen->SetPercent(ShieldRegenPercent);
        }
    }
}



void ABlasterPlayerController::SetHUDScore(float Score)
{
    BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

    if (BlasterHUD)
    {
        CharacterOverlay = CharacterOverlay == nullptr ? BlasterHUD->GetCharacterOverlay() : CharacterOverlay;

        bool bHUDValid = CharacterOverlay &&
            CharacterOverlay->ScoreAmount;

        if (bHUDValid)
        {
            FString ScoreString = FString::Printf(TEXT("%d"), FMath::FloorToInt(Score));
            CharacterOverlay->ScoreAmount->SetText(FText::FromString(ScoreString));
            HUDScore = 0;
        }
        else
        {
            bInitializeHUDScore = true;
            HUDScore = Score;
        }
    }
}

void ABlasterPlayerController::SetHUDDefeats(int32 Defeats)
{
    BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

    if (BlasterHUD)
    {
        CharacterOverlay = CharacterOverlay == nullptr ? BlasterHUD->GetCharacterOverlay() : CharacterOverlay;
    
        bool bHUDValid = CharacterOverlay &&
        CharacterOverlay->DefeatsAmount;

        if (bHUDValid)
        {
            FString DefeatsString = FString::Printf(TEXT("%d"), Defeats);
            CharacterOverlay->DefeatsAmount->SetText(FText::FromString(DefeatsString));
            HUDDefeats = 0;
        }
        else
        {
            bInitializeHUDDefeats = true;
            HUDDefeats = Defeats;
        }
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

    if (BlasterHUD)
    {
        CharacterOverlay = CharacterOverlay == nullptr ? BlasterHUD->GetCharacterOverlay() : CharacterOverlay;
    
        bool bHUDValid = CharacterOverlay &&
        CharacterOverlay->WeaponAmmoAmount;

        if (bHUDValid)
        {
            FString AmmoString = FString::Printf(TEXT("%d"), Ammo);
            CharacterOverlay->WeaponAmmoAmount->SetText(FText::FromString(AmmoString));
            HUDAmmo = 0;
        }
        else
        {
            bInitializeHUDWeaponAmmo = true;
            HUDAmmo = Ammo;
        }
    }
}

void ABlasterPlayerController::SetHUDWeaponType(EWeaponType WeaponType)
{
    BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

    if (BlasterHUD)
    {
        CharacterOverlay = CharacterOverlay == nullptr ? BlasterHUD->GetCharacterOverlay() : CharacterOverlay;
    
        bool bHUDValid = CharacterOverlay &&
        CharacterOverlay->WeaponTypeText;

        if (bHUDValid)
        {
            FString WeaponString;
            switch (WeaponType)
            {
                case EWeaponType::EWT_MAX:
                    WeaponString = FString(TEXT(""));
                    break;
                case EWeaponType::EWT_AssaultRifle:
                    WeaponString = FString(TEXT("Assault Rifle"));
                    break;
                case EWeaponType::EWT_GrenadeLauncher:
                    WeaponString = FString(TEXT("Grenade Launcher"));
                    break;
                case EWeaponType::EWT_Pistol:
                    WeaponString = FString(TEXT("Pistol"));
                    break;
                case EWeaponType::EWT_RocketLauncher:
                    WeaponString = FString(TEXT("Rocket Launcher"));
                    break;
                case EWeaponType::EWT_Shotgun:
                    WeaponString = FString(TEXT("Shotgun"));
                    break;
                case EWeaponType::EWT_SniperRifle:
                    WeaponString = FString(TEXT("Sniper Rifle"));
                    break;
                case EWeaponType::EWT_SubmachineGun:
                    WeaponString = FString(TEXT("SMG"));
                    break;
            }

            CharacterOverlay->WeaponTypeText->SetText(FText::FromString(WeaponString));
            HUDWeaponType = EWeaponType::EWT_MAX;
        }
        else
        {
            bInitializeHUDWeaponType = true;
            HUDWeaponType = WeaponType;
        }
    }
}

void ABlasterPlayerController::SetHUDCarriedAmmo(int32 Ammo)
{
    BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

    if (BlasterHUD)
    {
        CharacterOverlay = CharacterOverlay == nullptr ? BlasterHUD->GetCharacterOverlay() : CharacterOverlay;
    
        bool bHUDValid = CharacterOverlay &&
        CharacterOverlay->CarriedAmmoAmount;

        if (bHUDValid)
        {
            FString AmmoString = FString::Printf(TEXT("%d"), Ammo);
            CharacterOverlay->CarriedAmmoAmount->SetText(FText::FromString(AmmoString));
            HUDCarriedAmmo = 0;
        }
        else
        {
            bInitializeHUDCarriedAmmo = true;
            HUDCarriedAmmo = Ammo;
        }
    }
}

void ABlasterPlayerController::SetHUDMatchCountdown(float CountdownTime)
{
    BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

    if (BlasterHUD)
    {
        CharacterOverlay = CharacterOverlay == nullptr ? BlasterHUD->GetCharacterOverlay() : CharacterOverlay;
    
        bool bHUDValid = CharacterOverlay &&
        CharacterOverlay->MatchCountdownText;

        if (bHUDValid)
        {
            if (CountdownTime < 0.f)
            {
                CharacterOverlay->MatchCountdownText->SetText(FText());
                return;
            }

            int32 Minutes = FMath::FloorToInt(CountdownTime / 60.f);
            int32 Seconds = CountdownTime - Minutes * 60;
            FString TimeString = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
            CharacterOverlay->MatchCountdownText->SetText(FText::FromString(TimeString));

            CharacterOverlay->MatchCountdownText->SetColorAndOpacity((CountdownTime < 30.f) ? FSlateColor(FColor::Red) : FSlateColor(FColor::White));
        }
        else
        {
            CharacterOverlay = nullptr;
        }
    }
}

void ABlasterPlayerController::SetHUDAnnouncementCountdown(float CountdownTime)
{
    BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

    if (BlasterHUD)
    {
        Announcement = Announcement == nullptr ? BlasterHUD->GetAnnouncement() : Announcement;
    
        bool bHUDValid = Announcement &&
        Announcement->WarmupTime;

        if (bHUDValid)
        {
            if (CountdownTime < 0.f)
            {
                Announcement->WarmupTime->SetText(FText::FromString("Loading..."));
                return;
            }

            int32 Minutes = FMath::FloorToInt(CountdownTime / 60.f);
            int32 Seconds = CountdownTime - Minutes * 60;
            FString TimeString = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
            Announcement->WarmupTime->SetText(FText::FromString(TimeString));
        }
        else
        {
            Announcement = nullptr;
        }
    }
}

void ABlasterPlayerController::SetHUDTime()
{
    float TimeLeft = 0.f;

    if (MatchState == MatchState::WaitingToStart)
    {
        TimeLeft = WarmupTime - GetServerTime() + LevelStartingTime;
    }
    else if (MatchState == MatchState::InProgress)
    {
        TimeLeft = WarmupTime + MatchTime - GetServerTime() + LevelStartingTime;
    }
    else if (MatchState == MatchState::Cooldown)
    {
        TimeLeft = WarmupTime + MatchTime + CooldownTime - GetServerTime() + LevelStartingTime;
    }

    uint32 SecondsLeft = FMath::CeilToInt(TimeLeft);
    
    if (HasAuthority())
    {
        BlasterGameMode = BlasterGameMode == nullptr ? Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this)) :  BlasterGameMode;
        if (BlasterGameMode)
        {
            SecondsLeft = FMath::CeilToInt(BlasterGameMode->GetCountdownTime() + LevelStartingTime);
        }
    }

    if (CountdownInt != SecondsLeft)
    {
        if (MatchState == MatchState::WaitingToStart || MatchState == MatchState::Cooldown)
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

void ABlasterPlayerController::SetHUDGrenades(int32 Grenades)
{
    BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

    if (BlasterHUD)
    {
        CharacterOverlay = CharacterOverlay == nullptr ? BlasterHUD->GetCharacterOverlay() : CharacterOverlay;
    
        bool bHUDValid = CharacterOverlay &&
        CharacterOverlay->GrenadesText;

        if (bHUDValid)
        {
            FString GrenadesString = FString::Printf(TEXT("%d"), Grenades);
            CharacterOverlay->GrenadesText->SetText(FText::FromString(GrenadesString));
            HUDGrenades = 0;
        }
        else
        {
            bInitializeHUDGrenades = true;
            HUDGrenades = Grenades;
        }
    }
}

void ABlasterPlayerController::PollInit()
{
    if (CharacterOverlay == nullptr)
    {
        BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
        if (BlasterHUD)
        {
            CharacterOverlay = CharacterOverlay == nullptr ? BlasterHUD->GetCharacterOverlay() : CharacterOverlay;
            if (CharacterOverlay)
            {
                if (bInitializeHUDHealth) SetHUDHealth(HUDHealth, HUDMaxHealth);
                if (bInitializeHUDShield) SetHUDShield(HUDShield, HUDMaxShield);  
                if (bInitializeHUDScore) SetHUDScore(HUDScore);
                if (bInitializeHUDDefeats) SetHUDDefeats(HUDDefeats);
                if (bInitializeHUDWeaponType) SetHUDWeaponType(HUDWeaponType);
                if (bInitializeHUDCarriedAmmo) SetHUDCarriedAmmo(HUDCarriedAmmo);
                if (bInitializeHUDWeaponAmmo) SetHUDWeaponAmmo(HUDAmmo);
                
                if (bInitializeHUDGrenades)
                {
                    ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(GetPawn());
				    if (BlasterCharacter && BlasterCharacter->GetCombat())
                    {
                        SetHUDGrenades(BlasterCharacter->GetCombat()->GetGrenades());
                    }
                }

                ServerGetMatchState();
            }
        }
    }

    if (Announcement == nullptr)
    {
        BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
        if (BlasterHUD)
        {
            Announcement = Announcement == nullptr ? BlasterHUD->GetAnnouncement() : Announcement;
            if (Announcement)
            {
                ServerGetMatchState();
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
    else if (MatchState == MatchState::Cooldown)
    {
        HandleCooldown();
    }
    else if (MatchState == MatchState::WaitingToStart)
    {
        HandleWaitingToStart();
    }
}

void ABlasterPlayerController::OnRep_MatchState()
{
    if (MatchState == MatchState::InProgress)
    {
        HandleMatchHasStarted();
       
        UWorld* World = GetWorld();
        if (World)
        {
            if (GetCharacter())
            {
                ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(GetCharacter());
                if (BlasterCharacter)
                {
                    BlasterCharacter->UpdateHUDHealth();
                    BlasterCharacter->UpdateHUDShield();
                    BlasterCharacter->UpdateHUDAmmo();

                    if (BlasterCharacter->GetBuffComponent())
                    {
                        BlasterCharacter->GetBuffComponent()->UpdateHUDHealing();
                        BlasterCharacter->GetBuffComponent()->UpdateHUDShieldRegen();
                    }
                }
            }
        }
    }
    else if (MatchState == MatchState::Cooldown)
    {
        HandleCooldown();
    }
    else if (MatchState == MatchState::WaitingToStart)
    {
        HandleWaitingToStart();
    }
}

void ABlasterPlayerController::HandleWaitingToStart()
{
    BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

    if (BlasterHUD)
    {
        CharacterOverlay = CharacterOverlay == nullptr ? BlasterHUD->GetCharacterOverlay() : CharacterOverlay;

        if (CharacterOverlay)
        {
            CharacterOverlay->SetVisibility(ESlateVisibility::Hidden);
        }

        Announcement = Announcement == nullptr ? BlasterHUD->GetAnnouncement() : Announcement;
    
        if (Announcement)
        {
            if (Announcement->AnnouncementText && Announcement->InfoText)
            {
                Announcement->SetVisibility(ESlateVisibility::Visible);
                FString AnnouncementString("Match Starts In:");
                Announcement->AnnouncementText->SetText(FText::FromString(AnnouncementString));
                FString MatchStartInfoString("Fly Around - W A S D");
                Announcement->InfoText->SetText(FText::FromString(MatchStartInfoString));
            }
        }
    }
}

void ABlasterPlayerController::HandleMatchHasStarted()
{
    BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

    if (BlasterHUD)
    {
        CharacterOverlay = CharacterOverlay == nullptr ? BlasterHUD->GetCharacterOverlay() : CharacterOverlay;

        if (CharacterOverlay)
        {
            CharacterOverlay->SetVisibility(ESlateVisibility::Visible);
        }
        
        Announcement = Announcement == nullptr ? BlasterHUD->GetAnnouncement() : Announcement;

        if (Announcement)
        {
            Announcement->SetVisibility(ESlateVisibility::Hidden);
        }
    }
}

void ABlasterPlayerController::HandleCooldown()
{
    BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;

    if (BlasterHUD)
    {
        CharacterOverlay = CharacterOverlay == nullptr ? BlasterHUD->GetCharacterOverlay() : CharacterOverlay;
        if (CharacterOverlay)
        {
            CharacterOverlay->SetVisibility(ESlateVisibility::Hidden);
        }
        
        Announcement = Announcement == nullptr ? BlasterHUD->GetAnnouncement() : Announcement;
    
        if (Announcement)
        {
            if (Announcement->AnnouncementText && Announcement->InfoText)
            {
                Announcement->SetVisibility(ESlateVisibility::Visible);
                FString AnnouncementString("New Match Starts In:");
                Announcement->AnnouncementText->SetText(FText::FromString(AnnouncementString));

                ABlasterGameState* BlasterGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this));
                ABlasterPlayerState* BlasterPlayerState = GetPlayerState<ABlasterPlayerState>();
                if (BlasterGameState && BlasterPlayerState)
                {
                    FString WinningPlayersString;

                    TArray<ABlasterPlayerState*> TopPlayers = BlasterGameState->TopScoringPlayers;
                    if (TopPlayers.Num() == 0)
                    {
                        WinningPlayersString = FString("Nobody won, losers!");
                    }
                    else if (TopPlayers.Num() == 1 && TopPlayers[0] == BlasterPlayerState)
                    {
                        WinningPlayersString = FString("You are the winner!");
                    }
                    else if (TopPlayers.Num() == 1)
                    {
                        WinningPlayersString = FString::Printf(TEXT("Winner:\n%s"), *TopPlayers[0]->GetPlayerName());
                    }
                    else if (TopPlayers.Num() > 1)
                    {
                        WinningPlayersString = FString::Printf(TEXT("Players tied for the win:\n"));
                        for (ABlasterPlayerState* TiedPlayer : TopPlayers)
                        {
                            WinningPlayersString.Append(FString::Printf(TEXT("%s\n"), *TiedPlayer->GetPlayerName()));
                        }
                    }

                    Announcement->InfoText->SetText(FText::FromString(WinningPlayersString));
                }
            }
        }
    }

    ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(GetPawn());

    if (BlasterCharacter && BlasterCharacter->GetCombat())
    {
        BlasterCharacter->bDisableGameplay = true;
        BlasterCharacter->GetCombat()->FireButtonPressed(false);
        BlasterCharacter->GetCombat()->DisableCrosshairs();
        
        if (BlasterCharacter->IsAiming())
        {
            BlasterCharacter->GetCombat()->SetAiming(false);

            if (BlasterCharacter->GetFollowCamera())
            {
                BlasterCharacter->GetFollowCamera()->SetFieldOfView(BlasterCharacter->GetCombat()->GetDefaultFOV());
            }
        }

        BlasterCharacter->GetCombat()->HandleRoundEnd();
    }
}