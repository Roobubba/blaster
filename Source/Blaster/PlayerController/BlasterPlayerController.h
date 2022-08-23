// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Blaster/BlasterTypes/WeaponType.h"
#include "BlasterPlayerController.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHighPingDelegate, bool, bPingTooHigh);

/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterPlayerController : public APlayerController
{
	GENERATED_BODY()

public:

	virtual void Tick(float DeltaTime) override;
	virtual float GetServerTime();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void SetHUDHealth(float Health, float MaxHealth);
	void SetHUDHealthExtraHealing(float HealingPercent);

	void SetHUDShield(float Shield, float MaxShield);
	void SetHUDShieldExtraRegen(float ShieldRegenPercent);

	void SetHUDScore(float Score);
	void SetHUDDefeats(int32 Defeats);

	void BroadcastElimination(APlayerState* Attacker, APlayerState* Victim);
	void ToggleChatInput();

	UFUNCTION(Server, Reliable)
	void ServerBroadcastChatMessage(APlayerController* Sender, const FString& Message);

	void BroadcastChatMessage(const FString& Message);

	virtual void OnPossess(APawn* InPawn) override;

	void SetHUDWeaponAmmo(int32 Ammo);
	void SetHUDWeaponType(EWeaponType WeaponType);
	void SetHUDCarriedAmmo(int32 Ammo);
	void SetHUDMatchCountdown(float CountdownTime);
	void SetHUDGrenades(int32 Grenades);

	void SetHUDAnnouncementCountdown(float CountdownTime);

	void ShowPickupAnnouncement(FString PickupAnnouncement, float DisplayTime, FColor Colour);
	void RemoveAllPickupTexts();

	virtual void ReceivedPlayer() override; // earliest time to sync with server clock
	void OnMatchStateSet(FName State, bool bTeamsMatch = false);
	void HandleMatchHasStarted(bool bTeamsMatch = false);
	void HandleCooldown();
	void HandleWaitingToStart();

	UFUNCTION(Server, Reliable)
	void ServerGetMatchState();

	float SingleTripTime = 0.f;

	FHighPingDelegate HighPingDelegate;

	void HideTeamScores();
	void InitTeamScores();
	void SetHUDRedTeamScore(int32 RedScore);
	void SetHUDBlueTeamScore(int32 BlueScore);

protected:

	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	void SetHUDTime();
	void PollInit();

	/*
	* Sync time between client and server
	*/

	// Requests the current server time, passing in the client's time when the request was sent
	UFUNCTION(Server, Reliable)
	void ServerRequestServerTime(float TimeOfClientRequest);

	// Reports the current server time to the client in response to ServerRequestServerTime
	UFUNCTION(Client, Reliable)
	void ClientReportServerTime(float TimeOfClientRequest, float TimeServerReceivedClientRequest);

	// Difference between client and server time
	float ClientServerTimeDelta = 0.f;

	UPROPERTY(EditAnywhere, Category = Time)
	float TimeSyncFrequency = 5.f;
	float TimeSinceLastSync = 0.f;
	void CheckTimeSync(float DeltaTime);

	UFUNCTION(Client, Reliable)
	void ClientJoinMidgame(FName StateOfMatch, float Warmup, float Match, float Cooldown, float StartingTime, bool bTeamsMatch);

	void HighPingWarning();
	void StopHighPingWarning();
	void CheckPing(float DeltaTime);
	void ShowReturnToMainMenu();

	UFUNCTION(Client, Reliable)
	void ClientEliminationAnnouncement(APlayerState* Attacker, APlayerState* Victim);

	UFUNCTION(Client, Reliable)
	void ClientReceiveChatMessage(const FString& Message);

	UFUNCTION(Client, Reliable)
	void ClientAddPickupText(const FString& PickupAnnouncement, float DisplayTime, const FColor& Colour);

private:

	UPROPERTY(EditAnywhere, Category = HUD)
	TSubclassOf<class UUserWidget> ReturnToMainMenuWidget;
	
	class UReturnToMainMenu* ReturnToMainMenu;
	bool bReturnToMainMenuOpen = false;

	UPROPERTY()
	class ABlasterHUD* BlasterHUD;

	UPROPERTY()
	class ABlasterGameMode* BlasterGameMode;

	float MatchTime = 0.f;
	float WarmupTime = 0.f;
	float LevelStartingTime = 0.f;
	float CooldownTime = 0.f;
	uint32 CountdownInt = 0;

	//UPROPERTY(ReplicatedUsing = OnRep_MatchState)
	FName MatchState;

	//UFUNCTION()
	//void OnRep_MatchState();

	UPROPERTY()
	class UCharacterOverlay* CharacterOverlay = nullptr;

	UPROPERTY()
	class UAnnouncement* Announcement = nullptr;

	UPROPERTY()
	class UChatInput* ChatInput = nullptr;

	bool bInitializeHUDHealth;
	bool bInitializeHUDShield;	
	bool bInitializeHUDScore;
	bool bInitializeHUDDefeats;
	bool bInitializeHUDWeaponAmmo;
	bool bInitializeHUDCarriedAmmo;
	bool bInitializeHUDGrenades;
	bool bInitializeHUDWeaponType;

	float HUDHealth;
	float HUDMaxHealth;
	float HUDShield;
	float HUDMaxShield;
	float HUDScore;
	int32 HUDDefeats;
	int32 HUDAmmo = 0;
	int32 HUDCarriedAmmo = 0;
	int32 HUDGrenades = 4;
	EWeaponType HUDWeaponType = EWeaponType::EWT_MAX;

	float HighPingRunningTime = 0.f;
	float HighPingAnimationRunningTime = 0.f;

	UPROPERTY(EditAnywhere)
	float HighPingMaximumDuration = 5.f;

	UPROPERTY(EditAnywhere)
	float HighPingCheckFrequency = 20.f;

	UFUNCTION(Server, Reliable)
	void ServerReportPingStatus(bool bPingTooHigh);

	UPROPERTY(EditAnywhere)
	float HighPingThreshold = 50.f;

public:
	FORCEINLINE FName GetMatchState() const { return MatchState; }
};
