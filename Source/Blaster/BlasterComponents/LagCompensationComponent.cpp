// Fill out your copyright notice in the Description page of Project Settings.


#include "LagCompensationComponent.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Blaster/BlasterComponents/BuffComponent.h"
#include "Blaster/Weapon/Weapon.h"
#include "Components/BoxComponent.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Blaster/BlasterComponents/CombatComponent.h"
#include "Blaster/Blaster.h"

ULagCompensationComponent::ULagCompensationComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void ULagCompensationComponent::BeginPlay()
{
	Super::BeginPlay();

	FFramePackage Package;
	SaveFramePackage(Package);
	//ShowFramePackage(Package, FColor::Orange);	
}

void ULagCompensationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (Character == nullptr || !Character->HasAuthority())
	{
		return;
	}
	
	SaveFramePackage();	
}

void ULagCompensationComponent::ServerScoreRequest_Implementation(const TArray<ABlasterCharacter*>& HitCharacters, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation, float HitTime, const int32& NumberOfPellets, const uint32& Seed)
{
	// perform HitScan in a loop for all pellets as per HitScanWeapon. Save into TMap<ABlasterCharacter*, float> the total damage for each hit character.
	// The HitCharacters array is the total array the client is claiming to hit, so we will only need to check the frame histories of these specific characters here.
	// No physics forces required here, they are handled on the server regardless of bUseServerSideRewind

	Character = Character == nullptr ? Cast<ABlasterCharacter>(GetOwner()) : Character;
	if (Character == nullptr)
	{
		return;
	}

	Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;
	if (Controller == nullptr)
	{
		return;
	}

	AWeapon* InstigatorWeapon =  Character->GetEquippedWeapon();
	if (InstigatorWeapon == nullptr)
	{
		return;
	}

	TMap<ABlasterCharacter*, float> DamageMap;

	// Create array of HitBoxes for the set of HitCharacters once, and then check against this for all pellets before moving them all back again
	TArray<FFramePackage> FramePackages;
	for (ABlasterCharacter* HitCharacter : HitCharacters)
	{
		FFramePackage Package = GetFrameToCheck(HitCharacter, HitTime);
		if (Package.PackageCharacter)
		{
			FramePackages.Add(Package);
		}
	} 

	TArray<FFramePackage> OriginalFramePackages;
	for (FFramePackage& Package : FramePackages)
	{
		FFramePackage OriginalPackage;
		CacheBoxPositions(Package.PackageCharacter, OriginalPackage);
		OriginalFramePackages.Add(OriginalPackage);
		MoveBoxes(Package);
		EnableCharacterMeshCollision(Package.PackageCharacter, ECollisionEnabled::NoCollision);

		//Enable collision for head first, then line trace rest of the boxes only if we didn't score a headshot
		if (!Package.PackageCharacter)
		{
			continue;
		}

		UBoxComponent* HeadBox = Package.PackageCharacter->HitCollisionBoxes[FName("HitboxHead")];
		HeadBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		HeadBox->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);
	}

	UWorld* World = GetWorld();

	float Multiplier = 1.f;

	if (Character->GetBuffComponent())
	{
		Multiplier = FMath::Max(Multiplier, Character->GetBuffComponent()->GetDamageMultiplier()); // we *could* create a history for this but as SSR will only be used up to max few 100ms, we will leave this for now
	}

	TArray<bool> PelletsThatAlreadyHit;

	for (int32 i = 0; i < NumberOfPellets; i++)
	{
		bool bHit = HitScanServerSideRewind(TraceStart, HitLocation, DamageMap, Multiplier, (uint32) i, Seed, World, InstigatorWeapon, true);
		PelletsThatAlreadyHit.Add(bHit);
	}

	for (FFramePackage& Package : FramePackages)
	{
		// disable collision for head & line trace rest of the boxes only if we didn't score a headshot
		if (!Package.PackageCharacter)
		{
			continue;
		}

		for (auto& HitBoxPair : Package.PackageCharacter->HitCollisionBoxes)
		{
			if (HitBoxPair.Value != nullptr)
			{
				HitBoxPair.Value->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
				HitBoxPair.Value->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);
			}
		}

		// Disable the head boxes
		UBoxComponent* HeadBox = Package.PackageCharacter->HitCollisionBoxes[FName("HitboxHead")];
		if (HeadBox == nullptr)
		{
			continue;
		}
		HeadBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	for (int32 i = 0; i < NumberOfPellets; i++)
	{
		if (!PelletsThatAlreadyHit[i])
		{
			HitScanServerSideRewind(TraceStart, HitLocation, DamageMap, Multiplier, (uint32) i, Seed, World, InstigatorWeapon);
		}
	}

	for(auto& DamageEvent : DamageMap)
	{
		if (DamageEvent.Key)
		{
			UGameplayStatics::ApplyDamage(DamageEvent.Key, DamageEvent.Value, Controller, InstigatorWeapon, UDamageType::StaticClass());
		}
	}

	for (FFramePackage& Package : OriginalFramePackages)
	{
		MoveBoxes(Package, true);
		if (Package.PackageCharacter)
		{
			EnableCharacterMeshCollision(Package.PackageCharacter, ECollisionEnabled::QueryAndPhysics);
		}
	}
}

