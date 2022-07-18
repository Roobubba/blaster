// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pickup.h"
#include "DamagePickup.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API ADamagePickup : public APickup
{
	GENERATED_BODY()

public:
	ADamagePickup();

protected:
	virtual void OnSphereOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult	
	) override;

private:
	UPROPERTY(EditAnywhere)
	float DamageBuffMultiplier = 2.f;

	UPROPERTY(EditAnywhere)
	float BuffTime = 15.f;
};
