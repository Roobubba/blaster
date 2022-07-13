// Fill out your copyright notice in the Description page of Project Settings.

#include "CombatComponent.h"
#include "Blaster/Weapon/Weapon.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Components/SphereComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Sound/SoundCue.h"
#include "Camera/CameraComponent.h"
#include "Engine/Scene.h"
#include "TimerManager.h"
#include "Blaster/GameState/BlasterGameState.h"
#include "Blaster/GameMode/BlasterGameMode.h"
#include "Blaster/Character/BlasterAnimInstance.h"
#include "Blaster/Weapon/Projectile.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	BaseWalkSpeed = 600.f;
	AimWalkSpeed = 350.f;
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCombatComponent, EquippedWeapon);
	DOREPLIFETIME(UCombatComponent, bAiming);
	DOREPLIFETIME(UCombatComponent, CombatState);
	DOREPLIFETIME_CONDITION(UCombatComponent, CarriedAmmo, COND_OwnerOnly);
	DOREPLIFETIME(UCombatComponent, Grenades);
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	CombatState = ECombatState::ECS_Unoccupied;
	CarriedAmmo = 0;
	Grenades = 4;

	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
		if (Character->GetFollowCamera())
		{
			DefaultFOV = Character->GetFollowCamera()->FieldOfView;
			CurrentFOV = DefaultFOV;
		}

		if (Character->HasAuthority())
		{
			InitializeCarriedAmmo();
		}
	}

	bDrawCrosshairs = true;
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (Character && Character->IsLocallyControlled())
	{
		FHitResult HitResult;
		TraceUnderCrosshairs(HitResult);
		HitTarget = HitResult.ImpactPoint;

		InterpFOV(DeltaTime);
		SetHUDCrosshairs(DeltaTime);
	}
}

void UCombatComponent::InterpFOV(float DeltaTime)
{
	if (EquippedWeapon == nullptr)
	{
		return;
	}

	if (bAiming)
	{
		CurrentFOV = FMath::FInterpTo(CurrentFOV, EquippedWeapon->GetZoomedFOV(), DeltaTime, EquippedWeapon->GetZoomInterpSpeed());
	}
	else
	{
		CurrentFOV = FMath::FInterpTo(CurrentFOV, DefaultFOV, DeltaTime, ZoomInterpSpeed);
	}

	if (Character && Character->GetFollowCamera())
	{
		Character->GetFollowCamera()->SetFieldOfView(CurrentFOV);
	}
}

void UCombatComponent::SetHUDCrosshairs(float DeltaTime)
{
	if (Character == nullptr || Character->Controller == nullptr)
	{
		return;
	}

	Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;

	if (Controller && Controller->GetMatchState() == MatchState::InProgress)
	{
		BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(Controller->GetHUD()) : BlasterHUD;

		if (BlasterHUD)
		{
			
			if (EquippedWeapon)
			{
				HUDPackage.CrosshairsCentre = EquippedWeapon->CrosshairsCentre;
				HUDPackage.CrosshairsLeft = EquippedWeapon->CrosshairsLeft;
				HUDPackage.CrosshairsRight = EquippedWeapon->CrosshairsRight;
				HUDPackage.CrosshairsTop = EquippedWeapon->CrosshairsTop;
				HUDPackage.CrosshairsBottom = EquippedWeapon->CrosshairsBottom;
			}
			else
			{
				HUDPackage.CrosshairsCentre = nullptr;
				HUDPackage.CrosshairsLeft = nullptr;
				HUDPackage.CrosshairsRight = nullptr;
				HUDPackage.CrosshairsTop = nullptr;
				HUDPackage.CrosshairsBottom = nullptr;
			}

			FVector Velocity = Character->GetVelocity();
			Velocity.Z = 0.f;
			CrosshairVelocityFactor = Velocity.Size() / BaseWalkSpeed;

			if (Character->GetCharacterMovement()->IsFalling())
			{
				CrosshairInAirFactor =  FMath::FInterpTo(CrosshairInAirFactor, 2.25f, DeltaTime, 2.25f);
			}
			else
			{
				CrosshairInAirFactor =  FMath::FInterpTo(CrosshairInAirFactor, 0.f, DeltaTime, 30.f);
			}

			if (bAiming)
			{
				CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, -0.58f, DeltaTime, 30.f);
			}
			else
			{
				CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.f, DeltaTime, 30.f);
			}

			CrosshairShootingFactor = FMath::FInterpTo(CrosshairShootingFactor, 0.f, DeltaTime, EquippedWeapon ? EquippedWeapon->CrosshairShootingInterpSpeed : 40.f);

			HUDPackage.CrosshairSpread = 
				0.58f +
				CrosshairVelocityFactor +
				CrosshairInAirFactor +
				CrosshairAimFactor +
				CrosshairShootingFactor;

			BlasterHUD->SetHUDPackage(HUDPackage);
		}
	}
}

