// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Projectile.h"
#include "ProjectileGrenade.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API AProjectileGrenade : public AProjectile
{
	GENERATED_BODY()

public:
	AProjectileGrenade();

#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& Event) override;
#endif

protected:
	virtual void BeginPlay() override;

	void StartExplodeTimer();
	void ExplodeTimerFinished();
	void ApplyDamage();
	void ApplyPhysicsImpulses(float DamageMultiplier);

	UFUNCTION()
	void OnBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity);

private:
	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = "true"))
	class USoundCue* BounceSound;

	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = "true"))
	float InnerDamageRadius = 200.f;

	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = "true"))
	float OuterDamageRadius = 500.f;

	FTimerHandle ExplodeTimer;

	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = "true"))
	float ExplodeTime = 2.f;
};
