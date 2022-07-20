// Fill out your copyright notice in the Description page of Project Settings.

#include "Weapon.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Net/UnrealNetwork.h"
#include "Animation/AnimationAsset.h"
#include "Components/SkeletalMeshComponent.h"
#include "Casing.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Blaster/BlasterComponents/CombatComponent.h"

AWeapon::AWeapon()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);
	
	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	SetRootComponent(WeaponMesh);

	WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	WeaponMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_BLUE);
	WeaponMesh->MarkRenderStateDirty();
	EnableCustomDepth(true);

	AreaSphere = CreateDefaultSubobject<USphereComponent>(TEXT("AreaSphere"));
	AreaSphere->SetupAttachment(RootComponent);

	AreaSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	PickupWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("WidgetComponent"));
	PickupWidget->SetupAttachment(RootComponent);
}

void AWeapon::BeginPlay()
{
	Super::BeginPlay();
	
	if (PickupWidget)
	{
		PickupWidget->SetVisibility(false);
	}

	AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	AreaSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	AreaSphere->OnComponentBeginOverlap.AddDynamic(this, &AWeapon::OnSphereOverlap);
	AreaSphere->OnComponentEndOverlap.AddDynamic(this, &AWeapon::OnSphereEndOverlap);
}

void AWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWeapon, WeaponState);
	DOREPLIFETIME(AWeapon, Ammo);
}

void AWeapon::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,	UPrimitiveComponent* OtherComp,	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor);
	if (BlasterCharacter)
	{
		BlasterCharacter->SetOverlappingWeapon(this);
	}
}

void AWeapon::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor);
	if (BlasterCharacter)
	{
		BlasterCharacter->SetOverlappingWeapon(nullptr);
	}
}

void AWeapon::SetWeaponState(EWeaponState State)
{
	WeaponState = State;
	OnWeaponStateSet();
}

void AWeapon::OnWeaponStateSet()
{
	switch (WeaponState)
	{
		case EWeaponState::EWS_Dropped:
			OnDropped();
			break;
		case EWeaponState::EWS_Equipped:
			OnEquipped();
			break;
		case EWeaponState::EWS_EquippedSecondary:
			OnEquippedSecondary();
			break;
		default:
			break;
	}
}

void AWeapon::OnRep_WeaponState()
{
	OnWeaponStateSet();
}

void AWeapon::OnEquipped()
{
	ShowPickupWidget(false);
	if (AreaSphere)
	{
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (WeaponMesh == nullptr)
	{
		return;
	}
	EnableCustomDepth(false);
	WeaponMesh->SetSimulatePhysics(false);
	if (WeaponType == EWeaponType::EWT_SubmachineGun)
	{
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		WeaponMesh->SetEnableGravity(true);
		WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	}
	else
	{
		WeaponMesh->SetEnableGravity(false);
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}	
}

void AWeapon::OnDropped()
{
	if (HasAuthority() && AreaSphere)
	{
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}

	if (WeaponMesh == nullptr)
	{
		return;
	}
	
	WeaponMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_BLUE);
	WeaponMesh->MarkRenderStateDirty();
	EnableCustomDepth(true);
	WeaponMesh->SetSimulatePhysics(true);
	WeaponMesh->SetEnableGravity(true);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
}