void UCombatComponent::DisableCrosshairs()
{
	bDrawCrosshairs = false;
	
	if (Character == nullptr || Character->Controller == nullptr)
	{
		return;
	}

	Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;

	if (Controller)
	{
		BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(Controller->GetHUD()) : BlasterHUD;

		if (BlasterHUD)
		{
			HUDPackage.CrosshairsCentre = nullptr;
			HUDPackage.CrosshairsLeft = nullptr;
			HUDPackage.CrosshairsRight = nullptr;
			HUDPackage.CrosshairsTop = nullptr;
			HUDPackage.CrosshairsBottom = nullptr;
			HUDPackage.CrosshairSpread = 0.58f;
			BlasterHUD->SetHUDPackage(HUDPackage);
		}
	}
}

void UCombatComponent::SetAiming(bool bIsAiming)
{
	if (Character == nullptr || EquippedWeapon == nullptr)
	{
		return;
	}

	bAiming = bIsAiming;
	
	ServerSetAiming(bIsAiming);

	Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;

	if (Character->IsLocallyControlled() && EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SniperRifle)
	{
		Character->ShowSniperScopeWidget(bIsAiming);
	}
}

void UCombatComponent::ServerSetAiming_Implementation(bool bIsAiming)
{
	bAiming = bIsAiming;
	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
}

void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip)
{
	if (Character == nullptr || WeaponToEquip == nullptr || CombatState != ECombatState::ECS_Unoccupied)
	{
		return;
	}

	DropEquippedWeapon();
	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
	AttachActorToRightHand(EquippedWeapon);
	EquippedWeapon->SetOwner(Character);
	EquippedWeapon->SetHUDAmmo();
	EquippedWeapon->SetHUDWeaponType();
	UpdateCurrentAmmo();
	PlayEquipWeaponSound();
	ReloadEmptyWeapon();
	Character->GetCharacterMovement()->bOrientRotationToMovement = false;
	Character->bUseControllerRotationYaw = true;
}

void UCombatComponent::DropEquippedWeapon()
{
	if (EquippedWeapon)
	{
		EquippedWeapon->Dropped();
	}
}

void UCombatComponent::AttachActorToRightHand(AActor* ActorToAttach)
{
	if (Character == nullptr || Character->GetMesh() == nullptr || ActorToAttach == nullptr)
	{
		return;
	}

	const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket"));
	if (HandSocket)
	{
		HandSocket->AttachActor(ActorToAttach, Character->GetMesh());
	}
}

void UCombatComponent::AttachActorToLeftHand(AActor* ActorToAttach)
{
	if (Character == nullptr || Character->GetMesh() == nullptr || ActorToAttach == nullptr || EquippedWeapon == nullptr)
	{
		return;
	}

	bool bUsePistolSocket = EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Pistol || 
		EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SubmachineGun;

	FName SocketName = bUsePistolSocket ? FName("PistolSocket") : FName("LeftHandSocket");

	const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(SocketName);
	if (HandSocket)
	{
		HandSocket->AttachActor(ActorToAttach, Character->GetMesh());
	}
}