bool ULagCompensationComponent::HitScanServerSideRewind(const FVector& TraceStart, const FVector& HitTarget, TMap<ABlasterCharacter*, float> &DamageMap, const float& DamageMultiplier, const uint32& PelletNum, const uint32& Seed, const UWorld* World, const AWeapon* InstigatorWeapon, bool bHeadShots)
{
	if (Character == nullptr || Character->GetEquippedWeapon() == nullptr)
	{
		return false;
	}
	
	bool bHit = false;

	FVector NewTraceDirection = InstigatorWeapon->VConeProcedural((HitTarget - TraceStart).GetSafeNormal(), InstigatorWeapon->GetSpread(), PelletNum, Seed);
    FVector End = TraceStart + (NewTraceDirection * TRACE_LENGTH);
    FHitResult FireHit;

    if (World)
    {
        World->LineTraceSingleByChannel
        (
            FireHit,
            TraceStart,
            End,
            ECC_HitBox
        );

        FVector BeamEnd = End;

        if (FireHit.bBlockingHit)
        {
            BeamEnd = FireHit.ImpactPoint;
        
            ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(FireHit.GetActor());
            if (BlasterCharacter)
            {
                if (DamageMap.Contains(BlasterCharacter))
                {
                    DamageMap[BlasterCharacter] += DamageMultiplier * (InstigatorWeapon->GetDamage() + (bHeadShots ? InstigatorWeapon->GetHeadShotDamage() : 0.f));
                }
                else
                {
                    DamageMap.Emplace(BlasterCharacter, DamageMultiplier * (InstigatorWeapon->GetDamage() + (bHeadShots ? InstigatorWeapon->GetHeadShotDamage() : 0.f)));
                }

				bHit = true;
            }
        }
    }

	return bHit;
}


void ULagCompensationComponent::ProjectileServerScoreRequest_Implementation(ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize100& InitialVelocity, float HitTime)
{
	if (HitCharacter == nullptr)
	{
		return;
	}

	Character = Character == nullptr ? Cast<ABlasterCharacter>(GetOwner()) : Character;
	if (Character == nullptr)
	{
		return;
	}

	Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;
	if (Controller == nullptr)
	{
		return;
	}

	AWeapon* InstigatorWeapon =  Character->GetEquippedWeapon();
	if (InstigatorWeapon == nullptr)
	{
		return;
	}

	float Damage = 0.f;

	FFramePackage Package = GetFrameToCheck(HitCharacter, HitTime);

	FFramePackage OriginalPackage;
	CacheBoxPositions(HitCharacter, OriginalPackage);
	MoveBoxes(Package);
	EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::NoCollision);

	UBoxComponent* HeadBox = HitCharacter->HitCollisionBoxes[FName("HitboxHead")];
	HeadBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	HeadBox->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);

	UWorld* World = GetWorld();

	float Multiplier = 1.f;

	if (Character->GetBuffComponent())
	{
		Multiplier = FMath::Max(Multiplier, Character->GetBuffComponent()->GetDamageMultiplier()); // we *could* create a history for this but as SSR will only be used up to max few 100ms, we will leave this for now
	}

	bool bHit = ProjectileServerSideRewind(TraceStart, InitialVelocity);

	if (bHit)
	{
		Damage = Multiplier * (InstigatorWeapon->GetDamage() + InstigatorWeapon->GetHeadShotDamage());
	}
	else
	{
		for (auto& HitBoxPair : Package.PackageCharacter->HitCollisionBoxes)
		{
			if (HitBoxPair.Value != nullptr)
			{
				HitBoxPair.Value->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
				HitBoxPair.Value->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);
			}
		}

		if (HeadBox)
		{
			HeadBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
		
		bHit = ProjectileServerSideRewind(TraceStart, InitialVelocity);

		if (bHit)
		{
			Damage = InstigatorWeapon->GetDamage() * Multiplier;
		}
	}

	if (bHit)
	{
		UGameplayStatics::ApplyDamage(HitCharacter, Damage, Controller, InstigatorWeapon, UDamageType::StaticClass());
	}

	MoveBoxes(Package, true);
	EnableCharacterMeshCollision(HitCharacter, ECollisionEnabled::QueryAndPhysics);
}

