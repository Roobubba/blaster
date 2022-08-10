// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon.h"
#include "ProjectileWeapon.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API AProjectileWeapon : public AWeapon
{
	GENERATED_BODY()
	
public:
	virtual void Fire (const FVector& HitTarget, const int32& Seed) override;

private:
	UPROPERTY(EditAnywhere)
	TSubclassOf<class AProjectile> ProjectileClass;

	UPROPERTY(EditAnywhere)
	TSubclassOf<AProjectile> ServerSideRewindProjectileClass;

	void SpawnProjectile(const TSubclassOf<AProjectile>& ProjectileClassToSpawn, const FVector& Location, const FRotator& TargetRotation, const FVector& ToTarget, const FActorSpawnParameters& SpawnParams, const bool& bUseSSR);
};
