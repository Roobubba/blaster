// Fill out your copyright notice in the Description page of Project Settings.

#include "BlasterCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/WidgetComponent.h"
#include "Net/UnrealNetwork.h"
#include "Blaster/Weapon/Weapon.h"
#include "Blaster/BlasterComponents/CombatComponent.h"
#include "Blaster/BlasterComponents/BuffComponent.h"
#include "Blaster/BlasterComponents/LagCompensationComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "BlasterAnimInstance.h"
#include "Blaster/Blaster.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Blaster/GameMode/BlasterGameMode.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "Particles/ParticleSystemComponent.h"
#include "Blaster/PlayerState/BlasterPlayerState.h"
#include "Blaster/BlasterTypes/WeaponType.h"
#include "Components/BoxComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Blaster/GameState/BlasterGameState.h"
#include "Blaster/PlayerStart/TeamPlayerStart.h"

ABlasterCharacter::ABlasterCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetMesh());
	CameraBoom->TargetArmLength = 600.f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;

	OverheadWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverheadWidget"));
	OverheadWidget->SetupAttachment(RootComponent);

	CombatComponent = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	CombatComponent->SetIsReplicated(true);

	BuffComponent = CreateDefaultSubobject<UBuffComponent>(TEXT("BuffComponent"));
	BuffComponent->SetIsReplicated(true);

	LagCompensationComponent = CreateDefaultSubobject<ULagCompensationComponent>(TEXT("LagCompensationComponent"));

	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;

	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
	GetMesh()->SetCollisionObjectType(ECC_SkeletalMesh);

	GetCharacterMovement()->RotationRate = FRotator(0.f, 0.f, 850.f);

	TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	NetUpdateFrequency = 66.f;
	MinNetUpdateFrequency = 33.f;

	DissolveTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("TimelineComponent"));

	AttachedGrenade = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Attached Grenade"));
	AttachedGrenade->SetupAttachment(GetMesh(), FName("GrenadeSocket"));
	AttachedGrenade->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
	GetCharacterMovement()->MaxWalkSpeedCrouched = CrouchWalkSpeed;

	// Hit Boxes

	HitboxHead = CreateDefaultSubobject<UBoxComponent>(TEXT("HitboxHead"));
	HitboxHead->SetupAttachment(GetMesh(), FName("head"));
	HitCollisionBoxes.Add(FName("HitboxHead"), HitboxHead);

	HitboxPelvis = CreateDefaultSubobject<UBoxComponent>(TEXT("HitboxPelvis"));
	HitboxPelvis->SetupAttachment(GetMesh(), FName("pelvis"));
	HitCollisionBoxes.Add(FName("HitboxPelvis"), HitboxPelvis);

	HitboxSpineUpper = CreateDefaultSubobject<UBoxComponent>(TEXT("HitboxSpineUpper"));
	HitboxSpineUpper->SetupAttachment(GetMesh(), FName("spine_02"));
	HitCollisionBoxes.Add(FName("HitboxSpineUpper"), HitboxSpineUpper);

	HitboxSpineLower = CreateDefaultSubobject<UBoxComponent>(TEXT("HitboxSpineLower"));
	HitboxSpineLower->SetupAttachment(GetMesh(), FName("spine_03"));
	HitCollisionBoxes.Add(FName("HitboxSpineLower"), HitboxSpineLower);
		
	HitboxUpperArmLeft = CreateDefaultSubobject<UBoxComponent>(TEXT("HitboxUpperArmLeft"));
	HitboxUpperArmLeft->SetupAttachment(GetMesh(), FName("upperarm_l"));
	HitCollisionBoxes.Add(FName("HitboxUpperArmLeft"), HitboxUpperArmLeft);

	HitboxUpperArmRight = CreateDefaultSubobject<UBoxComponent>(TEXT("HitboxUpperArmRight"));
	HitboxUpperArmRight->SetupAttachment(GetMesh(), FName("upperarm_r"));
	HitCollisionBoxes.Add(FName("HitboxUpperArmRight"), HitboxUpperArmRight);

	HitboxLowerArmLeft = CreateDefaultSubobject<UBoxComponent>(TEXT("HitboxLowerArmLeft"));
	HitboxLowerArmLeft->SetupAttachment(GetMesh(), FName("lowerarm_l"));
	HitCollisionBoxes.Add(FName("HitboxLowerArmLeft"), HitboxLowerArmLeft);

	HitboxLowerArmRight = CreateDefaultSubobject<UBoxComponent>(TEXT("HitboxLowerArmRight"));
	HitboxLowerArmRight->SetupAttachment(GetMesh(), FName("lowerarm_r"));
	HitCollisionBoxes.Add(FName("HitboxLowerArmRight"), HitboxLowerArmRight);

	HitboxHandLeft = CreateDefaultSubobject<UBoxComponent>(TEXT("HitboxHandLeft"));
	HitboxHandLeft->SetupAttachment(GetMesh(), FName("hand_l"));
	HitCollisionBoxes.Add(FName("HitboxHandLeft"), HitboxHandLeft);

	HitboxHandRight = CreateDefaultSubobject<UBoxComponent>(TEXT("HitboxHandRight"));
	HitboxHandRight->SetupAttachment(GetMesh(), FName("hand_r"));
	HitCollisionBoxes.Add(FName("HitboxHandRight"), HitboxHandRight);

	HitboxBackpack = CreateDefaultSubobject<UBoxComponent>(TEXT("HitboxBackpack"));
	HitboxBackpack->SetupAttachment(GetMesh(), FName("backpack"));
	HitCollisionBoxes.Add(FName("HitboxBackpack"), HitboxBackpack);

	HitboxBlanket = CreateDefaultSubobject<UBoxComponent>(TEXT("HitboxBlanket"));
	HitboxBlanket->SetupAttachment(GetMesh(), FName("backpack"));
	HitCollisionBoxes.Add(FName("HitboxBlanket"), HitboxBlanket);

	HitboxThighLeft = CreateDefaultSubobject<UBoxComponent>(TEXT("HitboxThighLeft"));
	HitboxThighLeft->SetupAttachment(GetMesh(), FName("thigh_l"));
	HitCollisionBoxes.Add(FName("HitboxThighLeft"), HitboxThighLeft);

	HitboxThighRight = CreateDefaultSubobject<UBoxComponent>(TEXT("HitboxThighRight"));
	HitboxThighRight->SetupAttachment(GetMesh(), FName("thigh_r"));
	HitCollisionBoxes.Add(FName("HitboxThighRight"), HitboxThighRight);

	HitboxCalfLeft = CreateDefaultSubobject<UBoxComponent>(TEXT("HitboxCalfLeft"));
	HitboxCalfLeft->SetupAttachment(GetMesh(), FName("calf_l"));
	HitCollisionBoxes.Add(FName("HitboxCalfLeft"), HitboxCalfLeft);

	HitboxCalfRight = CreateDefaultSubobject<UBoxComponent>(TEXT("HitboxCalfRight"));
	HitboxCalfRight->SetupAttachment(GetMesh(), FName("calf_r"));
	HitCollisionBoxes.Add(FName("HitboxCalfRight"), HitboxCalfRight);

	HitboxFootLeft = CreateDefaultSubobject<UBoxComponent>(TEXT("HitboxFootLeft"));
	HitboxFootLeft->SetupAttachment(GetMesh(), FName("foot_l"));
	HitCollisionBoxes.Add(FName("HitboxFootLeft"), HitboxFootLeft);

	HitboxFootRight = CreateDefaultSubobject<UBoxComponent>(TEXT("HitboxFootRight"));
	HitboxFootRight->SetupAttachment(GetMesh(), FName("foot_r"));
	HitCollisionBoxes.Add(FName("HitboxFootRight"), HitboxFootRight);

	for (auto Box : HitCollisionBoxes)
	{
		if (Box.Value)
		{
			Box.Value->SetCollisionObjectType(ECC_HitBox);
			Box.Value->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			Box.Value->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);
			Box.Value->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
}

void ABlasterCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (CombatComponent)
	{
		CombatComponent->Character = this;
	}

	if (BuffComponent)
	{
		BuffComponent->Character = this;

		if (GetCharacterMovement())
		{
			BuffComponent->SetInitialJumpVerticalVelocity(GetCharacterMovement()->JumpZVelocity);
		}
	}

	if (LagCompensationComponent)
	{
		LagCompensationComponent->Character = this;

		if (Controller)
		{
			LagCompensationComponent->Controller = Cast<ABlasterPlayerController>(Controller);
		}
	}
}

void ABlasterCharacter::Destroyed()
{
	Super::Destroyed();
	
	if (ElimBotComponent)
	{
		ElimBotComponent->DestroyComponent();
	}

	BlasterGameMode = BlasterGameMode == nullptr ? Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this)) : BlasterGameMode;
	bool bMatchNotInProgress = BlasterGameMode && BlasterGameMode->GetMatchState() != MatchState::InProgress;

	if (CombatComponent && CombatComponent->EquippedWeapon && bMatchNotInProgress)
	{
		CombatComponent->EquippedWeapon->Destroy();
	}
}

void ABlasterCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ABlasterCharacter, OverlappingWeapon, COND_OwnerOnly);
	DOREPLIFETIME(ABlasterCharacter, Health);
	DOREPLIFETIME(ABlasterCharacter, Shield);
	DOREPLIFETIME(ABlasterCharacter, bDisableGameplay);
}

void ABlasterCharacter::OnRep_ReplicatedMovement()
{
	Super::OnRep_ReplicatedMovement();

	SimProxiesTurn();
	TimeSinceLastMovementReplication = 0.f;
}

void ABlasterCharacter::BeginPlay()
{
	Super::BeginPlay();

	SpawnDefaultWeapon();
	UpdateHUDHealth();
	UpdateHUDShield();
	
	if (CombatComponent)
	{
		CombatComponent->UpdateHUDGrenades();
	}

	FollowCamera->PostProcessSettings.bOverride_DepthOfFieldFocalDistance = 1;

	if (HasAuthority())
	{
		OnTakeAnyDamage.AddDynamic(this, &ABlasterCharacter::ReceiveDamage);
	}

	if (AttachedGrenade)
	{
		AttachedGrenade->SetVisibility(false);
	}
}

void ABlasterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	RotateInPlace(DeltaTime);
	HideCharacterIfCameraClose();
	PollInit();
}

void ABlasterCharacter::RotateInPlace(float DeltaTime)
{
	if (CombatComponent && CombatComponent->bHoldingTheFlagon)
	{
		bUseControllerRotationYaw = false;
		GetCharacterMovement()->bOrientRotationToMovement = true;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;		
		return;
	}

	if (bDisableGameplay) 
	{
		bUseControllerRotationYaw = false;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;		
		return;
	}

	if (GetLocalRole() > ENetRole::ROLE_SimulatedProxy && IsLocallyControlled())
	{
		AimOffset(DeltaTime);
	}
	else
	{
		TimeSinceLastMovementReplication += DeltaTime;
		if (TimeSinceLastMovementReplication > 0.25f)
		{
			OnRep_ReplicatedMovement();
		}

		CalculateAO_Pitch();
	}

	if (CombatComponent)
	{
		FollowCamera->PostProcessSettings.DepthOfFieldFocalDistance = (CombatComponent->HitTarget - FollowCamera->GetComponentLocation()).Size();
	}
}

void ABlasterCharacter::Eliminate(bool bPlayerLeftGame)
{
	DropOrDestroyWeapons();
	MulticastEliminate(bPlayerLeftGame);
}

void ABlasterCharacter::MulticastEliminate_Implementation(bool bPlayerLeftGame)
{
	bLeftGame = bPlayerLeftGame;

	if (BlasterPlayerController)
	{
		BlasterPlayerController->SetHUDWeaponType(EWeaponType::EWT_MAX);
		BlasterPlayerController->SetHUDCarriedAmmo(0);
		BlasterPlayerController->SetHUDWeaponAmmo(0);
		BlasterPlayerController->RemoveAllPickupTexts();
	}

	bEliminated = true;
	PlayElimMontage();

	if (DissolveMaterialInstance)
	{
		DynamicDissolveMaterialInstance = UMaterialInstanceDynamic::Create(DissolveMaterialInstance, this);
		GetMesh()->SetMaterial(0, DynamicDissolveMaterialInstance);
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), 0.f);
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Glow"), 200.f);
	}

	StartDissolve();

	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->StopMovementImmediately();

	bDisableGameplay = true;

	if (CombatComponent)
	{
		if (CombatComponent->bHoldingTheFlagon && CombatComponent->TheFlagonWeapon)
		{
			CombatComponent->TheFlagonWeapon->Dropped();
		}

		CombatComponent->FireButtonPressed(false);
	}

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	if (ElimBotEffect)
	{
		FVector ElimBotSpawnPoint(GetActorLocation().X, GetActorLocation().Y, GetActorLocation().Z + 200.f);
		ElimBotComponent = UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			ElimBotEffect,
			ElimBotSpawnPoint,
			GetActorRotation()
			);
	}

	if (ElimBotSound)
	{
		UGameplayStatics::SpawnSoundAtLocation(
			this,
			ElimBotSound,
			GetActorLocation()
		);
	}

	bool bHideSniperScope = IsLocallyControlled() && 
		CombatComponent && 
		CombatComponent->bAiming && 
		CombatComponent->EquippedWeapon && 
		CombatComponent->EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SniperRifle;

	if (bHideSniperScope)
	{
		ShowSniperScopeWidget(false);
	}

	if (CrownComponent)
	{
		CrownComponent->DestroyComponent();
	}
	
	GetWorldTimerManager().SetTimer(
		EliminateTimer,
		this,
		&ABlasterCharacter::EliminateTimerFinished,
		EliminateDelay
	);
}

void ABlasterCharacter::EliminateTimerFinished()
{
	BlasterGameMode = BlasterGameMode == nullptr ? GetWorld()->GetAuthGameMode<ABlasterGameMode>() : BlasterGameMode;
	if (BlasterGameMode && !bLeftGame)
	{
		BlasterGameMode->RequestRespawn(this, Controller);
	}
	
	if (bLeftGame && IsLocallyControlled())
	{
		OnLeftGame.Broadcast();
	}
}

void ABlasterCharacter::ServerLeaveGame_Implementation()
{
	BlasterGameMode = BlasterGameMode == nullptr ? Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this)) : BlasterGameMode;
	BlasterPlayerState = BlasterPlayerState == nullptr ? GetPlayerState<ABlasterPlayerState>() : BlasterPlayerState;

	if (BlasterGameMode && BlasterPlayerState)
	{
		BlasterGameMode->PlayerLeftGame(BlasterPlayerState);
	}
}

