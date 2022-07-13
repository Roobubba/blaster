// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Blaster/BlasterTypes/WeaponType.h"
#include "BlasterPlayerController.generated.h"

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
	void SetHUDScore(float Score);
	void SetHUDDefeats(int32 Defeats);

	void AnnounceElim();

	virtual void OnPossess(APawn* InPawn) override;

	void SetHUDWeaponAmmo(int32 Ammo);
	void SetHUDWeaponType(EWeaponType WeaponType);
	void SetHUDCarriedAmmo(int32 Ammo);
	void SetHUDMatchCountdown(float CountdownTime);
	void SetHUDGrenades(int32 Grenades);

	void SetHUDAnnouncementCountdown(float CountdownTime);

	virtual void ReceivedPlayer() override; // earliest time to sync with server clock
	void OnMatchStateSet(FName State);
	void HandleMatchHasStarted();
	void HandleCooldown();
	void HandleWaitingToStart();

	UFUNCTION(Server, Reliable)
	void ServerGetMatchState();

protected:
	virtual void BeginPlay() override;

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
	void ClientJoinMidgame(FName StateOfMatch, float Warmup, float Match, float Cooldown, float StartingTime);

private:
	UPROPERTY()
	class ABlasterHUD* BlasterHUD;

	UPROPERTY()
	class ABlasterGameMode* BlasterGameMode;

	float MatchTime = 0.f;
	float WarmupTime = 0.f;
	float LevelStartingTime = 0.f;
	float CooldownTime = 0.f;
	uint32 CountdownInt = 0;

	UPROPERTY(ReplicatedUsing = OnRep_MatchState)
	FName MatchState;

	UFUNCTION()
	void OnRep_MatchState();

	UPROPERTY()
	class UCharacterOverlay* CharacterOverlay = nullptr;

	UPROPERTY()
	class UAnnouncement* Announcement = nullptr;

	bool bInitializeHUDHealth;
	bool bInitializeHUDScore;
	bool bInitializeHUDDefeats;
	bool bInitializeHUDWeaponAmmo;
	bool bInitializeHUDCarriedAmmo;
	bool bInitializeHUDGrenades;
	bool bInitializeHUDWeaponType;

	float HUDHealth;
	float HUDMaxHealth;
	float HUDScore;
	int32 HUDDefeats;
	int32 HUDAmmo;
	int32 HUDCarriedAmmo;
	int32 HUDGrenades = 4;
	EWeaponType HUDWeaponType = EWeaponType::EWT_MAX;

public:
	FORCEINLINE FName GetMatchState() const { return MatchState; }
};
