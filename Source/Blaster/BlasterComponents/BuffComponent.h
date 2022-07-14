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

UCLASS()
class BLASTER_API UBuffComponent : public UActorComponent
{
	GENERATED_BODY()
	
public:	
	UBuffComponent();

	friend class ABlasterCharacter;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void AddNewHealing(float HealthAmount, float HealingDelay, float HealingTime);
	void UpdateHUDHealing();

protected:
	virtual void BeginPlay() override;

	void Heal(float DeltaTime);

private:
	UPROPERTY()
	class ABlasterCharacter* Character;

	UPROPERTY()
	class ABlasterPlayerController* Controller;

	TArray<Healing> HealingArray;

	UPROPERTY(ReplicatedUsing = OnRep_TargetHealingPercent, VisibleAnywhere)
	float TargetHealingPercent = 0.f;

	UFUNCTION()
	void OnRep_TargetHealingPercent();

public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

};
