// Fill out your copyright notice in the Description page of Project Settings.

#include "ProjectileGrenade.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Sound/SoundCue.h"
#include "Blaster/Blaster.h"
#include "Components/BoxComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "NiagaraEmitterInstance.h"

AProjectileGrenade::AProjectileGrenade()
{
    ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GrenadeMesh"));
    ProjectileMesh->SetupAttachment(RootComponent);
    ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
    ProjectileMovementComponent->SetIsReplicated(true);
    ProjectileMovementComponent->bShouldBounce = true;
    
}

#if WITH_EDITOR
void AProjectileGrenade::PostEditChangeProperty(struct FPropertyChangedEvent& Event)
{
    Super::PostEditChangeProperty(Event);

    FName PropertyName = Event.Property != nullptr ? Event.Property->GetFName() : NAME_None;
    if (PropertyName == GET_MEMBER_NAME_CHECKED(AProjectileGrenade, InitialSpeed))
    {
        if (ProjectileMovementComponent)
        {
            ProjectileMovementComponent->InitialSpeed = InitialSpeed;
            ProjectileMovementComponent->MaxSpeed = InitialSpeed;
        }
    }
}
#endif

void AProjectileGrenade::BeginPlay()
{
    AActor::BeginPlay();
    
    bUseServerSideRewind = false;

    StartExplodeTimer();
    SpawnTrailSystem();
    ProjectileMovementComponent->OnProjectileBounce.AddDynamic(this, &AProjectileGrenade::OnBounce);
}

void AProjectileGrenade::OnBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity)
{
    if (BounceSound)
    {
        UGameplayStatics::PlaySoundAtLocation
        (
            this,
            BounceSound,
            GetActorLocation()
        );
    }
}

void AProjectileGrenade::StartExplodeTimer()
{
	GetWorldTimerManager().SetTimer(ExplodeTimer, this, &AProjectileGrenade::ExplodeTimerFinished, ExplodeTime);
}

void AProjectileGrenade::ExplodeTimerFinished()
{
    SpawnImpactEffects();
    ApplyDamage();
    ApplyPhysicsImpulses();

    if (ProjectileMesh)
    {
        ProjectileMesh->SetVisibility(false);
    }

    if (ProjectileMovementComponent)
    {
        ProjectileMovementComponent->StopMovementImmediately();
        ProjectileMovementComponent->OnProjectileBounce.RemoveAll(this);
    }

    if (CollisionBox)
    {
        CollisionBox->SetActive(false);
    }

    if (TrailSystem && TrailSystemComponent)
    {
        TrailSystemComponent->SetVariableFloat(FName("SmokeSpawnRate"), 0.f);
    }

    StartDestroyTimer();
}

void AProjectileGrenade::ApplyDamage()
{
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
}

void AProjectileGrenade::ApplyPhysicsImpulses()
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
                    float Force = PhysicsImpactForce * (1.f - FMath::Clamp((ImpulseDirection.Size() - InnerDamageRadius) / (2.f * OuterDamageRadius - InnerDamageRadius), 0.f, 1.f));
                    PrimitiveComponents[0]->AddImpulse(ImpulseDirection.GetSafeNormal() * Force);
                }
            }
        }
    }
}