// Fill out your copyright notice in the Description page of Project Settings.


#include "ChatMessage.h"
#include "Components/TextBlock.h"

void UChatMessage::SetChatMessageText(FString Sender, FString ChatMessage)
{
    if (ChatMessageText)
    {
        FString ChatText = FString::Printf(TEXT("%s: %s"), *Sender, *ChatMessage);
        ChatMessageText->SetText(FText::FromString(ChatText));
    }
}