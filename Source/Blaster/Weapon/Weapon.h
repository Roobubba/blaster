// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Blaster/BlasterTypes/WeaponType.h"
#include "Weapon.generated.h"


#define NOISE_A (uint32)0xB6297A4D
#define NOISE_B (uint32)0x68E31DA4
#define NOISE_C (uint32)0x1B56C4E9
#define RANDOM_TO_FLOAT 2.32830643653869628906e-010f
#define LARGERANDOM_A (uint32)198491317
#define LARGERANDOM_B (uint32)6542989
#define LARGERANDOM_C (uint32)60493

UENUM(BlueprintType)
enum class EWeaponState : uint8
{
	EWS_Initial UMETA(DisplayName = "Initial State"),
	EWS_Equipped UMETA(DisplayName = "Equipped"),
	EWS_EquippedSecondary UMETA(DisplayName = "Equipped Secondary"),
	EWS_Dropped UMETA(DisplayName = "Dropped"),

	EWS_MAX UMETA(DisplayName = "DefaultMax")
};

UCLASS()
class BLASTER_API AWeapon : public AActor
{
	GENERATED_BODY()
	
public:	
	AWeapon();
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void OnRep_Owner() override;

	void SetHUDAmmo();
	void SetHUDWeaponType();
	
	void ShowPickupWidget(bool bShowWidget);
	virtual void Fire(const FVector& HitTarget, const int32& Seed);

	void Dropped();
	void AddAmmo(int32 AmmoToAdd);
	
	/** 
	* Textures for the Weapon crosshairs
	**/
	
	UPROPERTY(EditAnywhere, Category = Crosshairs)
	class UTexture2D* CrosshairsCentre;
	UPROPERTY(EditAnywhere, Category = Crosshairs)
	UTexture2D* CrosshairsLeft;
	UPROPERTY(EditAnywhere, Category = Crosshairs)
	UTexture2D* CrosshairsRight;
	UPROPERTY(EditAnywhere, Category = Crosshairs)
	UTexture2D* CrosshairsTop;
	UPROPERTY(EditAnywhere, Category = Crosshairs)
	UTexture2D* CrosshairsBottom;

	/** 
	* Zoomed FOV while aiming
	**/
	UPROPERTY(EditAnywhere)
	float ZoomedFOV = 45.f;

	UPROPERTY(EditAnywhere)
	float ZoomInterpSpeed = 20.f;

	UPROPERTY(EditAnywhere, Category = Crosshairs)
	float CrosshairShootingExpansionAmount = 0.2f;

	UPROPERTY(EditAnywhere, Category = Crosshairs)
	float CrosshairShootingInterpSpeed = 40.f;

	UPROPERTY(EditAnywhere, Category = Combat)
	float FireDelay = 0.15f;

	UPROPERTY(EditAnywhere, Category = Combat)
	bool bAutomatic = true;

	UPROPERTY(EditAnywhere)
	class USoundCue* EquipSound;

	// Toggle Custom Depth
	void EnableCustomDepth(bool bEnable);

	bool bDestroyWeapon = false;

protected:
	virtual void BeginPlay() override;
	virtual void OnWeaponStateSet();
	virtual void OnEquipped();
	virtual void OnDropped();
	virtual void OnEquippedSecondary();

	UFUNCTION()
	virtual void OnSphereOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

	UFUNCTION()
	void OnSphereEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex
	);

	UPROPERTY(EditAnywhere)
	float Spread = 0.f;
	
	UPROPERTY(EditAnywhere)
	float Damage = 20.f;

	UPROPERTY(EditAnywhere)
	bool bUseServerSideRewind = false;

	UPROPERTY()
	class ABlasterCharacter* BlasterOwnerCharacter;
	UPROPERTY()
	class ABlasterPlayerController* BlasterOwnerController;

private:

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	USkeletalMeshComponent* WeaponMesh;

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	class USphereComponent* AreaSphere;

	UPROPERTY(ReplicatedUsing = OnRep_WeaponState, VisibleAnywhere, Category = "Weapon Properties")
	EWeaponState WeaponState;

	UFUNCTION()
	void OnRep_WeaponState();

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	class UWidgetComponent* PickupWidget;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	class UAnimationAsset* FireAnimation;

	UPROPERTY(EditAnywhere)
	TSubclassOf<class ACasing> CasingClass;

	UPROPERTY(EditAnywhere)
	int32 Ammo;

	void SpendRound();

	UFUNCTION(Client, Reliable)
	void ClientUpdateAmmo(int32 ServerAmmo);

	UFUNCTION(Client, Reliable)
	void ClientAddAmmo(int32 AmmoToAdd);

	//Number of unprocessed server requests for Ammo.
	// Incremented in SpendRound, Decremented in ClientUpdateAmmo
	int32 SequenceAmmo = 0;

	UPROPERTY(EditAnywhere)
	int32 MagCapacity;

	UPROPERTY(EditAnywhere)
	EWeaponType WeaponType;

public:	

	void SetWeaponState(EWeaponState State);
	FORCEINLINE class USphereComponent* GetAreaSphere() const { return AreaSphere; }
	FORCEINLINE USkeletalMeshComponent* GetWeaponMesh() const { return WeaponMesh; }
	FORCEINLINE float GetZoomedFOV() const { return ZoomedFOV; }
	FORCEINLINE float GetZoomInterpSpeed() const { return ZoomInterpSpeed; }
	FORCEINLINE float GetFireDelay() const { return FireDelay; }
	FORCEINLINE bool GetIsAutomatic() const { return bAutomatic; }
	FORCEINLINE bool GetIsEmpty() const { return Ammo <= 0; }
	FORCEINLINE bool GetIsFull() const { return Ammo >= MagCapacity; }
	FORCEINLINE EWeaponType GetWeaponType() const { return WeaponType; }
	FORCEINLINE int32 GetAmmo() const { return Ammo; }
	FORCEINLINE int32 GetMagCapacity() const { return MagCapacity; }

	uint32 Hash(const uint32& Input, const uint32& Seed) const;
	float HashFloatZeroToOne(const uint32& Input, const uint32& Seed) const;
	FVector VConeProcedural(FVector const& Dir, float ConeHalfAngleDeg, const uint32& PelletNum, const uint32& Seed) const;
	FORCEINLINE float GetDamage() const { return Damage; }
	FORCEINLINE float GetSpread() const { return Spread; }
};