void UCombatComponent::UpdateCurrentAmmo()
{
	if (EquippedWeapon == nullptr)
	{
		Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;

		if (Controller)
		{
			CarriedAmmo = 0;
			Controller->SetHUDCarriedAmmo(0);
			Controller->SetHUDWeaponAmmo(0);
			Controller->SetHUDWeaponType(EWeaponType::EWT_MAX);
		}

		return;
	}

	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
	}

	Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;

	if (Controller)
	{
		Controller->SetHUDCarriedAmmo(CarriedAmmo);
	}
}

void UCombatComponent::PlayEquipWeaponSound()
{
	if (Character && EquippedWeapon && EquippedWeapon->EquipSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, EquippedWeapon->EquipSound, Character->GetActorLocation());
	}
}

void UCombatComponent::ReloadEmptyWeapon()
{
	if (EquippedWeapon && EquippedWeapon->GetIsEmpty())
	{
		Reload();
	}
}

void UCombatComponent::OnRep_EquippedWeapon()
{
	if (EquippedWeapon && Character)
	{
		EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
		AttachActorToRightHand(EquippedWeapon);
		PlayEquipWeaponSound();
		Character->GetCharacterMovement()->bOrientRotationToMovement = false;
		Character->bUseControllerRotationYaw = true;
	}
}

void UCombatComponent::Reload()
{
	if (CarriedAmmo > 0 && CombatState == ECombatState::ECS_Unoccupied && EquippedWeapon && !EquippedWeapon->GetIsFull())
	{
		ServerReload();
	}
}

void UCombatComponent::ServerReload_Implementation()
{
	if (Character == nullptr || EquippedWeapon == nullptr)
	{
		return;
	}

	CombatState = ECombatState::ECS_Reloading;
	HandleReload();
}

void UCombatComponent::FinishReloading()
{
	if (Character == nullptr) return;
	if (Character->HasAuthority())
	{
		CombatState = ECombatState::ECS_Unoccupied;
		UpdateAmmoValues();
	}

	if (bFireButtonPressed)
	{
		Fire();
	}
}

void UCombatComponent::UpdateAmmoValues()
{
	if (EquippedWeapon == nullptr)
	{
		Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;

		if (Controller)
		{
			CarriedAmmo = 0;
			Controller->SetHUDCarriedAmmo(0);
			Controller->SetHUDWeaponAmmo(0);
			Controller->SetHUDWeaponType(EWeaponType::EWT_MAX);
		}
		
		return;
	}

	int32 ReloadAmount = AmountToReload();

	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmoMap[EquippedWeapon->GetWeaponType()] -= ReloadAmount;
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
	}

	if (Controller)
	{
		Controller->SetHUDCarriedAmmo(CarriedAmmo);
	}

	EquippedWeapon->AddAmmo(ReloadAmount);
}

void UCombatComponent::UpdateShotgunAmmoValues()
{
	if (Character == nullptr || EquippedWeapon == nullptr)
	{
		return;
	}

	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmoMap[EquippedWeapon->GetWeaponType()] -= 1;
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
	}

	Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;

	if (Controller)
	{
		Controller->SetHUDCarriedAmmo(CarriedAmmo);
	}
	
	EquippedWeapon->AddAmmo(1);

	bCanFire = true;

	if (EquippedWeapon->GetIsFull() || CarriedAmmo <= 0)
	{
		JumpToShotgunEnd();
	}
}

void UCombatComponent::JumpToShotgunEnd()
{
	UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance();

	if (AnimInstance && Character->GetReloadAnimMontage())
	{
		AnimInstance->Montage_JumpToSection(FName("ShotgunEnd"));
	}
}

void UCombatComponent::ShotgunShellReload()
{
	if (Character && Character->HasAuthority())
	{
		UpdateShotgunAmmoValues();
	}
}

void UCombatComponent::OnRep_CombatState()
{
	switch (CombatState)
	{
		case ECombatState::ECS_Unoccupied:
			if (bFireButtonPressed)
			{
				Fire();
			}
			break;
		case ECombatState::ECS_Reloading:
			HandleReload();
			break;
		case ECombatState::ECS_ThrowingGrenade:
			if (Character && !Character->IsLocallyControlled())
			{
				Character->PlayThrowGrenadeMontage();
				AttachActorToLeftHand(EquippedWeapon);
				ShowAttachedGrenade(true);
			}
			break;
	}
}

