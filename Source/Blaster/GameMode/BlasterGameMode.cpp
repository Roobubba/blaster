// Fill out your copyright notice in the Description page of Project Settings.

#include "BlasterGameMode.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "Blaster/PlayerState/BlasterPlayerState.h"
#include "Blaster/GameState/BlasterGameState.h"

namespace MatchState
{
	const FName Cooldown = FName("Cooldown");
}

ABlasterGameMode::ABlasterGameMode()
{
    bDelayedStart = true;
}

void ABlasterGameMode::BeginPlay()
{
    Super::BeginPlay();

    LevelStartingTime = GetWorld()->GetTimeSeconds();
}

void ABlasterGameMode::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (MatchState == MatchState::WaitingToStart)
    {
        CountdownTime = WarmupTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;

        if (CountdownTime <= 0.f)
        {
            StartMatch();
        }
    }
    else if (MatchState == MatchState::InProgress)
    {
        CountdownTime = WarmupTime + MatchTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;

        if (CountdownTime <= 0.f)
        {
            SetMatchState(MatchState::Cooldown);
        }
    }
    else if (MatchState == MatchState::Cooldown)
    {
        CountdownTime = CooldownTime + WarmupTime + MatchTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
        
        if (CountdownTime <= 0.f)
        {
            UWorld* World = GetWorld();
            if (World)
            {
                bUseSeamlessTravel = true;
                World->ServerTravel(FString("/Game/Maps/BlasterMap?listen"), true);
            }
        }
    }
}

void ABlasterGameMode::OnMatchStateSet()
{
    Super::OnMatchStateSet();

    if (MatchState == MatchState::WaitingToStart)
    {
        LevelStartingTime = GetWorld()->GetTimeSeconds();
    }
    
    for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
    {
        ABlasterPlayerController* BlasterPlayer = Cast<ABlasterPlayerController>(*Iterator);
        if (BlasterPlayer)
        {
            BlasterPlayer->ServerGetMatchState();
        }
    }
}

void ABlasterGameMode::PlayerEliminated(ABlasterCharacter* VictimCharacter, ABlasterPlayerController* VictimController, ABlasterPlayerController* AttackerController)
{
    ABlasterPlayerState* AttackerPlayerState = AttackerController ? Cast<ABlasterPlayerState>(AttackerController->PlayerState) : nullptr;
    ABlasterPlayerState* VictimPlayerState = VictimController ? Cast<ABlasterPlayerState>(VictimController->PlayerState) : nullptr;
    ABlasterGameState* BlasterGameState = GetGameState<ABlasterGameState>();

    if (AttackerPlayerState && AttackerPlayerState != VictimPlayerState && BlasterGameState)
    {
        TArray<ABlasterPlayerState*> PlayersCurrentlyInTheLead;
        for (auto LeadPlayer : BlasterGameState->TopScoringPlayers)
        {
            PlayersCurrentlyInTheLead.Add(LeadPlayer);
        }

        AttackerPlayerState->AddToScore(1.f);
        BlasterGameState->UpdateTopScore(AttackerPlayerState);

        if (BlasterGameState->TopScoringPlayers.Contains(AttackerPlayerState))
        {
            ABlasterCharacter* NewLeader = Cast<ABlasterCharacter>(AttackerPlayerState->GetPawn());
            if (NewLeader)
            {
                NewLeader->MulticastGainedTheLead();
            }
        }

        for (int32 i = 0; i < PlayersCurrentlyInTheLead.Num(); i++)
        {
            if (!BlasterGameState->TopScoringPlayers.Contains(PlayersCurrentlyInTheLead[i]))
            {
                ABlasterCharacter* Loser = Cast<ABlasterCharacter>(PlayersCurrentlyInTheLead[i]->GetPawn());
                if (Loser)
                {
                    Loser->MulticastLostTheLead();
                }
            }
        }
    }

    if (VictimPlayerState)
    {
        VictimPlayerState->AddToDefeats(1);
    }

    if (VictimCharacter)
    {
        VictimCharacter->Eliminate(false);
    }

    for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
    {
        ABlasterPlayerController* BlasterPlayer = Cast<ABlasterPlayerController>(*Iterator);
        if (BlasterPlayer && AttackerPlayerState && VictimPlayerState)
        {
            BlasterPlayer->BroadcastElimination(AttackerPlayerState, VictimPlayerState);
        }
    }
}

void ABlasterGameMode::RequestRespawn(ACharacter* EliminatedCharacter, AController* EliminatedController)
{
    if (EliminatedCharacter)
    {
        EliminatedCharacter->Reset();
        EliminatedCharacter->Destroy();
    }

    if (EliminatedController)
    {
        TArray<AActor*> PlayerStarts;
        UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), PlayerStarts);
        int32 Selection = FMath::RandRange(0, PlayerStarts.Num() - 1);
        RestartPlayerAtPlayerStart(EliminatedController, PlayerStarts[Selection]);
    }
}

void ABlasterGameMode::PlayerLeftGame(ABlasterPlayerState* PlayerLeaving)
{
    if (PlayerLeaving == nullptr)
    {
        return;
    }

    ABlasterGameState* BlasterGameState = GetGameState<ABlasterGameState>();
    if (BlasterGameState && BlasterGameState->TopScoringPlayers.Contains(PlayerLeaving))
    {
        BlasterGameState->TopScoringPlayers.Remove(PlayerLeaving);
    }

    ABlasterCharacter* CharacterLeaving = Cast<ABlasterCharacter>(PlayerLeaving->GetPawn());
    if (CharacterLeaving)
    {
        CharacterLeaving->Eliminate(true);
    }
}