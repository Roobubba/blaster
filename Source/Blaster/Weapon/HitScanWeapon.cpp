// Fill out your copyright notice in the Description page of Project Settings.

#include "HitScanWeapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"
#include "Kismet/KismetMathLibrary.h"
#include "Blaster/BlasterComponents/CombatComponent.h"
#include "Blaster/Blaster.h"

void AHitScanWeapon::Fire(const FVector& HitTarget)
{
    Super::Fire(HitTarget);

    APawn* OwnerPawn = Cast<APawn>(GetOwner());

    if (OwnerPawn == nullptr)
    {
        return;
    }
    
    AController* InstigatorController = OwnerPawn->GetController();

    const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");

    if (MuzzleFlashSocket)
    {
        FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
        FVector Start = SocketTransform.GetLocation();

        FHitResult FireHit;
        UWorld* World = GetWorld();
        if (World)
        {
            if (MuzzleFlash)
            {
                UGameplayStatics::SpawnEmitterAtLocation
                (
                    World,
                    MuzzleFlash,
                    SocketTransform
                );
            }

            if (FireSound)
            {
                UGameplayStatics::PlaySoundAtLocation
                (
                    this,
                    FireSound,
                    GetActorLocation()
                );
            }

            TMap<ABlasterCharacter*, float> DamageMap;
            for (int i = 0; i < PelletCount; i++)
            {
                HitScan(Start, HitTarget, InstigatorController, DamageMap);
            }

            if (HasAuthority() && InstigatorController)
            {
                for(auto DamageEvent : DamageMap)
                {
                    if (DamageEvent.Key)
                    {
                        UGameplayStatics::ApplyDamage(DamageEvent.Key, DamageEvent.Value, InstigatorController, this, UDamageType::StaticClass());
                    }
                }
            }
        }
    }
}

void AHitScanWeapon::HitScan(const FVector& TraceStart, const FVector& HitTarget, AController* InstigatorController, TMap<ABlasterCharacter*, float> &DamageMap)
{
    FVector NewTraceDirection = UKismetMathLibrary::RandomUnitVectorInConeInDegrees((HitTarget - TraceStart).GetSafeNormal(), Spread);
    FVector End = TraceStart + (NewTraceDirection * UCombatComponent::GetTraceLength());
    FHitResult FireHit;

    UWorld* World = GetWorld();
    if (World)
    {
        World->LineTraceSingleByChannel
        (
            FireHit,
            TraceStart,
            End,
            ECollisionChannel::ECC_Visibility
        );

        FVector BeamEnd = End;

        if (FireHit.bBlockingHit)
        {
            BeamEnd = FireHit.ImpactPoint;

            if (HasAuthority() && InstigatorController)
            {
                ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(FireHit.GetActor());
                if (BlasterCharacter)
                {
                    if (DamageMap.Contains(BlasterCharacter))
                    {
                        DamageMap[BlasterCharacter] += Damage;
                    }
                    else
                    {
                        DamageMap.Emplace(BlasterCharacter, Damage);
                    }
                }
                else
                {
                    TArray<UPrimitiveComponent*> PrimitiveComponents;
                    FireHit.GetActor()->GetComponents(PrimitiveComponents, false);

                    if (PrimitiveComponents.Num() > 0 && PrimitiveComponents[0]->GetCollisionObjectType() == ECC_PhysicsMesh)
                    {
                        UE_LOG(LogTemp, Warning, TEXT("Hit UPrimitiveComponent"));
                        PrimitiveComponents[0]->AddImpulseAtLocation(NewTraceDirection * PhysicsImpactForcePerPellet, BeamEnd);
                    }
                }

            }

            if (ImpactParticles)
            {
                UGameplayStatics::SpawnEmitterAtLocation
                (
                    World,
                    ImpactParticles,
                    FireHit.ImpactPoint,
                    FireHit.ImpactNormal.Rotation()
                );
            }

            if (HitSound)
            {
                UGameplayStatics::PlaySoundAtLocation
                (
                    this,
                    HitSound,
                    FireHit.ImpactPoint,
                    0.5f,
                    FMath::FRandRange(-0.5f, 0.5f)
                );
            }
        }

        if (BeamParticles)
        {
            UParticleSystemComponent* Beam = UGameplayStatics::SpawnEmitterAtLocation
            (
                World,
                BeamParticles,
                TraceStart,
                FRotator::ZeroRotator,
                true
            );

            if (Beam)
            {
                Beam->SetVectorParameter(FName("Target"), BeamEnd);
            }
        }
    }
}