// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TeamsGameMode.h"
#include "CaptureTheFlagonGameMode.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API ACaptureTheFlagonGameMode : public ATeamsGameMode
{
	GENERATED_BODY()

public:
	virtual void PlayerEliminated(class ABlasterCharacter* VictimCharacter, class ABlasterPlayerController* VictimController, ABlasterPlayerController* AttackerController) override;
	void FlagonCaptured(class AFlagon* Flagon, class AFlagonZone* Zone);

};
