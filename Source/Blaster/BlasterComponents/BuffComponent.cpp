// Fill out your copyright notice in the Description page of Project Settings.


#include "BuffComponent.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Net/UnrealNetwork.h"

UBuffComponent::UBuffComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

}

void UBuffComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UBuffComponent, TargetHealingPercent);
}

void UBuffComponent::BeginPlay()
{
	Super::BeginPlay();
	
}

void UBuffComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	Heal(DeltaTime);
}

void UBuffComponent::UpdateHUDHealing()
{
	if (Character)
	{
		if (Character->HasAuthority())
		{
			float TargetHealing = Character->GetHealth();

			for (Healing &CurrentHealing : HealingArray)
			{
				TargetHealing += CurrentHealing.HealAmountRemaining;
			}

			TargetHealingPercent = TargetHealing / Character->GetMaxHealth();
		}

		Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;
		if (Controller)
		{
			Controller->SetHUDHealthExtraHealing(TargetHealingPercent);
		}
	}
}

void UBuffComponent::Heal(float DeltaTime)
{
	if (Character && !Character->IsEliminated() && HealingArray.Num() > 0)
	{
		float TotalQueuedHealing = 0.f;
		float TotalAmountToHealThisFrame = 0.f;
		float MaximumPossibleHealingThisFrame = Character->GetMaxHealth() - Character->GetHealth();
		bool bHealMaxAndDestroyRemainingHealing = false;
		for (Healing &CurrentHealing : HealingArray)
		{
			if (bHealMaxAndDestroyRemainingHealing)
			{
				break;
			}

			TotalQueuedHealing += CurrentHealing.HealAmountRemaining;

			if (CurrentHealing.HealDelayRemaining > 0.f)
			{
				CurrentHealing.HealDelayRemaining -= DeltaTime;
				continue;
			}
			else
			{
				float AmountToHealThisFrame = FMath::Min(CurrentHealing.TargetHealingRate * DeltaTime, CurrentHealing.HealAmountRemaining);
				TotalAmountToHealThisFrame += AmountToHealThisFrame;
				CurrentHealing.HealAmountRemaining -= AmountToHealThisFrame;
				CurrentHealing.HealTimeRemaining -= DeltaTime;

				if (TotalAmountToHealThisFrame >= MaximumPossibleHealingThisFrame)
				{
					bHealMaxAndDestroyRemainingHealing = true;
				}
			}
		}

		TargetHealingPercent = (TotalQueuedHealing + Character->GetHealth()) / Character->GetMaxHealth();

		if (bHealMaxAndDestroyRemainingHealing)
		{
			Character->SetHealth(Character->GetMaxHealth());
			Character->UpdateHUDHealth();

			HealingArray.Empty();
		}
		else if (TotalAmountToHealThisFrame > 0.f)
		{
			Character->SetHealth(FMath::Min(Character->GetHealth() + TotalAmountToHealThisFrame, Character->GetMaxHealth()));
			Character->UpdateHUDHealth();
			HealingArray.RemoveAllSwap([](Healing &Val)
			{
				return Val.HealTimeRemaining <= 0.f || Val.HealAmountRemaining <= 0.f;
			});
		}

		UpdateHUDHealing();
	}
}

void UBuffComponent::AddNewHealing(float HealthAmount, float HealingDelay, float HealingTime)
{
	if (Character)
	{
		Healing HealingToAdd;
		HealingToAdd.HealAmountRemaining = HealthAmount;
		HealingToAdd.HealDelayRemaining = HealingDelay;
		HealingToAdd.HealTimeRemaining = HealingTime;
		HealingToAdd.TargetHealingRate = HealingTime > 0.f ? HealthAmount / HealingTime : 1000000.f;
		HealingArray.Emplace(HealingToAdd);
		
		UpdateHUDHealing();
	}
}

void UBuffComponent::OnRep_TargetHealingPercent()
{
	UpdateHUDHealing();
}
