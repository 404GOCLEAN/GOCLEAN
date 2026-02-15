// Fill out your copyright notice in the Description page of Project Settings.

#include "ServerModule/GameSession/GameSessionInstance.h"
#include "ServerModule/Voice/VoiceChattingManager.h"



static float Clamp01(float v) { return FMath::Clamp(v, 0.f, 1.f); }

void UGameSessionInstance::Voice_UpdateCache(bool bJoined, float EnergyRaw01, float LevelCorrected01, bool bMicMuted, const TMap<int32, TPair<FVector, float>>& InStimuliMap)
{
    bVoiceJoined = bJoined;
    VoiceEnergyRaw01 = EnergyRaw01;
    VoiceLevelCorrected01 = LevelCorrected01;
    bVoiceMicMuted = bMicMuted;
    VoiceStimuliMap = InStimuliMap;
}
