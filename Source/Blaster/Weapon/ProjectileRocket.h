// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Projectile.h"
#include "ProjectileRocket.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API AProjectileRocket : public AProjectile
{
	GENERATED_BODY()

public:

	AProjectileRocket();

#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& Event) override;
#endif

protected:

	virtual void BeginPlay() override;
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit) override;
	
	void ApplyPhysicsImpulses();

	UPROPERTY(EditAnywhere)
	class USoundCue* ProjectileLoop;

	UPROPERTY()
	class UAudioComponent* ProjectileLoopComponent;

	UPROPERTY(EditAnywhere)
	class USoundAttenuation* ProjectileLoopAttentuation;

	UPROPERTY(VisibleAnywhere)
	class URocketMovementComponent* RocketMovementComponent;

private:

	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = "true"))
	float InnerDamageRadius = 200.f;

	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = "true"))
	float OuterDamageRadius = 500.f;
};