void ABlasterCharacter::MulticastGainedTheLead_Implementation()
{
	if (CrownSystem == nullptr)
	{
		return;
	}

	if (CrownComponent == nullptr)
	{
		FFXSystemSpawnParameters Params = FFXSystemSpawnParameters();
		Params.AttachPointName = FName("CrownSocket");
		Params.AttachToComponent = GetMesh();
		Params.bAutoDestroy = false;
		Params.LocationType = EAttachLocation::KeepRelativeOffset;
		Params.Location = FVector(0.f, 0.f, 0.f);
		Params.Rotation = FRotator(0.f, 0.f, 0.f);
		Params.SystemTemplate = CrownSystem;
		Params.Scale = FVector(0.75f, 0.75f, 0.75f);
		CrownComponent = UNiagaraFunctionLibrary::SpawnSystemAttachedWithParams(Params);

		//CrownComponent = UNiagaraFunctionLibrary::SpawnSystemAttached
		//(
		//	CrownSystem,
		//	GetMesh(),
		//	FName("CrownSocket"),
		//	GetActorLocation() + FVector(0.f, 0.f, 115.f),
		//	GetActorRotation(),
		//	EAttachLocation::KeepWorldPosition,
		//	false
		//);
	}

	if (CrownComponent)
	{
		CrownComponent->Activate();
	}
}

void ABlasterCharacter::MulticastLostTheLead_Implementation()
{
	if (CrownComponent)
	{
		CrownComponent->DestroyComponent();
	}
}

void ABlasterCharacter::DropOrDestroyWeapons()
{
	if (CombatComponent && CombatComponent->EquippedWeapon)
	{
		DropOrDestroyWeapon(CombatComponent->EquippedWeapon);
	}

	if (CombatComponent && CombatComponent->SecondaryWeapon)
	{
		DropOrDestroyWeapon(CombatComponent->SecondaryWeapon);
	}

	if (CombatComponent && CombatComponent->TheFlagonWeapon)
	{
		CombatComponent->TheFlagonWeapon->Dropped();
		CombatComponent->bHoldingTheFlagon = false;
	}
}

void ABlasterCharacter::DropOrDestroyWeapon(AWeapon* Weapon)
{
	if (Weapon == nullptr)
	{
		return;
	}

	if (Weapon->bDestroyWeapon)
	{
		Weapon->Destroy();
	}
	else
	{
		Weapon->Dropped();
	}
}

void ABlasterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &ThisClass::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ThisClass::MoveRight);
	PlayerInputComponent->BindAxis("Turn", this, &ThisClass::Turn);
	PlayerInputComponent->BindAxis("LookUp", this, &ThisClass::LookUp);
	
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ABlasterCharacter::Jump);
	PlayerInputComponent->BindAction("Equip", IE_Pressed, this, &ABlasterCharacter::EquipButtonPressed);
	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &ABlasterCharacter::CrouchButtonPressed);
	PlayerInputComponent->BindAction("Aim", IE_Pressed, this, &ABlasterCharacter::AimButtonPressed);
	PlayerInputComponent->BindAction("Aim", IE_Released, this, &ABlasterCharacter::AimButtonReleased);
	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &ABlasterCharacter::FireButtonPressed);
	PlayerInputComponent->BindAction("Fire", IE_Released, this, &ABlasterCharacter::FireButtonReleased);
	PlayerInputComponent->BindAction("Reload", IE_Pressed, this, &ABlasterCharacter::ReloadButtonPressed);
	PlayerInputComponent->BindAction("ThrowGrenade", IE_Pressed, this, &ABlasterCharacter::GrenadeButtonPressed);
	PlayerInputComponent->BindAction("ChatInput", IE_Pressed, this, &ABlasterCharacter::ChatInputButtonPressed);
}

void ABlasterCharacter::FellOutOfWorld(const UDamageType &dmgType)
{
	// Not calling Super, we will handle the death here manually

	if (HasAuthority())
	{
		UGameplayStatics::ApplyDamage(this, 10000.f, Controller, this, UDamageType::StaticClass());
	}
}

void ABlasterCharacter::ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, class AController* InstigatorController, AActor* DamageCauser)
{
	if (bEliminated || !GetWorld())
	{
		return;
	}

	BlasterGameMode = BlasterGameMode == nullptr ? GetWorld()->GetAuthGameMode<ABlasterGameMode>() : BlasterGameMode;
	if (BlasterGameMode)
	{		
		Damage = BlasterGameMode->CalculateDamage(InstigatorController, Controller, Damage);
		float DamageToHealth = FMath::Max(0.f, Damage - Shield);
		Shield = FMath::Clamp(Shield - Damage, 0.f, MaxShield);	
		Health = FMath::Clamp(Health - DamageToHealth, 0.f, MaxHealth);

		UpdateHUDHealth();
		UpdateHUDShield();
		
		if (BuffComponent)
		{
			BuffComponent->UpdateHUDHealing();
			BuffComponent->UpdateHUDShieldRegen();
		}

		if (Health <= 0.f)
		{
			BlasterPlayerController = BlasterPlayerController == nullptr ? Cast<ABlasterPlayerController>(Controller) : BlasterPlayerController;
			ABlasterPlayerController* AttackerController = Cast<ABlasterPlayerController>(InstigatorController);

			BlasterGameMode->PlayerEliminated(this, BlasterPlayerController, AttackerController);
		}
		else
		{
			PlayHitReactMontage();
		}
	}
}