void UCombatComponent::HandleReload()
{
	if (Character == nullptr) return;

	Character->PlayReloadMontage();
}

void UCombatComponent::FireButtonPressed(bool bPressed)
{
	bFireButtonPressed = bPressed;

	if (bFireButtonPressed && EquippedWeapon)
	{
		Fire();
	}
}

int32 UCombatComponent::AmountToReload()
{
	if (EquippedWeapon == nullptr)
	{
		return 0;
	}

	int32 SpaceInWeapon = EquippedWeapon->GetMagCapacity() - EquippedWeapon->GetAmmo();

	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		int32 AmountCarried = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
		int32 AmountToAdd = FMath::Min(AmountCarried, SpaceInWeapon);
		return FMath::Max(AmountToAdd, 0);
	}

	return 0;
}

void UCombatComponent::Fire()
{
	if (CanFire())
	{
		bCanFire = false;
		ServerFire(HitTarget);

		if (EquippedWeapon)
		{
			CrosshairShootingFactor += EquippedWeapon->CrosshairShootingExpansionAmount;
			CrosshairShootingFactor = FMath::Clamp(CrosshairShootingFactor, 0.f, 4.f);
		}

		StartFireTimer();
	}
}

void UCombatComponent::StartFireTimer()
{
	if (EquippedWeapon == nullptr || Character == nullptr)
	{
		return;
	}

	Character->GetWorldTimerManager().SetTimer(FireTimer, this, &UCombatComponent::FireTimerFinished, EquippedWeapon->GetFireDelay());
}

void UCombatComponent::FireTimerFinished()
{
	if (EquippedWeapon == nullptr)
	{
		return;
	}

	bCanFire = true;
	
	if (bFireButtonPressed && EquippedWeapon && EquippedWeapon->GetIsAutomatic())
	{
		Fire();
	}

	ReloadEmptyWeapon();
}

void UCombatComponent::ServerFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	MulticastFire(TraceHitTarget);
}

void UCombatComponent::MulticastFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	if (EquippedWeapon == nullptr)
	{
		return;
	}

	if (Character && CombatState == ECombatState::ECS_Reloading && EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Shotgun)
	{
		Character->PlayFireMontage(bAiming);
		EquippedWeapon->Fire(TraceHitTarget);
		CombatState = ECombatState::ECS_Unoccupied;
		return;
	}

	if (Character && CombatState == ECombatState::ECS_Unoccupied)
	{
		Character->PlayFireMontage(bAiming);
		EquippedWeapon->Fire(TraceHitTarget);
	}
}

void UCombatComponent::ThrowGrenade()
{
	if (CombatState != ECombatState::ECS_Unoccupied || EquippedWeapon == nullptr || Grenades <= 0)
	{
		return;
	}

	CombatState = ECombatState::ECS_ThrowingGrenade;

	if (Character)
	{
		Character->PlayThrowGrenadeMontage();
		AttachActorToLeftHand(EquippedWeapon);
		ShowAttachedGrenade(true);

	
		if (!Character->HasAuthority())
		{
			ServerThrowGrenade();
		}
		else
		{
			Grenades = FMath::Clamp(Grenades - 1, 0, MaxGrenades);
			UpdateHUDGrenades();
		}
	}
}

void UCombatComponent::ServerThrowGrenade_Implementation()
{
	CombatState = ECombatState::ECS_ThrowingGrenade;

	if (Character)
	{
		Character->PlayThrowGrenadeMontage();
		AttachActorToLeftHand(EquippedWeapon);
		ShowAttachedGrenade(true);
	}

	Grenades = FMath::Clamp(Grenades - 1, 0, MaxGrenades);
	UpdateHUDGrenades();
}

void UCombatComponent::UpdateHUDGrenades()
{
	Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;

	if (Controller)
	{
		Controller->SetHUDGrenades(Grenades);
	}
}

void UCombatComponent::ShowAttachedGrenade(bool bShowGrenade)
{
	if (Character && Character->GetAttachedGrenade())
	{
		Character->GetAttachedGrenade()->SetVisibility(bShowGrenade);
	}
}

