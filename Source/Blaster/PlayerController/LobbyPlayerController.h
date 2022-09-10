// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "LobbyPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API ALobbyPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(NetMulticast, Reliable)
	void MulticastSetPlayerList(const TArray<FString>& LobbyPlayerNames);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastSetLobbyDetails(const int32& DesiredMaxPlayerCount, const FString& DesiredMatchType);

	virtual void PostSeamlessTravel() override;

protected:

	
private:

	void PollInit();

	TArray<FString> PlayerNames;
	FString MatchType { "" };
	int32 MaxPublicConnections{ 2 };

	void UpdateLobbyHUD();

	UPROPERTY()
	class ALobbyHUD* LobbyHUD;

public:

	FORCEINLINE TArray<FString> GetPlayerNames() const { return PlayerNames; }
	FORCEINLINE int32 GetMaxPublicConnections() const { return MaxPublicConnections; }
	FORCEINLINE int32 GetRequiredPublicConnections() const { return FMath::Max(2, MaxPublicConnections / 2); }
	FORCEINLINE FString GetMatchType() const { return MatchType; }
};