void ABlasterCharacter::PlayFireMontage(bool bAiming)
{
	if (CombatComponent == nullptr || CombatComponent->EquippedWeapon == nullptr)
	{
		return;
	}

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (AnimInstance && FireWeaponMontage)
	{
		AnimInstance->Montage_Play(FireWeaponMontage);
		FName SectionName;
		SectionName = bAiming ? FName("RifleAim") : FName("RifleHip");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABlasterCharacter::PlayReloadMontage()
{
	if (CombatComponent == nullptr || CombatComponent->EquippedWeapon == nullptr)
	{
		return;
	}

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (AnimInstance && ReloadMontage)
	{
		AnimInstance->Montage_Play(ReloadMontage);
		FName SectionName;

		switch (CombatComponent->EquippedWeapon->GetWeaponType())
		{
			case EWeaponType::EWT_AssaultRifle:
				SectionName = FName("Rifle");
				break;
			case EWeaponType::EWT_SniperRifle:
				SectionName = FName("SniperRifle");
				break;
			case EWeaponType::EWT_RocketLauncher:
				SectionName = FName("RocketLauncher");
				break;
			case EWeaponType::EWT_Pistol:
				SectionName = FName("Pistol");
				break;
			case EWeaponType::EWT_SubmachineGun:
				SectionName = FName("Pistol");
				break;
			case EWeaponType::EWT_Shotgun:
				SectionName = FName("Shotgun");
				break;
			case EWeaponType::EWT_GrenadeLauncher:
				SectionName = FName("GrenadeLauncher");
				break;
		}

		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABlasterCharacter::PlayElimMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (AnimInstance && ElimMontage)
	{
		AnimInstance->Montage_Play(ElimMontage);
	}
}

void ABlasterCharacter::PlayHitReactMontage()
{
	if (CombatComponent == nullptr || CombatComponent->EquippedWeapon == nullptr)
	{
		return;
	}

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (AnimInstance && HitReactMontage)
	{
		AnimInstance->Montage_Play(HitReactMontage);
		FName SectionName("FromFront");

		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABlasterCharacter::PlayThrowGrenadeMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (AnimInstance && ThrowGrenadeMontage)
	{
		AnimInstance->Montage_Play(ThrowGrenadeMontage);
	}
}

void ABlasterCharacter::PlaySwapMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (AnimInstance && SwapMontage)
	{
		AnimInstance->Montage_Play(SwapMontage);
	}
}

void ABlasterCharacter::EquipButtonPressed()
{
	if (bDisableGameplay) return;

	if (CombatComponent)
	{
		if (CombatComponent->bHoldingTheFlagon)
		{
			return;
		}

		if (CombatComponent->CombatState == ECombatState::ECS_Unoccupied)
		{
			ServerEquipButtonPressed();
		}

		bool bSwap = CombatComponent->ShouldSwapWeapons() &&
			!HasAuthority() && 
			CombatComponent->CombatState == ECombatState::ECS_Unoccupied &&
			OverlappingWeapon == nullptr;

		if (bSwap)
		{
			PlaySwapMontage();
			CombatComponent->CombatState = ECombatState::ECS_SwappingWeapon;
			bFinishedSwapping = false;
		}
	}
}

void ABlasterCharacter::ServerEquipButtonPressed_Implementation()
{
	if (CombatComponent)
	{
		if (OverlappingWeapon)
		{
			CombatComponent->EquipWeapon(OverlappingWeapon);
		}
		else if (CombatComponent->ShouldSwapWeapons())
		{
			CombatComponent->SwapWeapons();
		}
	}
}

void ABlasterCharacter::UpdateMovementSpeed()
{
	float BuffMultiplier = 1.f;
	if (BuffComponent)
	{
		BuffMultiplier = BuffComponent->GetSpeedMultiplier();
	}
	
	if (GetMovementComponent())
	{
		if (IsHoldingTheFlagon())
		{
			GetCharacterMovement()->MaxWalkSpeed = BuffMultiplier * AimWalkSpeed;
		}
		else
		{
			GetCharacterMovement()->MaxWalkSpeed = IsAiming() ? BuffMultiplier * AimWalkSpeed : BuffMultiplier * BaseWalkSpeed;
		}

		GetCharacterMovement()->MaxWalkSpeedCrouched = BuffMultiplier * CrouchWalkSpeed;
	}
}

void ABlasterCharacter::CrouchButtonPressed()
{
	if (bDisableGameplay) return;

	if (CombatComponent && CombatComponent->bHoldingTheFlagon)
	{
		return;
	}

	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Crouch();
	}
}

void ABlasterCharacter::AimButtonPressed()
{
	if (bDisableGameplay) return;

	if (CombatComponent && !CombatComponent->bHoldingTheFlagon)
	{	
		CombatComponent->SetAiming(true);
	}
}

void ABlasterCharacter::AimButtonReleased()
{
	if (bDisableGameplay) return;

	if (CombatComponent)
	{
		CombatComponent->SetAiming(false);	
	}
}

void ABlasterCharacter::FireButtonPressed()
{
	if (bDisableGameplay) return;

	if (CombatComponent && !CombatComponent->bHoldingTheFlagon)
	{
		CombatComponent->FireButtonPressed(true);	
	}
}

void ABlasterCharacter::FireButtonReleased()
{
	if (bDisableGameplay) return;

	if (CombatComponent && !CombatComponent->bHoldingTheFlagon)
	{
		CombatComponent->FireButtonPressed(false);	
	}
}

void ABlasterCharacter::ReloadButtonPressed()
{
	if (bDisableGameplay) return;

	if (CombatComponent && !CombatComponent->bHoldingTheFlagon)
	{
		CombatComponent->Reload();	
	}
}

void ABlasterCharacter::GrenadeButtonPressed()
{
	if (CombatComponent && !CombatComponent->bHoldingTheFlagon)
	{
		CombatComponent->ThrowGrenade();
	}
}

void ABlasterCharacter::ChatInputButtonPressed()
{
	if (BlasterPlayerController)
	{
		BlasterPlayerController->ToggleChatInput();
	}
}

void ABlasterCharacter::MoveForward(float Value)
{
	if (bDisableGameplay) return;

	if (Controller != nullptr && Value != 0.f)
	{
		const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
		const FVector Direction(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X));
		AddMovementInput(Direction, Value);
	}
}

void ABlasterCharacter::MoveRight(float Value)
{
	if (bDisableGameplay) return;

	if (Controller != nullptr && Value != 0.f)
	{
		const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
		const FVector Direction(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y));
		AddMovementInput(Direction, Value);
	}
}