void UCombatComponent::ThrowGrenadeFinished()
{
	CombatState = ECombatState::ECS_Unoccupied;
	AttachActorToRightHand(EquippedWeapon);
}

void UCombatComponent::OnRep_Grenades()
{
	UpdateHUDGrenades();
}


void UCombatComponent::LaunchGrenade()
{
	ShowAttachedGrenade(false);
	if (Character && Character->IsLocallyControlled())
	{
		ServerLaunchGrenade(HitTarget);
	}
}

void UCombatComponent::ServerLaunchGrenade_Implementation(const FVector_NetQuantize& Target)
{
	if (Character && GrenadeClass && Character->GetAttachedGrenade())
	{
		const FVector StartingLocation = Character->GetAttachedGrenade()->GetComponentLocation();
		FVector ToTarget = Target - StartingLocation;
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = Character;
		SpawnParams.Instigator = Character;
		UWorld* World = GetWorld();
		if (World)
		{
			World->SpawnActor<AProjectile>(
				GrenadeClass,
				StartingLocation,
				ToTarget.Rotation(),
				SpawnParams
				);
		}
	}
}

void UCombatComponent::TraceUnderCrosshairs(FHitResult& TraceHitResult)
{
	FVector2D ViewportSize;
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}

	FVector2D CrosshairLocation(ViewportSize.X / 2.f, ViewportSize.Y / 2.f);
	FVector CrosshairWorldPosition;
	FVector CrosshairWorldDirection;
	bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(
		UGameplayStatics::GetPlayerController(this, 0),
		CrosshairLocation,
		CrosshairWorldPosition,
		CrosshairWorldDirection
	);

	if (bScreenToWorld)
	{
		FVector Start = CrosshairWorldPosition;

		if (Character)
		{
			float DistanceToCharacter = (Character->GetActorLocation() - Start).Size();
			Start += CrosshairWorldDirection * (DistanceToCharacter + 100.f);
		}

		FVector End = Start + CrosshairWorldDirection * TRACE_LENGTH;
		bool bHit = GetWorld()->LineTraceSingleByChannel(
			TraceHitResult,
			Start,
			End,
			ECollisionChannel::ECC_Visibility
		);

		// Do we need this?
		if (!TraceHitResult.bBlockingHit)
		{
			TraceHitResult.ImpactPoint = End;
		}

		if (TraceHitResult.GetActor() && TraceHitResult.GetActor()->Implements<UInteractWithCrosshairsInterface>())
		{
			HUDPackage.CrosshairsColour = FLinearColor::Red;
		}
		else
		{
			HUDPackage.CrosshairsColour = FLinearColor::White;
		}
	}
}

bool UCombatComponent::CanFire()
{
	if (EquippedWeapon == nullptr)
	{
		return false;
	}

	if (!EquippedWeapon->GetIsEmpty() && bCanFire && CombatState == ECombatState::ECS_Reloading && EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Shotgun)
	{
		return true;
	}

	return !EquippedWeapon->GetIsEmpty() && bCanFire && CombatState == ECombatState::ECS_Unoccupied;
}

void UCombatComponent::OnRep_CarriedAmmo()
{
	Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;

	if (Controller)
	{
		Controller->SetHUDCarriedAmmo(CarriedAmmo);
	}

	bool bJumpToShotgunEnd = CombatState == ECombatState::ECS_Reloading &&
		EquippedWeapon != nullptr &&
		EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Shotgun &&
		CarriedAmmo <= 0;

	if (bJumpToShotgunEnd)
	{
		JumpToShotgunEnd();
	}
}

void UCombatComponent::InitializeCarriedAmmo()
{
	CarriedAmmoMap.Emplace(EWeaponType::EWT_AssaultRifle, StartingARAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_RocketLauncher, StartingRocketAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_Pistol, StartingPistolAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_SubmachineGun, StartingSMGAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_Shotgun, StartingShotgunAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_SniperRifle, StartingSniperAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_GrenadeLauncher, StartingGrenadeLauncherAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_MAX, 0);

}

void UCombatComponent::HandleRoundEnd()
{
	EquippedWeapon = nullptr;
}