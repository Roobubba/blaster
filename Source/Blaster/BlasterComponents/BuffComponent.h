// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BuffComponent.generated.h"

struct Healing
{
public:
	float HealAmountRemaining;
	float HealDelayRemaining;
	float HealTimeRemaining;
	float TargetHealingRate;
};

struct ShieldRegen
{
public:
	float ShieldRegenRemaining;
	float ShieldRegenDelayRemaining;
	float ShieldRegenTimeRemaining;
	float TargetShieldRegenRate;
};

UCLASS()
class BLASTER_API UBuffComponent : public UActorComponent
{
	GENERATED_BODY()
	
public:	
	UBuffComponent();

	friend class ABlasterCharacter;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void AddNewHealing(float HealthAmount, float HealingDelay, float HealingTime);
	void UpdateHUDHealing();
	void AddNewShield(float ShieldAmount, float ShieldRegenDelay, float ShieldRegenTime);
	void UpdateHUDShieldRegen();

	void BuffSpeed(float BaseSpeedMultiplier, float SpeedBuffTime);
	void ResetSpeed();

	void BuffJump(float JumpMultiplier, float JumpBuffTime);
	void ResetJump();
	void SetInitialJumpVerticalVelocity(float InitialJumpVerticalVelocity);

	void ApplyDamageBuff(float DamageBuffMultiplier, float BuffTime);
	void ResetDamageBuff();

protected:
	virtual void BeginPlay() override;
	void SetInitialCrouchSpeed(float Speed);
	void Heal(float DeltaTime);
	void RegenShield(float DeltaTime);

private:
	UPROPERTY()
	class ABlasterCharacter* Character;

	UPROPERTY()
	class ABlasterPlayerController* Controller;

	/*
		Healing Buff
	*/

	TArray<Healing> HealingArray;

	UPROPERTY(ReplicatedUsing = OnRep_TargetHealingPercent, VisibleAnywhere)
	float TargetHealingPercent = 0.f;

	UFUNCTION()
	void OnRep_TargetHealingPercent();

	/*
		Shield Buff
	*/

	TArray<ShieldRegen> ShieldRegenArray;

	UPROPERTY(ReplicatedUsing = OnRep_TargetShieldRegenPercent, VisibleAnywhere)
	float TargetShieldRegenPercent = 0.f;

	UFUNCTION()
	void OnRep_TargetShieldRegenPercent();

	/*
		Speed Buff
	*/

	FTimerHandle SpeedBuffTimer;
	
	float BaseSpeedMultiplier = 1.f;

	UFUNCTION(NetMulticast, Reliable)
	void MulticastSpeedBuff(float NewSpeedMultiplier);

	/*
		Jump Buff
	*/

	FTimerHandle JumpBuffTimer;
	float BaseJumpMultiplier = 1.f;
	float InitialJumpVerticalVelocity;

	UFUNCTION(NetMulticast, Reliable)
	void MulticastJumpBuff(float NewJumpMultiplier);

	void UpdateJumpVerticalVelocity();

	/*
		Damage Buff
	*/

	FTimerHandle DamageBuffTimer;
	
	float BaseDamageMultiplier = 1.f;

	UFUNCTION(NetMulticast, Reliable)
	void MulticastDamageBuff(float NewDamageMultiplier);
	
public:	
	FORCEINLINE float GetSpeedMultiplier() const { return BaseSpeedMultiplier; }
	FORCEINLINE float GetDamageMultiplier() const { return BaseDamageMultiplier; }
};
