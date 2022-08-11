// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Blaster/BlasterTypes/TurningInPlace.h"
#include "Blaster/Interfaces/InteractWithCrosshairsInterface.h"
#include "Components/TimelineComponent.h"
#include "Blaster/BlasterTypes/CombatState.h"
#include "BlasterCharacter.generated.h"


UCLASS()
class BLASTER_API ABlasterCharacter : public ACharacter, public IInteractWithCrosshairsInterface
{
	GENERATED_BODY()

public:
	ABlasterCharacter();

	virtual void Tick(float DeltaTime) override;
	virtual void FellOutOfWorld(const class UDamageType& dmgType) override;
	virtual void Destroyed() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostInitializeComponents() override;
	virtual void OnRep_ReplicatedMovement() override;

	void PlayFireMontage(bool bAiming);
	void PlayReloadMontage();
	void PlayElimMontage();
	void PlayThrowGrenadeMontage();
	void PlaySwapMontage();
	
	void Eliminate();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastEliminate();

	UPROPERTY(Replicated)
	bool bDisableGameplay = false;
	void UpdateHUDHealth();
	void UpdateHUDShield();
	void UpdateHUDAmmo();

	UFUNCTION(BlueprintImplementableEvent)
	void ShowSniperScopeWidget(bool ShowScope);

	void UpdateMovementSpeed();

	void SpawnDefaultWeapon();

	UPROPERTY()
	TMap<FName, class UBoxComponent*> HitCollisionBoxes;

	bool bFinishedSwapping = false;

protected:
	virtual void BeginPlay() override;

	void MoveForward(float Value);
	void MoveRight(float Value);
	void Turn(float Value);
	void LookUp(float Value);
	void EquipButtonPressed();
	void CrouchButtonPressed();
	void AimButtonPressed();
	void AimButtonReleased();
	void FireButtonPressed();
	void FireButtonReleased();
	void ReloadButtonPressed();
	void GrenadeButtonPressed();

	void AimOffset(float DeltaTime);
	void CalculateAO_Pitch();
	void SimProxiesTurn();

	virtual void Jump() override;
	void PlayHitReactMontage();

	UFUNCTION()
	void ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, class AController* InstigatorController, AActor* DamageCauser);
	
	void RotateInPlace(float DeltaTime);

	void DropOrDestroyWeapon(AWeapon* Weapon);
	void DropOrDestroyWeapons();

	// Hit Boxes for Server side rewind
	// Naming deliberately to match character bone names

	UPROPERTY(EditAnywhere)
	UBoxComponent* HitboxHead;
	
	UPROPERTY(EditAnywhere)
	UBoxComponent* HitboxPelvis;

	UPROPERTY(EditAnywhere)
	UBoxComponent* HitboxSpineUpper;

	UPROPERTY(EditAnywhere)
	UBoxComponent* HitboxSpineLower;

	UPROPERTY(EditAnywhere)
	UBoxComponent* HitboxUpperArmLeft;

	UPROPERTY(EditAnywhere)
	UBoxComponent* HitboxUpperArmRight;

	UPROPERTY(EditAnywhere)
	UBoxComponent* HitboxLowerArmLeft;

	UPROPERTY(EditAnywhere)
	UBoxComponent* HitboxLowerArmRight;

	UPROPERTY(EditAnywhere)
	UBoxComponent* HitboxHandLeft;

	UPROPERTY(EditAnywhere)
	UBoxComponent* HitboxHandRight;

	UPROPERTY(EditAnywhere)
	UBoxComponent* HitboxBackpack;

	UPROPERTY(EditAnywhere)
	UBoxComponent* HitboxBlanket;

	UPROPERTY(EditAnywhere)
	UBoxComponent* HitboxThighLeft;

	UPROPERTY(EditAnywhere)
	UBoxComponent* HitboxThighRight;

	UPROPERTY(EditAnywhere)
	UBoxComponent* HitboxCalfLeft;

	UPROPERTY(EditAnywhere)
	UBoxComponent* HitboxCalfRight;
	
	UPROPERTY(EditAnywhere)
	UBoxComponent* HitboxFootLeft;

	UPROPERTY(EditAnywhere)
	UBoxComponent* HitboxFootRight;