bool ULagCompensationComponent::ProjectileServerSideRewind(const FVector& TraceStart, const FVector& InitialVelocity)
{
	FPredictProjectilePathParams PathParams;
	PathParams.bTraceWithChannel = true;
	PathParams.bTraceWithCollision = true;
	PathParams.LaunchVelocity = InitialVelocity;
	PathParams.MaxSimTime = MaxRecordTime;
	PathParams.ProjectileRadius = 5.f;
	PathParams.SimFrequency = 15.f;
	PathParams.StartLocation = TraceStart;
	PathParams.TraceChannel = ECC_HitBox;
	PathParams.ActorsToIgnore.Add(GetOwner());

	FPredictProjectilePathResult PathResult;
	UGameplayStatics::PredictProjectilePath(this, PathParams, PathResult);

	return PathResult.HitResult.bBlockingHit;
}

FFramePackage ULagCompensationComponent::GetFrameToCheck(ABlasterCharacter* HitCharacter, float HitTime)
{
	bool bReturn = 
		HitCharacter == nullptr ||
		HitCharacter->GetLagCompensation() == nullptr ||
		HitCharacter->GetLagCompensation()->FrameHistory.GetHead() == nullptr ||
		HitCharacter->GetLagCompensation()->FrameHistory.GetTail() == nullptr;

	if (bReturn)
	{
		return FFramePackage();
	}

	//Check this frame to verify the hit
	FFramePackage FrameToCheck;
	bool bShouldInterpolate = true;

	// Frame history of the character that we're checking as a hit target
	const TDoubleLinkedList<FFramePackage>& History = HitCharacter->GetLagCompensation()->FrameHistory;

	const float OldestHistoryTime = History.GetTail()->GetValue().Time;

	if (OldestHistoryTime > HitTime)
	{
		// Too old for ServerSideRewind
		return FFramePackage();
	}

	if (OldestHistoryTime == HitTime)
	{
		FrameToCheck = History.GetTail()->GetValue();
		bShouldInterpolate = false;
	}

	const float NewestHistoryTime = History.GetHead()->GetValue().Time;
	if (NewestHistoryTime <= HitTime)
	{
		// Check only the first frame
		FrameToCheck = History.GetHead()->GetValue();
		bShouldInterpolate = false;
	}

	TDoubleLinkedList<FFramePackage>::TDoubleLinkedListNode* Younger = History.GetHead();
	TDoubleLinkedList<FFramePackage>::TDoubleLinkedListNode* Older = Younger;

	while (Older->GetValue().Time > HitTime)
	{
		if (Older->GetNextNode() == nullptr)
		{
			break;
		}

		Older = Older->GetNextNode();

		if (Older->GetValue().Time > HitTime)
		{
			Younger = Older;
		}
	}

	if (Older->GetValue().Time == HitTime)
	{
		// (Highly unlikely) early exit condition
		FrameToCheck = Older->GetValue();
		bShouldInterpolate = false;
	}

	if (bShouldInterpolate)
	{
		FrameToCheck = InterpBetweenFrames(Older->GetValue(), Younger->GetValue(), HitTime);
	}

	if (FrameToCheck.PackageCharacter == nullptr)
	{
		FrameToCheck.PackageCharacter = HitCharacter;
	}

	return FrameToCheck;
}

void ULagCompensationComponent::SaveFramePackage()
{
	FFramePackage ThisFrame;
	SaveFramePackage(ThisFrame);
	
	if (FrameHistory.Num() <= 1)
	{
		FrameHistory.AddHead(ThisFrame);
	}
	else
	{
		float HistoryLength = FrameHistory.GetHead()->GetValue().Time - FrameHistory.GetTail()->GetValue().Time;

		while (HistoryLength > MaxRecordTime)
		{
			FrameHistory.RemoveNode(FrameHistory.GetTail());
			HistoryLength = FrameHistory.GetHead()->GetValue().Time - FrameHistory.GetTail()->GetValue().Time;
		}

		FrameHistory.AddHead(ThisFrame);
	}

	//ShowFramePackage(ThisFrame, FColor::Red);
}

