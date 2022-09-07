// Fill out your copyright notice in the Description page of Project Settings.


#include "FlagonZone.h"
#include "Components/SphereComponent.h"
#include "Blaster/Weapon/Flagon.h"
#include "Blaster/GameMode/CaptureTheFlagonGameMode.h"

AFlagonZone::AFlagonZone()
{
	PrimaryActorTick.bCanEverTick = false;
	ZoneSphere = CreateDefaultSubobject<USphereComponent>(TEXT("ZoneSphere"));
	SetRootComponent(ZoneSphere);

}

void AFlagonZone::BeginPlay()
{
	Super::BeginPlay();
	ZoneSphere->OnComponentBeginOverlap.AddDynamic(this, &AFlagonZone::OnSphereOverlap);
}

void AFlagonZone::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,	UPrimitiveComponent* OtherComp,	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AFlagon* OverlappingFlagon = Cast<AFlagon>(OtherActor);
	if (OverlappingFlagon && OverlappingFlagon->GetTeam() != Team)
	{
		UWorld* World = GetWorld();
		if (World)
		{
			ACaptureTheFlagonGameMode* CaptureTheFlagGameMode = World->GetAuthGameMode<ACaptureTheFlagonGameMode>();
			if (CaptureTheFlagGameMode)
			{
				CaptureTheFlagGameMode->FlagonCaptured(OverlappingFlagon, this);	
			}

			OverlappingFlagon->ResetFlagon();
		}
	}
}
