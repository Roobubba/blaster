// Fill out your copyright notice in the Description page of Project Settings.

#include "HitScanWeapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/BlasterComponents/BuffComponent.h" 
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"
#include "Kismet/KismetMathLibrary.h"
#include "Blaster/BlasterComponents/CombatComponent.h"
#include "Blaster/Blaster.h"

#include "DrawDebugHelpers.h"

void AHitScanWeapon::Fire(const FIntVector& HitTargetInt)
{
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

            float Multiplier = 1.f;

            ABlasterCharacter* OwnerCharacter = Cast<ABlasterCharacter>(GetOwner());
            if (OwnerCharacter && OwnerCharacter->GetBuffComponent())
            {
                Multiplier = FMath::Max(Multiplier, OwnerCharacter->GetBuffComponent()->GetDamageMultiplier());
            }

            TMap<ABlasterCharacter*, float> DamageMap;
            for (int i = 0; i < PelletCount; i++)
            {
                HitScan(Start, HitTargetInt, InstigatorController, DamageMap, Multiplier, (uint32) i, GenerateSeed(HitTargetInt));
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

    Super::Fire(HitTargetInt);
}

void AHitScanWeapon::HitScan(const FVector& TraceStart, const FIntVector& HitTargetInt, AController* InstigatorController, TMap<ABlasterCharacter*, float> &DamageMap, const float& DamageMultiplier, const uint32& PelletNum, const uint32& Seed)
{
    FVector HitTarget = FVector(HitTargetInt.X / 100.f, HitTargetInt.Y / 100.f, HitTargetInt.Z / 100.f);

    FVector NewTraceDirection = VConeProcedural((HitTarget - TraceStart).GetSafeNormal(), Spread, PelletNum, Seed);
    FVector End = TraceStart + (NewTraceDirection * TRACE_LENGTH);
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

            DrawDebugSphere
            (
                World,
                BeamEnd,
                16.f,
                12,
                FColor::Orange,
                true
            );

            if (HasAuthority() && InstigatorController)
            {
                ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(FireHit.GetActor());
                if (BlasterCharacter)
                {
                    if (DamageMap.Contains(BlasterCharacter))
                    {
                        DamageMap[BlasterCharacter] += Damage * DamageMultiplier;
                    }
                    else
                    {
                        DamageMap.Emplace(BlasterCharacter, Damage * DamageMultiplier);
                    }
                }
                else
                {
                    TArray<UPrimitiveComponent*> PrimitiveComponents;
                    FireHit.GetActor()->GetComponents(PrimitiveComponents, false);

                    if (PrimitiveComponents.Num() > 0 && PrimitiveComponents[0]->GetCollisionObjectType() == ECC_PhysicsMesh)
                    {
                        PrimitiveComponents[0]->AddImpulseAtLocation(NewTraceDirection * PhysicsImpactForcePerPellet * DamageMultiplier, BeamEnd);
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