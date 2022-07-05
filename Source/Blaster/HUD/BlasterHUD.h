// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "Components/TimelineComponent.h"
#include "BlasterHUD.generated.h"

USTRUCT(BlueprintType)
struct FHUDPackage
{
	GENERATED_BODY()

public:
	UPROPERTY()
	class UTexture2D* CrosshairsCentre = nullptr;
	UPROPERTY()
	UTexture2D* CrosshairsLeft = nullptr;
	UPROPERTY()
	UTexture2D* CrosshairsRight = nullptr;
	UPROPERTY()
	UTexture2D* CrosshairsTop = nullptr;
	UPROPERTY()
	UTexture2D* CrosshairsBottom = nullptr;
	float CrosshairSpread = 0.58f;
	FLinearColor CrosshairsColour = FLinearColor::White;
};

/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterHUD : public AHUD
{
	GENERATED_BODY()
	
public:

	ABlasterHUD();
	virtual void DrawHUD() override;
	void ShowElimMessage();

protected:

	virtual void BeginPlay() override;

private:
	FHUDPackage HUDPackage;

	void DrawCrosshair(UTexture2D* Texture, FVector2D ViewportCentre, FVector2D Spread, FLinearColor CrosshairColour);

	UPROPERTY(EditAnywhere)
	float CrosshairSpreadMax = 16.f;

	UPROPERTY(VisibleAnywhere)
	UTimelineComponent* ElimTextTimeline;
	FOnTimelineFloat ElimTextTrack;

	UFUNCTION()
	void UpdateElimTextAlpha(float ElimTextAlpha);

	UPROPERTY(EditAnywhere)
	UCurveFloat* ElimTextCurve;

	FTimerHandle ElimTextTimerHandle;

	UFUNCTION()
	void ElimTextTimerFinished();

	UPROPERTY(EditAnywhere, Category = "Player Stats")
	TSubclassOf<class UUserWidget> CharacterOverlayClass;

	UPROPERTY()
	class UCharacterOverlay* CharacterOverlay = nullptr;

	UPROPERTY(EditAnywhere, Category = "Announcements")
	TSubclassOf<class UUserWidget> AnnouncementClass;

	UPROPERTY()
	class UAnnouncement* Announcement = nullptr;

	void AddCharacterOverlay();
	void AddAnnouncement();

public:
	FORCEINLINE void SetHUDPackage(const FHUDPackage& Package) { HUDPackage = Package; }
	FORCEINLINE UAnnouncement* GetAnnouncement() const { return Announcement; }
	FORCEINLINE UCharacterOverlay* GetCharacterOverlay() const { return CharacterOverlay; }

};
