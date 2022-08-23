// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TimelineComponent.h"
#include "PickupText.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API UPickupText : public UUserWidget
{
	GENERATED_BODY()
	
public:

	UPROPERTY(meta = (BindWidget))
	class UHorizontalBox* PickupTextBox;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* PickupText;

	void SetPickupText(FString PickupText, float DisplayTime, class ABlasterHUD* BlasterHUD);

protected:

	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:

	float RemainingTime;

	UPROPERTY(EditAnywhere)
	UCurveFloat* AnnouncementTextCurve;

	UFUNCTION()
	void PickupTextTimerFinished();

	UFUNCTION()
	void UpdateAnnouncementTextAlpha(float AnnouncementTextAlpha);

	FTimeline AnnouncementTimeline;

	FOnTimelineFloat AnnouncementTextTrack;

	FTimerHandle AnnouncementTextTimerHandle;

	ABlasterHUD* BlasterHUD;

public:
	FORCEINLINE float GetRemainingTime() const { return RemainingTime; }
};
