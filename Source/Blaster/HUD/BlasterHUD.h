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

	virtual void DrawHUD() override;

	void AddEliminationAnnouncement(FString Attacker, FString Victim);

	void AddChatMessage(const FString& Message);
	void ResetChatSystem();

	UFUNCTION()
	void ChatInputCommitted(const FText& Text, ETextCommit::Type CommitMethod);

	void AddPickupText(FString PickupAnnouncement, float DisplayTime, FColor Colour);
	void PickupTextTimerFinished(class UPickupText* MessageToRemove);
	void RemoveAllPickupTexts();
protected:

	virtual void BeginPlay() override;

private:

	UPROPERTY()
	class APlayerController* OwningPlayerController;

	UPROPERTY()
	class ABlasterPlayerController* OwningBlasterPlayerController;

	FHUDPackage HUDPackage;

	void DrawCrosshair(UTexture2D* Texture, FVector2D ViewportCentre, FVector2D Spread, FLinearColor CrosshairColour);

	UPROPERTY(EditAnywhere)
	float CrosshairSpreadMax = 16.f;

	UPROPERTY(EditAnywhere, Category = "PickupText")
	TSubclassOf<UPickupText> PickupTextClass;

	UPROPERTY()
	TArray<UPickupText*> PickupTexts;

	float PickupTextBaseYPosition = 175.f;
	float PickupTextBaseHeight = 40.f;

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

	UPROPERTY(EditAnywhere, Category = "Announcements")
	TSubclassOf<class UEliminationAnnouncement> EliminationAnnouncementClass;

	UPROPERTY(EditAnywhere)
	float EliminationAnnouncementTime = 5.f;

	UFUNCTION()
	void EliminationAnnouncementTimerFinished(UEliminationAnnouncement* MessageToRemove);

	UPROPERTY()
	TArray<UEliminationAnnouncement*> EliminationMessages;
	

	UPROPERTY(EditAnywhere, Category = "Chat")
	TSubclassOf<class UChatMessage> ChatMessageClass;

	UPROPERTY(EditAnywhere)
	float ChatMessageTime = 5.f;

	UFUNCTION()
	void ChatMessageTimerFinished(UChatMessage* MessageToRemove);

	UPROPERTY()
	TArray<UChatMessage*> ChatMessages;

	UPROPERTY(EditAnywhere, Category = "Chat")
	TSubclassOf<class UChatInput> ChatInputClass;

	UPROPERTY()
	UChatInput* ChatInput = nullptr;

	void AddChatInput();

public:
	FORCEINLINE void SetHUDPackage(const FHUDPackage& Package) { HUDPackage = Package; }
	FORCEINLINE UAnnouncement* GetAnnouncement() const { return Announcement; }
	FORCEINLINE UCharacterOverlay* GetCharacterOverlay() const { return CharacterOverlay; }
	FORCEINLINE UChatInput* GetChatInput() const { return ChatInput; }
};
