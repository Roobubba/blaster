// Fill out your copyright notice in the Description page of Project Settings.


#include "ChatMessage.h"
#include "Components/TextBlock.h"

void UChatMessage::SetChatMessageText(const FString& ChatMessage)
{
    if (ChatMessageText)
    {
        ChatMessageText->SetText(FText::FromString(ChatMessage));
    }
}