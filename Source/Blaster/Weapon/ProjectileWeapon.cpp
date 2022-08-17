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
        const FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
        
        const FVector ToTarget = VConeProcedural((HitTarget - SocketTransform.GetLocation()).GetSafeNormal(), Spread, 0, Seed);

        const FRotator TargetRotation = ToTarget.Rotation();

        FActorSpawnParameters SpawnParams;
        SpawnParams.Owner = GetOwner();
        SpawnParams.Instigator = InstigatorPawn;

        TSubclassOf<AProjectile> ProjectileClassToSpawn;
        bool bUseSSR = false;
        
        if (bUseServerSideRewind)
        {
            if (InstigatorPawn->HasAuthority())
            {
                if (InstigatorPawn->IsLocallyControlled())
                {
                    ProjectileClassToSpawn = ProjectileClass;
                    bUseSSR = false; 
                }
                else
                {
                    ProjectileClassToSpawn = ServerSideRewindProjectileClass;
                    bUseSSR = true;
                }
            }
            else
            {
                if (InstigatorPawn->IsLocallyControlled())
                {
                    ProjectileClassToSpawn = ServerSideRewindProjectileClass;
                    bUseSSR = true;
                }
                else
                {
                    ProjectileClassToSpawn = ServerSideRewindProjectileClass;
                    bUseSSR = false;
                }
            }
        }
        else if (InstigatorPawn->HasAuthority())
        {
            ProjectileClassToSpawn = ProjectileClass;
            bUseSSR = false;
        }
        
        if (ProjectileClassToSpawn)
        {
            SpawnProjectile(ProjectileClassToSpawn, SocketTransform.GetLocation(), TargetRotation, ToTarget, SpawnParams, bUseSSR);
        }
    }
}

void AProjectileWeapon::SpawnProjectile(const TSubclassOf<AProjectile>& ProjectileClassToSpawn, const FVector& Location, const FRotator& TargetRotation, const FVector& ToTarget, const FActorSpawnParameters& SpawnParams, const bool& bUseSSR)
{
    AProjectile* SpawnedProjectile = nullptr; 
    SpawnedProjectile = GetWorld()->SpawnActor<AProjectile>
        (
            ProjectileClassToSpawn,
            Location,
            TargetRotation,
            SpawnParams
        );

    ABlasterCharacter* Character = Cast<ABlasterCharacter>(GetOwner());
    if (Character && SpawnedProjectile && Character->GetBuffComponent())
    {
        SpawnedProjectile->bUseServerSideRewind = bUseSSR;
        SpawnedProjectile->TraceStart = Location;
        SpawnedProjectile->InitialVelocity = ToTarget * SpawnedProjectile->InitialSpeed;

        SpawnedProjectile->Damage = GetDamage();
        SpawnedProjectile->HeadShotDamage = GetHeadShotDamage();
    }
}