void ABlasterCharacter::Turn(float Value)
{
	AddControllerYawInput(Value);
}

void ABlasterCharacter::LookUp(float Value)
{
	AddControllerPitchInput(Value);
}

void ABlasterCharacter::HideCharacterIfCameraClose()
{
	if (!IsLocallyControlled())
	{
		return;
	}

	if ((FollowCamera->GetComponentLocation() - GetActorLocation()).Size() < CameraHideCharacterThreshold)
	{
		GetMesh()->SetVisibility(false);
		ToggleWeaponsIfCameraClose(true);
		if (CrownComponent)
		{
			CrownComponent->bHiddenInGame = true;
		}
	}
	else
	{
		GetMesh()->SetVisibility(true);
		ToggleWeaponsIfCameraClose(false);
		if (CrownComponent)
		{
			CrownComponent->bHiddenInGame = false;
		}
	}
}

void ABlasterCharacter::ToggleWeaponsIfCameraClose(bool bShowWeapons)
{
	if (CombatComponent)
	{
		if (CombatComponent->bHoldingTheFlagon && CombatComponent->GetFlagonStaticMesh())
		{
			CombatComponent->GetFlagonStaticMesh()->bOwnerNoSee = bShowWeapons;
		}

		if (CombatComponent->EquippedWeapon)
		{
			if (CombatComponent->EquippedWeapon->GetWeaponMesh())
			{
				CombatComponent->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = bShowWeapons;
			}
		}

		if (CombatComponent->SecondaryWeapon && CombatComponent->SecondaryWeapon->GetWeaponMesh())
		{
			CombatComponent->SecondaryWeapon->GetWeaponMesh()->bOwnerNoSee = bShowWeapons;
		}
	}
}

void ABlasterCharacter::SpawnDefaultWeapon()
{
	BlasterGameMode = BlasterGameMode == nullptr ? Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this)) : BlasterGameMode;
	if (BlasterGameMode)
	{
		UWorld* World = GetWorld();
		if (World && !bEliminated && DefaultWeaponClass)
		{
			AWeapon* StartingWeapon = World->SpawnActor<AWeapon>(DefaultWeaponClass);
			StartingWeapon->bDestroyWeapon = true;
			if (CombatComponent)
			{
				CombatComponent->EquipWeapon(StartingWeapon);
			}
		}
	}
}

void ABlasterCharacter::SetOverlappingWeapon(AWeapon* Weapon)
{
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(false);
	}

	OverlappingWeapon = Weapon;

	if (IsLocallyControlled())
	{
		if (OverlappingWeapon)
		{
			OverlappingWeapon->ShowPickupWidget(true);
		}
	}
}

void ABlasterCharacter::OnRep_OverlappingWeapon(AWeapon* LastWeapon)
{
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(true);
	}

	if (LastWeapon)
	{
		LastWeapon->ShowPickupWidget(false);
	}
}

float ABlasterCharacter::CalculateSpeed()
{
    FVector Velocity = GetVelocity();
    Velocity.Z = 0.f;
    return Velocity.Size();
}