void ULagCompensationComponent::SaveFramePackage(FFramePackage& Package)
{
	Character = Character == nullptr ? Cast<ABlasterCharacter>(GetOwner()) : Character;

	if (Character)
	{
		if (!GetWorld())
		{
			return;
		}

		Package.Time = GetWorld()->GetTimeSeconds();
		Package.PackageCharacter = Character;

		for (auto& BoxPair : Character->HitCollisionBoxes)
		{
			FBoxInformation BoxInformation;
			BoxInformation.Location = BoxPair.Value->GetComponentLocation();
			BoxInformation.Rotation = BoxPair.Value->GetComponentRotation();
			BoxInformation.BoxExtent = BoxPair.Value->GetScaledBoxExtent();

			Package.HitBoxInfo.Add(BoxPair.Key, BoxInformation);
		}
	}
}

void ULagCompensationComponent::ShowFramePackage(const FFramePackage& Package, const FColor& Color)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (auto& BoxInfo : Package.HitBoxInfo)
	{
		DrawDebugBox
		(
			World,
			BoxInfo.Value.Location,
			BoxInfo.Value.BoxExtent,
			FQuat(BoxInfo.Value.Rotation),
			Color,
			false,
			4.f
		);
	}
}

FFramePackage ULagCompensationComponent::InterpBetweenFrames(const FFramePackage& OlderFrame, const FFramePackage& YoungerFrame, float HitTime)
{
	const float Denominator = YoungerFrame.Time - OlderFrame.Time;
	if (Denominator <= 0.f)
	{
		return YoungerFrame;
	}

	FFramePackage OutPackage;
	const float InterpFraction = FMath::Clamp((HitTime - OlderFrame.Time) / Denominator, 0.f, 1.f);
	OutPackage.Time = HitTime;
	OutPackage.PackageCharacter = YoungerFrame.PackageCharacter;
	
	for (auto& YoungerPair : YoungerFrame.HitBoxInfo)
	{
		const FName& BoxInfoName = YoungerPair.Key;
		const FBoxInformation& OlderBox = OlderFrame.HitBoxInfo[BoxInfoName];
		const FBoxInformation& YoungerBox = YoungerPair.Value;

		FBoxInformation OutBoxInformation;
		OutBoxInformation.Location = FMath::Lerp(OlderBox.Location, YoungerBox.Location, InterpFraction);
		OutBoxInformation.Rotation = FMath::Lerp(OlderBox.Rotation, YoungerBox.Rotation, InterpFraction);
		OutBoxInformation.BoxExtent = YoungerBox.BoxExtent;
		OutPackage.HitBoxInfo.Add(BoxInfoName, OutBoxInformation);
	}

	return OutPackage;
}

void ULagCompensationComponent::CacheBoxPositions(ABlasterCharacter* CharacterToCache, FFramePackage& OutFramePackage)
{
	if (CharacterToCache == nullptr)
	{
		return;
	}

	OutFramePackage.PackageCharacter = CharacterToCache;

	for (auto& HitBoxPair : OutFramePackage.PackageCharacter->HitCollisionBoxes)
	{
		if (HitBoxPair.Value != nullptr)
		{
			FBoxInformation BoxInfo;
			BoxInfo.Location = HitBoxPair.Value->GetComponentLocation();
			BoxInfo.Rotation = HitBoxPair.Value->GetComponentRotation();
			BoxInfo.BoxExtent = HitBoxPair.Value->GetScaledBoxExtent();
			OutFramePackage.HitBoxInfo.Add(HitBoxPair.Key, BoxInfo);
		}
	}
}

void ULagCompensationComponent::MoveBoxes(const FFramePackage& Package, bool bResetBoxes)
{
	if (Package.PackageCharacter == nullptr)
	{
		return;
	}
	
	for (auto& HitBoxPair : Package.PackageCharacter->HitCollisionBoxes)
	{	
		if (HitBoxPair.Value != nullptr)
		{
			HitBoxPair.Value->SetWorldLocation(Package.HitBoxInfo[HitBoxPair.Key].Location);
			HitBoxPair.Value->SetWorldRotation(Package.HitBoxInfo[HitBoxPair.Key].Rotation);
			HitBoxPair.Value->SetBoxExtent(Package.HitBoxInfo[HitBoxPair.Key].BoxExtent);

			if (bResetBoxes)
			{
				HitBoxPair.Value->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			}
		}
	}
}

void ULagCompensationComponent::EnableCharacterMeshCollision(ABlasterCharacter* HitCharacter, ECollisionEnabled::Type CollisionEnabled)
{
	if (HitCharacter && HitCharacter->GetMesh())
	{
		HitCharacter->GetMesh()->SetCollisionEnabled(CollisionEnabled);
	}
}
