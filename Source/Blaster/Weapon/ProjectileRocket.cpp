// Fill out your copyright notice in the Description page of Project Settings.

#include "ProjectileRocket.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "Components/BoxComponent.h"
#include "Sound/SoundCue.h"
#include "Components/AudioComponent.h"
#include "RocketMovementComponent.h"
#include "Blaster/Blaster.h"

AProjectileRocket::AProjectileRocket()
{
    ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RocketMesh"));
    ProjectileMesh->SetupAttachment(RootComponent);
    ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    RocketMovementComponent = CreateDefaultSubobject<URocketMovementComponent>(TEXT("Rocket Movement Component"));
    RocketMovementComponent->bRotationFollowsVelocity = true;
    RocketMovementComponent->SetIsReplicated(true);
}

void AProjectileRocket::BeginPlay()
{
    Super::BeginPlay();

    if (!HasAuthority())
	{
		CollisionBox->OnComponentHit.AddDynamic(this, &AProjectileRocket::OnHit);
	}

    SpawnTrailSystem();

    if (ProjectileLoop && ProjectileLoopAttentuation)
    {
        ProjectileLoopComponent = UGameplayStatics::SpawnSoundAttached
        (
            ProjectileLoop,
            GetRootComponent(),
            FName(),
            GetActorLocation(),
            EAttachLocation::KeepWorldPosition,
            false,
            1.f,
            1.f,
            0.f,
            ProjectileLoopAttentuation,
            (USoundConcurrency*)nullptr,
            false
        );
    }
}

void AProjectileRocket::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
    if (OtherActor == GetOwner())
    {
        //UE_LOG(LogTemp, Warning, TEXT("Hit self"));
        return;
    }

    APawn* FiringPawn = GetInstigator();
    if (FiringPawn && HasAuthority())
    {
        AController* FiringController = FiringPawn->GetController();

        if (FiringController)
        {
            UGameplayStatics::ApplyRadialDamageWithFalloff(
                this,
                Damage,
                Damage / 4.f,
                GetActorLocation(),
                InnerDamageRadius,
                OuterDamageRadius,
                1.f,
                UDamageType::StaticClass(),
                TArray<AActor*>(),
                this,
                FiringController
            );
        }
    }

    SpawnImpactEffects();

    StartDestroyTimer();

    Super::OnHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit);

    ApplyPhysicsImpulses();

    if (ProjectileMesh)
    {
        ProjectileMesh->SetVisibility(false);
    }

    if (CollisionBox)
    {
        CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }

    if (TrailSystemComponent && TrailSystemComponent->GetSystemInstanceController())
    {
        TrailSystemComponent->GetSystemInstanceController()->Deactivate();
    }

    if (ProjectileLoopComponent && ProjectileLoopComponent->IsPlaying())
    {
        ProjectileLoopComponent->Stop();
    }
}

void AProjectileRocket::ApplyPhysicsImpulses()
{
    if (HasAuthority())
    {
        TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
        ObjectTypes.Add(TEnumAsByte(UEngineTypes::ConvertToObjectType(ECC_PhysicsMesh)));

        UClass* ActorClassFilter = AActor::StaticClass();
        
        TArray<AActor*> IgnoreActors;
        IgnoreActors.Init(this, 1);

        TArray<AActor*> OutActors;
        
        bool HitActors = UKismetSystemLibrary::SphereOverlapActors
        (
            this,
            GetActorLocation(),
            OuterDamageRadius,
            ObjectTypes,
            ActorClassFilter,
            IgnoreActors,
            OutActors
        );

        if (HitActors)
        {
            for (AActor* HitActor : OutActors)
            {
                TArray<UPrimitiveComponent*> PrimitiveComponents;
                HitActor->GetComponents(PrimitiveComponents, false);

                if (PrimitiveComponents.Num() > 0 && PrimitiveComponents[0]->GetCollisionObjectType() == ECC_PhysicsMesh)
                {
                    FVector ImpulseDirection = HitActor->GetActorLocation() - GetActorLocation();
                    float Force = 0.5f * PhysicsImpactForce * (1.f - FMath::Clamp((ImpulseDirection.Size() - InnerDamageRadius) / (2.f * OuterDamageRadius - InnerDamageRadius), 0.f, 1.f));
                    PrimitiveComponents[0]->AddImpulse(ImpulseDirection.GetSafeNormal() * Force);
                }
            }
        }
    }
}