void ABlasterCharacter::AimOffset(float DeltaTime)
{
	if (CombatComponent && CombatComponent->EquippedWeapon == nullptr)
	{
		return;
	}

	float Speed = CalculateSpeed();
	bool bIsInAir = GetCharacterMovement()->IsFalling();

	if (Speed == 0.f && !bIsInAir)
	{
		bRotateRootBone = true;
		FRotator CurrentAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(CurrentAimRotation, StartingAimRotation);
		AO_Yaw = DeltaAimRotation.Yaw;
		if (TurningInPlace == ETurningInPlace::ETIP_NotTurning)
		{
			InterpAO_Yaw = AO_Yaw;
		}

		bUseControllerRotationYaw = true;
		TurnInPlace(DeltaTime);
	}

	if (Speed > 0.f || bIsInAir)
	{
		bRotateRootBone = false;
		StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		AO_Yaw = 0.f;
		bUseControllerRotationYaw = true;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	}

	CalculateAO_Pitch();
}

void ABlasterCharacter::CalculateAO_Pitch()
{
	AO_Pitch = GetBaseAimRotation().Pitch;

	if (AO_Pitch > 90.f && !IsLocallyControlled())
	{
		// map decompressed value from 270-360 back to -90-0
		//FVector2D InRange(270.f, 360.f);
		//FVector2D OutRange(-90.f, 0.f);
		//AO_Pitch = FMath::GetMappedRangeValueClamped(InRange, OutRange, AO_Pitch);
		AO_Pitch = FMath::Clamp((AO_Pitch - 360.f), -90.f, 0.f);
	}
}

void ABlasterCharacter::SimProxiesTurn()
{
	if (CombatComponent == nullptr || CombatComponent->EquippedWeapon == nullptr)
	{
		return;
	}

	bRotateRootBone = false;

	if (CalculateSpeed() > 0.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		return;
	}

	ProxyRotationLastFrame = ProxyRotation;
	ProxyRotation = GetActorRotation();
	ProxyYaw = UKismetMathLibrary::NormalizedDeltaRotator(ProxyRotation, ProxyRotationLastFrame).Yaw;

	if (FMath::Abs(ProxyYaw) > TurnThreshold)
	{
		if (ProxyYaw > 0.f)
		{
			TurningInPlace = ETurningInPlace::ETIP_Right;
		}
		else
		{
			TurningInPlace = ETurningInPlace::ETIP_Left;
		}
	}
	else
	{
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	}
}

void ABlasterCharacter::Jump()
{
	if (bDisableGameplay) return;

	if (CombatComponent && CombatComponent->bHoldingTheFlagon)
	{
		return;
	}

	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Super::Jump();
	}
}

void ABlasterCharacter::TurnInPlace(float DeltaTime)
{
	if (AO_Yaw > 90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Right;
	}
	else if (AO_Yaw < -90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Left;
	}

	if (TurningInPlace != ETurningInPlace::ETIP_NotTurning)
	{
		InterpAO_Yaw = FMath::FInterpTo(InterpAO_Yaw, 0.f, DeltaTime, 4.f);
		AO_Yaw = InterpAO_Yaw;
		if (FMath::Abs(AO_Yaw) < 15.f)
		{
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
			StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		}
	}
}

AWeapon* ABlasterCharacter::GetEquippedWeapon()
{
	if (CombatComponent == nullptr)
	{
		return nullptr;
	}
	
	return CombatComponent->EquippedWeapon;
}

FVector ABlasterCharacter::GetHitTarget() const
{
	if (CombatComponent == nullptr)
	{
		return FVector();
	}

	return CombatComponent->HitTarget;
}

void ABlasterCharacter::OnRep_Health(float LastHealth)
{
	UpdateHUDHealth();
	if (BuffComponent)
	{
		BuffComponent->UpdateHUDHealing();
	}	

	if (!bEliminated && Health < LastHealth)
	{
		PlayHitReactMontage();
	}
}

void ABlasterCharacter::UpdateHUDHealth()
{
	BlasterPlayerController = BlasterPlayerController == nullptr ? Cast<ABlasterPlayerController>(Controller) : BlasterPlayerController;
	if (BlasterPlayerController)
	{
		BlasterPlayerController->SetHUDHealth(Health, MaxHealth);
	}
}

void ABlasterCharacter::OnRep_Shield(float LastShield)
{
	UpdateHUDShield();
	if (BuffComponent)
	{
		BuffComponent->UpdateHUDShieldRegen();
	}

	if (!bEliminated && Shield < LastShield)
	{
		PlayHitReactMontage();
	}
}

void ABlasterCharacter::UpdateHUDShield()
{
	BlasterPlayerController = BlasterPlayerController == nullptr ? Cast<ABlasterPlayerController>(Controller) : BlasterPlayerController;
	if (BlasterPlayerController)
	{
		BlasterPlayerController->SetHUDShield(Shield, MaxShield);
	}
}

void ABlasterCharacter::UpdateHUDAmmo()
{
	BlasterPlayerController = BlasterPlayerController == nullptr ? Cast<ABlasterPlayerController>(Controller) : BlasterPlayerController;
	if (BlasterPlayerController && CombatComponent && CombatComponent->EquippedWeapon)
	{
		BlasterPlayerController->SetHUDWeaponAmmo(CombatComponent->EquippedWeapon->GetAmmo());
		BlasterPlayerController->SetHUDCarriedAmmo(CombatComponent->CarriedAmmo);
		BlasterPlayerController->SetHUDGrenades(CombatComponent->GetGrenades());
		BlasterPlayerController->SetHUDWeaponType(CombatComponent->EquippedWeapon->GetWeaponType());
	}
}

