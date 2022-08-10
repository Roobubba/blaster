// Fill out your copyright notice in the Description page of Project Settings.

#include "ProjectileWeapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Projectile.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/BlasterComponents/BuffComponent.h"

void AProjectileWeapon::Fire (const FVector& HitTarget, const int32& Seed)
{
    Super::Fire(HitTarget, Seed);

    APawn* InstigatorPawn = Cast<APawn>(GetOwner());

    const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(FName("MuzzleFlash"));
    UWorld* World = GetWorld();
    if (MuzzleFlashSocket && World)
    {
        FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
        
        FVector ToTarget = VConeProcedural((HitTarget - SocketTransform.GetLocation()).GetSafeNormal(), Spread, 0, Seed);

        FRotator TargetRotation = ToTarget.Rotation();

        FActorSpawnParameters SpawnParams;
        SpawnParams.Owner = GetOwner();
        SpawnParams.Instigator = InstigatorPawn;

        if (!InstigatorPawn->HasAuthority() && !bUseServerSideRewind)
        {
            return;
        }

        TSubclassOf<AProjectile> ProjectileClassToSpawn = (InstigatorPawn->HasAuthority() && InstigatorPawn->IsLocallyControlled() && bUseServerSideRewind) ? ProjectileClass : ServerSideRewindProjectileClass;
        
        if (!ProjectileClassToSpawn) return;

        bool bUseSSR = (InstigatorPawn->HasAuthority() != InstigatorPawn->IsLocallyControlled()) && bUseServerSideRewind;
        
        AProjectile* SpawnedProjectile = nullptr; 
        SpawnedProjectile = World->SpawnActor<AProjectile>
            (
                ProjectileClassToSpawn,
                SocketTransform.GetLocation(),
                TargetRotation,
                SpawnParams
            );

        ABlasterCharacter* Character = Cast<ABlasterCharacter>(GetOwner());
        if (Character && SpawnedProjectile && Character->GetBuffComponent())
        {
            SpawnedProjectile->bUseServerSideRewind = bUseSSR;
            SpawnedProjectile->TraceStart = SocketTransform.GetLocation();
            SpawnedProjectile->InitialVelocity = ToTarget * SpawnedProjectile->InitialSpeed;

            float Multiplier = (Character->GetBuffComponent()->GetDamageMultiplier() > 1.f) ? Character->GetBuffComponent()->GetDamageMultiplier() : 1.f;
            SpawnedProjectile->Damage = GetDamage() * Multiplier;
        } 
    }
}