private:
	UPROPERTY(VisibleAnywhere, Category = Camera)
	class USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FollowCamera;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UWidgetComponent* OverheadWidget;

	UPROPERTY(ReplicatedUsing = OnRep_OverlappingWeapon)
	class AWeapon* OverlappingWeapon;

	UFUNCTION()
	void OnRep_OverlappingWeapon(AWeapon* LastWeapon);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	class UCombatComponent* CombatComponent;

	UPROPERTY(VisibleAnywhere, Category = Buff, meta = (AllowPrivateAccess = "true"))
	class UBuffComponent* BuffComponent;

	UPROPERTY(VisibleAnywhere, Category = LagCompensation, meta = (AllowPrivateAccess = "true"))
	class ULagCompensationComponent* LagCompensationComponent;

	UFUNCTION(Server, Reliable)
	void ServerEquipButtonPressed();

	UPROPERTY(EditAnywhere)
	float BaseWalkSpeed = 600.f;

	UPROPERTY(EditAnywhere)
	float AimWalkSpeed = 350.f;

	UPROPERTY(EditAnywhere)
	float CrouchWalkSpeed = 250.f;

	float AO_Yaw;
	float InterpAO_Yaw;
	float AO_Pitch;
	FRotator StartingAimRotation;

	ETurningInPlace TurningInPlace;
	void TurnInPlace(float DeltaTime);

	/*
	* Animation montages
	*/

	UPROPERTY(EditAnywhere, Category = Combat)
	class UAnimMontage* FireWeaponMontage;

	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* ReloadMontage;

	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* HitReactMontage;	

	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* ElimMontage;

	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* ThrowGrenadeMontage;

	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* SwapMontage;

	void HideCharacterIfCameraClose();
	void ToggleWeaponsIfCameraClose(bool bShowWeapons);

	UPROPERTY(EditAnywhere)
	float CameraHideCharacterThreshold = 200.f;

	bool bRotateRootBone;
	float TurnThreshold = 0.5f;
	FRotator ProxyRotationLastFrame;
	FRotator ProxyRotation;
	float ProxyYaw;
	float TimeSinceLastMovementReplication;
	float CalculateSpeed();

	/*
	* Player Health
	*/

	UPROPERTY(EditAnywhere, Category = "Player Stats")
	float MaxHealth = 100.f;

	UPROPERTY(ReplicatedUsing = OnRep_Health, VisibleAnywhere, Category = "Player Stats")
	float Health = 100.f;

	UFUNCTION()
	void OnRep_Health(float LastHealth);

	UPROPERTY(EditAnywhere, Category = "Player Stats")
	float MaxShield = 100.f;

	UPROPERTY(ReplicatedUsing = OnRep_Shield, VisibleAnywhere, Category = "Player Stats")
	float Shield = 100.f;

	UFUNCTION()
	void OnRep_Shield(float LastShield);

	UPROPERTY()
	class ABlasterPlayerController* BlasterPlayerController;
	
	void PollInit();

	UPROPERTY()
	class ABlasterPlayerState* BlasterPlayerState;

	bool bEliminated = false;

	UPROPERTY(EditDefaultsOnly)
	float EliminateDelay = 3.f;
	
	FTimerHandle EliminateTimer;

	void EliminateTimerFinished();

	/*
		Dissolve effect
	*/

	UPROPERTY(VisibleAnywhere)
	UTimelineComponent* DissolveTimeline;
	FOnTimelineFloat DissolveTrack;

	void StartDissolve();

	UFUNCTION()
	void UpdateDissolveMaterial(float DissolveValue);

	UPROPERTY(EditAnywhere)
	UCurveFloat* DissolveCurve;

	//Dynamic instance that we can change at runtime
	UPROPERTY(VisibleAnywhere, Category = Elimination)
	UMaterialInstanceDynamic* DynamicDissolveMaterialInstance;

	//MI set on the blueprint, used with the dynamic MI
	UPROPERTY(EditAnywhere, Category = Elimination)
	UMaterialInstance* DissolveMaterialInstance;

	/*
	 Elim Bot
	*/
	UPROPERTY(EditAnywhere)
	UParticleSystem* ElimBotEffect;

	UPROPERTY(VisibleAnywhere)
	UParticleSystemComponent* ElimBotComponent;

	UPROPERTY(EditAnywhere)
	class USoundCue* ElimBotSound;

	// Grenade
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* AttachedGrenade;

	UPROPERTY(EditAnywhere)
	TSubclassOf<AWeapon> DefaultWeaponClass;

public:	
	void SetOverlappingWeapon(AWeapon* Weapon);
	bool IsWeaponEquipped() const;
	bool IsAiming() const;
	FORCEINLINE float GetAO_Yaw() const { return AO_Yaw; }
	FORCEINLINE float GetAO_Pitch() const { return AO_Pitch; }
	FORCEINLINE ETurningInPlace GetTurningInPlace() const { return TurningInPlace; }
	FORCEINLINE bool ShouldRotateRootBone() const { return bRotateRootBone; }

	AWeapon* GetEquippedWeapon();

	FVector GetHitTarget() const;

	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	FORCEINLINE bool IsEliminated() const { return bEliminated; }

	FORCEINLINE float GetHealth() const { return Health; }
	FORCEINLINE float GetMaxHealth() const { return MaxHealth; }
	FORCEINLINE float GetShield() const { return Shield; }
	FORCEINLINE float GetMaxShield() const { return MaxShield; }
	ECombatState GetCombatState() const;
	FORCEINLINE UCombatComponent* GetCombat() const { return CombatComponent; }
	FORCEINLINE UBuffComponent* GetBuffComponent() const { return BuffComponent; }
	FORCEINLINE ULagCompensationComponent* GetLagCompensation() const { return LagCompensationComponent; }
	FORCEINLINE bool GetDisableGameplay() const { return bDisableGameplay; }
	FORCEINLINE UAnimMontage* GetReloadAnimMontage() const { return ReloadMontage; }
	FORCEINLINE UStaticMeshComponent* GetAttachedGrenade() const { return AttachedGrenade; }
	FORCEINLINE float GetBaseWalkSpeed() const { return BaseWalkSpeed; }
	FORCEINLINE void SetHealth(float Amount) { Health = Amount; }
	FORCEINLINE void SetShield(float Amount) { Shield = Amount; }
	bool IsLocallyReloading() const;
};

