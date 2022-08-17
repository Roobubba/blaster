// Fill out your copyright notice in the Description page of Project Settings.

#include "ProjectileBullet.h"
#include "Kismet/GameplayStatics.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Blaster/BlasterComponents/LagCompensationComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Blaster/BlasterComponents/BuffComponent.h"

AProjectileBullet::AProjectileBullet()
{
    ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
    ProjectileMovementComponent->SetIsReplicated(true);
    ProjectileMovementComponent->InitialSpeed = InitialSpeed;
    ProjectileMovementComponent->MaxSpeed = InitialSpeed;
}

#if WITH_EDITOR
void AProjectileBullet::PostEditChangeProperty(struct FPropertyChangedEvent& Event)
{
    Super::PostEditChangeProperty(Event);

    FName PropertyName = Event.Property != nullptr ? Event.Property->GetFName() : NAME_None;
    if (PropertyName == GET_MEMBER_NAME_CHECKED(AProjectileBullet, InitialSpeed))
    {
        if (ProjectileMovementComponent)
        {
            ProjectileMovementComponent->InitialSpeed = InitialSpeed;
            ProjectileMovementComponent->MaxSpeed = InitialSpeed;
        }
    }
}
#endif

void AProjectileBullet::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
    ABlasterCharacter* OwnerCharacter = Cast<ABlasterCharacter>(GetOwner());

    if (OwnerCharacter)
    {
        ABlasterPlayerController* OwnerController = Cast<ABlasterPlayerController>(OwnerCharacter->Controller);
        if (OwnerController)
        {
            if (OwnerCharacter->HasAuthority() && (!bUseServerSideRewind || OwnerCharacter->IsLocallyControlled()))
            {
                float Multiplier = 1.f;
                
                if (OwnerCharacter && OwnerCharacter->GetBuffComponent())
                {
                    Multiplier = FMath::Max(Multiplier, OwnerCharacter->GetBuffComponent()->GetDamageMultiplier());
                }

                float DamageToApply = Multiplier * (Damage + (Hit.BoneName.ToString() == FString("head") ? HeadShotDamage : 0.f));
                UGameplayStatics::ApplyDamage(OtherActor, DamageToApply, OwnerController, this, UDamageType::StaticClass());
                Super::OnHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit);
                Destroy();
                return;
            }
            
            if (bUseServerSideRewind && OwnerCharacter->GetLagCompensation() && OwnerCharacter->IsLocallyControlled())
            {
                ABlasterCharacter* HitCharacter = Cast<ABlasterCharacter>(OtherActor);
                if (HitCharacter)
                {
                    OwnerCharacter->GetLagCompensation()->ProjectileServerScoreRequest
                        (
                            HitCharacter,
                            TraceStart,
                            InitialVelocity,
                            OwnerController->GetServerTime() - OwnerController->SingleTripTime
                        );
                }
            }
        }
    }

    Super::OnHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit);
    Destroy();
}

void AProjectileBullet::Destroyed()
{
    SpawnImpactEffects();

    Super::Destroyed();
}