void ABlasterCharacter::PollInit()
{
	if (BlasterPlayerState == nullptr)
	{
		BlasterPlayerState = GetPlayerState<ABlasterPlayerState>();
		if (BlasterPlayerState)
		{
			OnPlayerStateInitialized();

			ABlasterGameState* BlasterGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this));
			if (BlasterGameState && BlasterGameState->TopScoringPlayers.Contains(BlasterPlayerState))
			{
				MulticastGainedTheLead();
			}
		}
	}
}

void ABlasterCharacter::OnPlayerStateInitialized()
{
	BlasterPlayerState->AddToScore(0.f);
	BlasterPlayerState->AddToDefeats(0);
	SetTeamColour(BlasterPlayerState->GetTeam());
	SetSpawnPoint();
}

void ABlasterCharacter::SetSpawnPoint()
{
	if (HasAuthority() && BlasterPlayerState->GetTeam() != ETeam::ET_NoTeam)
	{
		TArray<AActor*> PlayerStarts;
		UGameplayStatics::GetAllActorsOfClass(this, ATeamPlayerStart::StaticClass(), PlayerStarts);
		TArray<ATeamPlayerStart*> TeamPlayerStarts;
		for (auto Start : PlayerStarts)
		{
			ATeamPlayerStart* TeamStart = Cast<ATeamPlayerStart>(Start);
			if (TeamStart && TeamStart->Team == BlasterPlayerState->GetTeam())
			{
				TeamPlayerStarts.Add(TeamStart);
			}
		}

		if (TeamPlayerStarts.Num() > 0)
		{
			ATeamPlayerStart* ChosenPlayerStart = TeamPlayerStarts[FMath::RandRange(0, TeamPlayerStarts.Num() - 1)];
			SetActorLocationAndRotation(ChosenPlayerStart->GetActorLocation(), ChosenPlayerStart->GetActorRotation());
		}
	}
}

void ABlasterCharacter::SetTeamColour(ETeam Team)
{
	if (GetMesh() == nullptr || OriginalMaterialInstance == nullptr)
	{
		return;
	}

	switch (Team)
	{
		case ETeam::ET_NoTeam:
			GetMesh()->SetMaterial(0, OriginalMaterialInstance);
			DissolveMaterialInstance = BlueDissolveMaterialInstance;
		break;

		case ETeam::ET_RedTeam:	
			GetMesh()->SetMaterial(0, RedMaterialInstance);
			DissolveMaterialInstance = RedDissolveMaterialInstance;
		break;

		case ETeam::ET_BlueTeam:
			GetMesh()->SetMaterial(0, BlueMaterialInstance);
			DissolveMaterialInstance = BlueDissolveMaterialInstance;
		break;
	}
}

void ABlasterCharacter::StartDissolve()
{
	DissolveTrack.BindDynamic(this, &ABlasterCharacter::UpdateDissolveMaterial);
	if (DissolveCurve && DissolveTimeline)
	{
		DissolveTimeline->AddInterpFloat(DissolveCurve, DissolveTrack);
		DissolveTimeline->Play();
	}
}

void ABlasterCharacter::UpdateDissolveMaterial(float DissolveValue)
{
	if (DynamicDissolveMaterialInstance)
	{
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), DissolveValue);
	}
}

ECombatState ABlasterCharacter::GetCombatState() const
{
	if (CombatComponent == nullptr)
	{
		return ECombatState::ECS_MAX;
	}

	return CombatComponent->CombatState;
}

bool ABlasterCharacter::IsWeaponEquipped() const
{
	return (CombatComponent && CombatComponent->EquippedWeapon);
}

bool ABlasterCharacter::IsAiming() const
{
	return (CombatComponent && CombatComponent->bAiming);
}

bool ABlasterCharacter::IsLocallyReloading() const
{
	return (CombatComponent && CombatComponent->bLocallyReloading);
}

bool ABlasterCharacter::IsHoldingTheFlagon() const
{
	return (CombatComponent && CombatComponent->bHoldingTheFlagon);
}

ETeam ABlasterCharacter::GetTeam()
{
	BlasterPlayerState = BlasterPlayerState == nullptr ? GetPlayerState<ABlasterPlayerState>() : BlasterPlayerState;

	if (BlasterPlayerState == nullptr)
	{
		return ETeam::ET_NoTeam;
	}
	
	return BlasterPlayerState->GetTeam();
}

void ABlasterCharacter::SetHoldingTheFlagon(bool bHolding)
{
	if (CombatComponent == nullptr)
	{
		return;
	}

	CombatComponent->bHoldingTheFlagon = bHolding;
}

void ABlasterCharacter::FlagonDropped()
{
	if (CombatComponent && CombatComponent->EquippedWeapon)
	{
		bUseControllerRotationYaw = true;
		GetCharacterMovement()->bOrientRotationToMovement = false;
	}
}