void AWeapon::OnEquippedSecondary()
{
	if (AreaSphere)
	{
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	ShowPickupWidget(false);

	if (WeaponMesh == nullptr)
	{
		return;
	}
	
	BlasterOwnerCharacter = BlasterOwnerCharacter == nullptr ? Cast<ABlasterCharacter>(GetOwner()) : BlasterOwnerCharacter;
	if (BlasterOwnerCharacter)
	{
		if (BlasterOwnerCharacter->IsLocallyControlled())
		{
			WeaponMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_TAN);
			WeaponMesh->MarkRenderStateDirty();
			EnableCustomDepth(true);
		}
	}

	WeaponMesh->SetSimulatePhysics(false);

	if (WeaponType == EWeaponType::EWT_SubmachineGun)
	{
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		WeaponMesh->SetEnableGravity(true);
		WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	}
	else
	{
		WeaponMesh->SetEnableGravity(false);
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void AWeapon::ShowPickupWidget(bool bShowWidget)
{
	if (PickupWidget)
	{
		PickupWidget->SetVisibility(bShowWidget);
	}
}

void AWeapon::Fire(const FVector& HitTarget)
{
	if (FireAnimation)
	{
		WeaponMesh->PlayAnimation(FireAnimation, false);
	}

	if (CasingClass)
	{
		
		const USkeletalMeshSocket* AmmoEjectSocket = WeaponMesh->GetSocketByName("AmmoEject");

		if (AmmoEjectSocket)
		{
			FTransform SocketTransform = AmmoEjectSocket->GetSocketTransform(WeaponMesh);

			FActorSpawnParameters SpawnParams;

			UWorld* World = GetWorld();
			if (World)
			{
				World->SpawnActor<ACasing>(
				CasingClass,
				SocketTransform.GetLocation(),
				SocketTransform.GetRotation().Rotator()
				);
			}
		}
	}

	if (HasAuthority())
	{
		SpendRound();
	}
}

void AWeapon::Dropped()
{
	BlasterOwnerCharacter = BlasterOwnerCharacter == nullptr ? Cast<ABlasterCharacter>(GetOwner()) : BlasterOwnerCharacter;
	if (BlasterOwnerCharacter)
	{
		BlasterOwnerController = BlasterOwnerController == nullptr ? Cast<ABlasterPlayerController>(BlasterOwnerCharacter->Controller) : BlasterOwnerController;
		if (BlasterOwnerController)
		{
			BlasterOwnerController->SetHUDWeaponType(EWeaponType::EWT_MAX);
			BlasterOwnerController->SetHUDWeaponAmmo(0);
			BlasterOwnerController->SetHUDCarriedAmmo(0);
		}
	}

	SetWeaponState(EWeaponState::EWS_Dropped);
	FDetachmentTransformRules DetachRules(EDetachmentRule::KeepWorld, true);
	WeaponMesh->DetachFromComponent(DetachRules);

	SetOwner(nullptr);
	BlasterOwnerCharacter = nullptr;
	BlasterOwnerController = nullptr;
}

void AWeapon::SetHUDAmmo()
{
	BlasterOwnerCharacter = BlasterOwnerCharacter == nullptr ? Cast<ABlasterCharacter>(GetOwner()) : BlasterOwnerCharacter;
	if (BlasterOwnerCharacter)
	{
		BlasterOwnerController = BlasterOwnerController == nullptr ? Cast<ABlasterPlayerController>(BlasterOwnerCharacter->Controller) : BlasterOwnerController;
		if (BlasterOwnerController)
		{
			BlasterOwnerController->SetHUDWeaponAmmo(Ammo);
		}
	}
}

void AWeapon::SetHUDWeaponType()
{
	BlasterOwnerCharacter = BlasterOwnerCharacter == nullptr ? Cast<ABlasterCharacter>(GetOwner()) : BlasterOwnerCharacter;
	if (BlasterOwnerCharacter)
	{
		BlasterOwnerController = BlasterOwnerController == nullptr ? Cast<ABlasterPlayerController>(BlasterOwnerCharacter->Controller) : BlasterOwnerController;
		if (BlasterOwnerController)
		{
			BlasterOwnerController->SetHUDWeaponType(WeaponType);
		}
	}
}

void AWeapon::SpendRound()
{
	Ammo = FMath::Clamp(Ammo - 1, 0, MagCapacity);
	SetHUDAmmo();
}

void AWeapon::OnRep_Ammo()
{
	BlasterOwnerCharacter = BlasterOwnerCharacter == nullptr ? Cast<ABlasterCharacter>(GetOwner()) : BlasterOwnerCharacter;
	if (BlasterOwnerCharacter && BlasterOwnerCharacter->GetCombat() && GetIsFull())
	{	
		BlasterOwnerCharacter->GetCombat()->JumpToShotgunEnd();
	}

	SetHUDAmmo();
}

void AWeapon::OnRep_Owner()
{
	Super::OnRep_Owner();
	
	if (Owner == nullptr)
	{
		BlasterOwnerCharacter = nullptr;
		BlasterOwnerController = nullptr;
	}
	else
	{
		BlasterOwnerCharacter = BlasterOwnerCharacter == nullptr ? Cast<ABlasterCharacter>(Owner) : BlasterOwnerCharacter;
		if (BlasterOwnerCharacter && BlasterOwnerCharacter->GetEquippedWeapon() && BlasterOwnerCharacter->GetEquippedWeapon() == this)
		{
			//SetHUDAmmo();
			//SetHUDWeaponType();
			BlasterOwnerCharacter->UpdateHUDAmmo();
		}
	}
}

void AWeapon::AddAmmo(int32 AmmoToAdd)
{
	Ammo = FMath::Clamp(Ammo + AmmoToAdd, 0, MagCapacity);
	SetHUDAmmo();
}

void AWeapon::EnableCustomDepth(bool bEnable)
{
	if (WeaponMesh)
	{
		WeaponMesh->SetRenderCustomDepth(bEnable);
	}
}

uint32 AWeapon::Hash(const uint32 Input, const uint32 Seed)
{
    uint32 Mangled = Input;
    Mangled *= NOISE_A;
    Mangled += Seed;
    Mangled ^= (Mangled >> 8);
    Mangled += NOISE_B;
    Mangled ^= (Mangled << 8);
    Mangled *= NOISE_C;
    Mangled ^= (Mangled >> 8);

    return Mangled;
}

float AWeapon::HashFloatZeroToOne(const uint32 Input, const uint32 Seed)
{
    return (float) ((int) AWeapon::Hash(Input, Seed) * RANDOM_TO_FLOAT) + 0.5f;
}

uint32 AWeapon::GenerateSeed(const FVector& Dir)
{
	//FString NameServerClient = FString("Client ");
	//if (HasAuthority())
	//{
	//	NameServerClient = FString("Server ");
	//}
	//
	//UE_LOG(LogTemp, Warning, TEXT("%sDir values: %f, %f, %f"), *NameServerClient, Dir.X, Dir.Y, Dir.Z);

	uint32 ModifiedX = (uint32)FMath::RoundToInt32((Dir.X + 2.f) * 4);
	uint32 ModifiedY = (uint32)FMath::RoundToInt32((Dir.Y + 2.f) * 4);
	uint32 ModifiedZ = (uint32)FMath::RoundToInt32((Dir.Z + 2.f) * 4);

	//UE_LOG(LogTemp, Warning, TEXT("%sModified values: %d, %d, %d"), *NameServerClient, ModifiedX, ModifiedY, ModifiedZ);

	uint32 Seed = Ammo + (ModifiedX * 10000) + (ModifiedY * 1000) + (ModifiedZ * 100);
	return Seed;
}

FVector AWeapon::VConeProcedural(FVector const& Dir, float ConeHalfAngleDeg, uint32 PelletNum)
{
	const float ConeHalfAngleRad = FMath::DegreesToRadians(ConeHalfAngleDeg);
	if (ConeHalfAngleRad > 0.f)
	{
		const uint32 Seed = GenerateSeed(Dir);
		float const RandU = HashFloatZeroToOne(Seed, PelletNum);
		float const RandV = HashFloatZeroToOne(Seed, LARGERANDOM_A * PelletNum);

		// Get spherical coords that have an even distribution over the unit sphere
		// Method described at http://mathworld.wolfram.com/SpherePointPicking.html	
		float Theta = 2.f * PI * RandU;
		float Phi = FMath::Acos((2.f * RandV) - 1.f);

		// restrict phi to [0, ConeHalfAngleRad]
		// this gives an even distribution of points on the surface of the cone
		// centered at the origin, pointing upward (z), with the desired angle
		Phi = FMath::Fmod(Phi, ConeHalfAngleRad);

		// get axes we need to rotate around
		FMatrix const DirMat = FRotationMatrix(Dir.Rotation());
		// note the axis translation, since we want the variation to be around X
		FVector const DirZ = DirMat.GetScaledAxis( EAxis::X );		
		FVector const DirY = DirMat.GetScaledAxis( EAxis::Y );

		FVector Result = Dir.RotateAngleAxis(Phi * 180.f / PI, DirY);
		Result = Result.RotateAngleAxis(Theta * 180.f / PI, DirZ);

		// ensure it's a unit vector (might not have been passed in that way)
		Result = Result.GetSafeNormal();
		
		return Result;
	}
	else
	{
		return Dir.GetSafeNormal();
	}
}