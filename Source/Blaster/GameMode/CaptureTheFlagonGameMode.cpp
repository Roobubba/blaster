// Fill out your copyright notice in the Description page of Project Settings.


#include "CaptureTheFlagonGameMode.h"
#include "Blaster/Weapon/Flagon.h"
#include "Blaster/CaptureTheFlagon/FlagonZone.h"
#include "Blaster/GameState/BlasterGameState.h"

void ACaptureTheFlagonGameMode::PlayerEliminated(ABlasterCharacter* VictimCharacter, ABlasterPlayerController* VictimController, ABlasterPlayerController* AttackerController)
{
    ABlasterGameMode::PlayerEliminated(VictimCharacter, VictimController, AttackerController);

}

void ACaptureTheFlagonGameMode::FlagonCaptured(AFlagon* Flagon, AFlagonZone* Zone)
{
    if (Flagon == nullptr || Zone == nullptr)
    {
        return;
    }

    bool bValidCapture = Flagon->GetTeam() != Zone->Team;

    ABlasterGameState* BlasterGameState = Cast<ABlasterGameState>(GameState);
    if (BlasterGameState)
    {
        if (Zone->Team == ETeam::ET_BlueTeam)
        {
            BlasterGameState->BlueTeamScores();
        }
        
        if (Zone->Team == ETeam::ET_RedTeam)
        {
            BlasterGameState->RedTeamScores();
        }
    }
}