// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LagCompensationComponent.generated.h"

USTRUCT(BlueprintType)
struct FBoxInformation
{
	GENERATED_BODY()

	UPROPERTY()
	FVector Location;

	UPROPERTY()
	FRotator Rotation;

	UPROPERTY()
	FVector BoxExtent;

};

USTRUCT(BlueprintType)
struct FFramePackage
{
	GENERATED_BODY()

	UPROPERTY()
	float Time;

	UPROPERTY()
	TMap<FName, FBoxInformation> HitBoxInfo;

	UPROPERTY()
	ABlasterCharacter* PackageCharacter = nullptr;

};

UCLASS()
class BLASTER_API ULagCompensationComponent : public UActorComponent
{
	GENERATED_BODY()
	
public:	
	ULagCompensationComponent();
	
	friend class ABlasterCharacter;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	void ShowFramePackage(const FFramePackage& Package, const FColor& Color);

	UFUNCTION(Server, Reliable)
	void ServerScoreRequest(const TArray<ABlasterCharacter*>& HitCharacters, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation, float HitTime, const int32& NumberOfPellets, const uint32& Seed);

	UFUNCTION(Server, Reliable)
	void ProjectileServerScoreRequest(ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize100& InitialVelocity, float HitTime);


protected:
	virtual void BeginPlay() override;

	void SaveFramePackage(FFramePackage& Package);
	FFramePackage InterpBetweenFrames(const FFramePackage& OlderFrame, const FFramePackage& YoungerFrame, float HitTime);
	void CacheBoxPositions(ABlasterCharacter* CharacterToCache, FFramePackage& OutFramePackage);
	void MoveBoxes(const FFramePackage& Package, bool bResetBoxes = false);
	void EnableCharacterMeshCollision(ABlasterCharacter* HitCharacter, ECollisionEnabled::Type CollisionEnabled);

	void SaveFramePackage();

	bool HitScanServerSideRewind(const FVector& TraceStart, const FVector& HitTarget, TMap<ABlasterCharacter*, float> &DamageMap, const float& DamageMultiplier, const uint32& PelletNum, const uint32& Seed, const UWorld* World, const class AWeapon* InstigatorWeapon, bool bHeadShots = false);
	bool ProjectileServerSideRewind(const FVector& TraceStart, const FVector& InitialVelocity);
	
	FFramePackage GetFrameToCheck(ABlasterCharacter* HitCharacter, float HitTime);

private:
	UPROPERTY()
	ABlasterCharacter* Character;

	UPROPERTY()
	class ABlasterPlayerController* Controller;

	TDoubleLinkedList<FFramePackage> FrameHistory;
	
	UPROPERTY(EditAnywhere)
	float MaxRecordTime = 4.f;

public:	

};
