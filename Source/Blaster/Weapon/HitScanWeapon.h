// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon.h"
#include "HitScanWeapon.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API AHitScanWeapon : public AWeapon
{
	GENERATED_BODY()

public:

	virtual void Fire(const FVector& HitTarget, const int32& Seed) override;

protected:

	virtual void HitScan(const FVector& TraceStart, const FVector& HitTarget, TMap<ABlasterCharacter*, float> &DamageMap, const float& DamageMultiplier, const uint32& PelletNum, const uint32& Seed);
	
	UPROPERTY(EditAnywhere)
	int32 PelletCount = 1;

	UPROPERTY(EditAnywhere)
	float PhysicsImpactForcePerPellet = 14000.f;
	
private:

	UPROPERTY(EditAnywhere)
	class UParticleSystem* ImpactParticles;

	UPROPERTY(EditAnywhere)
	UParticleSystem* BeamParticles;

	UPROPERTY(EditAnywhere)
	UParticleSystem* MuzzleFlash;

	UPROPERTY(EditAnywhere)
	class USoundCue* FireSound;

	UPROPERTY(EditAnywhere)
	USoundCue* HitSound;
};
