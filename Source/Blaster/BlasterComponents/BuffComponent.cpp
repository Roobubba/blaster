// Fill out your copyright notice in the Description page of Project Settings.


#include "BuffComponent.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"

UBuffComponent::UBuffComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

}

void UBuffComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UBuffComponent, TargetHealingPercent);
	DOREPLIFETIME(UBuffComponent, TargetShieldRegenPercent);
	
}

void UBuffComponent::BeginPlay()
{
	Super::BeginPlay();
	
}
void UBuffComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	Heal(DeltaTime);
	RegenShield(DeltaTime);
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

void UBuffComponent::UpdateHUDShieldRegen()
{
	if (Character)
	{
		if (Character->HasAuthority())
		{
			float TargetShield = Character->GetShield();

			for (ShieldRegen &CurrentShield : ShieldRegenArray)
			{
				TargetShield += CurrentShield.ShieldRegenRemaining;
			}

			TargetShieldRegenPercent = TargetShield / Character->GetMaxShield();
		}

		Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;
		if (Controller)
		{
			Controller->SetHUDShieldExtraRegen(TargetShieldRegenPercent);
		}
	}
}

void UBuffComponent::RegenShield(float DeltaTime)
{
	if (Character && !Character->IsEliminated() && ShieldRegenArray.Num() > 0)
	{
		float TotalQueuedShields = 0.f;
		float TotalAmountToRegenThisFrame = 0.f;
		float MaximumPossibleShieldRegenThisFrame = Character->GetMaxShield() - Character->GetShield();
		bool bRegenMaxAndDestroyRemainingShieldRegens = false;
		for (ShieldRegen &CurrentShield : ShieldRegenArray)
		{
			if (bRegenMaxAndDestroyRemainingShieldRegens)
			{
				break;
			}

			TotalQueuedShields += CurrentShield.ShieldRegenRemaining;

			if (CurrentShield.ShieldRegenDelayRemaining > 0.f)
			{
				CurrentShield.ShieldRegenDelayRemaining -= DeltaTime;
				continue;
			}
			else
			{
				float AmountToRegenThisFrame = FMath::Min(CurrentShield.TargetShieldRegenRate * DeltaTime, CurrentShield.ShieldRegenRemaining);
				TotalAmountToRegenThisFrame += AmountToRegenThisFrame;
				CurrentShield.ShieldRegenRemaining -= AmountToRegenThisFrame;
				CurrentShield.ShieldRegenTimeRemaining -= DeltaTime;

				if (TotalAmountToRegenThisFrame >= MaximumPossibleShieldRegenThisFrame)
				{
					bRegenMaxAndDestroyRemainingShieldRegens = true;
				}
			}
		}

		TargetShieldRegenPercent = (TotalQueuedShields + Character->GetShield()) / Character->GetMaxShield();

		if (bRegenMaxAndDestroyRemainingShieldRegens)
		{
			Character->SetShield(Character->GetMaxShield());
			Character->UpdateHUDShield();

			ShieldRegenArray.Empty();
		}
		else if (TotalAmountToRegenThisFrame > 0.f)
		{
			Character->SetShield(FMath::Min(Character->GetShield() + TotalAmountToRegenThisFrame, Character->GetMaxShield()));
			Character->UpdateHUDShield();
			ShieldRegenArray.RemoveAllSwap([](ShieldRegen &Val)
			{
				return Val.ShieldRegenTimeRemaining <= 0.f || Val.ShieldRegenRemaining <= 0.f;
			});
		}

		UpdateHUDShieldRegen();
	}
}

void UBuffComponent::AddNewShield(float ShieldAmount, float ShieldRegenDelay, float ShieldRegenTime)
{
	if (Character)
	{
		ShieldRegen ShieldToAdd;
		ShieldToAdd.ShieldRegenRemaining = ShieldAmount;
		ShieldToAdd.ShieldRegenDelayRemaining = ShieldRegenDelay;
		ShieldToAdd.ShieldRegenTimeRemaining = ShieldRegenTime;
		ShieldToAdd.TargetShieldRegenRate = ShieldRegenTime > 0.f ? ShieldAmount / ShieldRegenTime : 1000000.f;
		ShieldRegenArray.Emplace(ShieldToAdd);
		
		UpdateHUDShieldRegen();
	}
}

void UBuffComponent::OnRep_TargetShieldRegenPercent()
{
	UpdateHUDShieldRegen();
}

void UBuffComponent::BuffSpeed(float Multiplier, float SpeedBuffTime)
{
	if (Character)
	{
		Character->GetWorldTimerManager().SetTimer
		(
			SpeedBuffTimer,
			this,
			&UBuffComponent::ResetSpeed,
			SpeedBuffTime
		);

		BaseSpeedMultiplier = Multiplier;

		MulticastSpeedBuff(BaseSpeedMultiplier);
	}
}

void UBuffComponent::ResetSpeed()
{
	BaseSpeedMultiplier = 1.f;

	MulticastSpeedBuff(BaseSpeedMultiplier);
}

void UBuffComponent::MulticastSpeedBuff_Implementation(float NewSpeedMultiplier)
{
	BaseSpeedMultiplier = NewSpeedMultiplier;
	
	if (Character)
	{
		Character->UpdateMovementSpeed();
	}
}

void UBuffComponent::BuffJump(float JumpMultiplier, float JumpBuffTime)
{
	if (Character)
	{
		Character->GetWorldTimerManager().SetTimer
		(
			JumpBuffTimer,
			this,
			&UBuffComponent::ResetJump,
			JumpBuffTime
		);

		BaseJumpMultiplier = JumpMultiplier;

		MulticastJumpBuff(BaseJumpMultiplier);
		UpdateJumpVerticalVelocity();
	}
}

void UBuffComponent::ResetJump()
{
	BaseJumpMultiplier = 1.f;
	MulticastJumpBuff(BaseJumpMultiplier);
	UpdateJumpVerticalVelocity();
}

void UBuffComponent::SetInitialJumpVerticalVelocity(float InitialJump)
{
	InitialJumpVerticalVelocity = InitialJump;
}

void UBuffComponent::MulticastJumpBuff_Implementation(float NewJumpMultiplier)
{
	BaseJumpMultiplier = NewJumpMultiplier;
	
	UpdateJumpVerticalVelocity();
}

void UBuffComponent::UpdateJumpVerticalVelocity()
{
	if (Character && Character->GetCharacterMovement())
	{
		Character->GetCharacterMovement()->JumpZVelocity = BaseJumpMultiplier * InitialJumpVerticalVelocity;
	}
}