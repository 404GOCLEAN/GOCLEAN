// Fill out your copyright notice in the Description page of Project Settings.


#include "GMapSystem/Server/GMapManager.h"

#include "GMapSystem/GDoorway.h"
#include "GDataManagerSubsystem.h"
#include "GTypes/DataTableRow/GMapDataRow.h"
#include "../../../GOCLEAN.h"


////////////////////////////////////////////
// DEFAULT
////////////////////////////////////////////

// Sets default values for this component's properties
UGMapManager::UGMapManager()
{
	// 
}


// Called when the game starts
void UGMapManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UE_LOG(LogMap, Log, TEXT("[GMapManager] Initialized."));

	UGameInstance* GameInstance = GetWorld()->GetGameInstance();
	if (!GameInstance) return;

	auto* DataManager = UGDataManagerSubsystem::Get(GetWorld());;
	if (!DataManager) return;

	TArray<FGMapDataRow*> Datas = DataManager->GetAllMapDatas();
	for (FGMapDataRow* Data : Datas)
	{
		if (Data)
		{
			FName ZoneName = Data->ZoneID;
			if (!ZoneDatas.Contains(ZoneName))
			{
				FGZoneData NewZoneData;

				NewZoneData.ZID = ZoneName;
				NewZoneData.Type = Data->Type;
				NewZoneData.Attribute = Data->Attribute;

				ZoneDatas.Add(ZoneName, NewZoneData);
			}
		}
	}

	UE_LOG(LogMap, Log, TEXT("[GMapManager] Complete setting zone datas : %d numbers of zone"), ZoneDatas.Num());
}

// Called when the game starts
void UGMapManager::Deinitialize()
{
	Super::Deinitialize();
	ZoneDatas.Empty();

	// ...

}


void UGMapManager::PlayerEnteredZone(FName ZoneName, AActor* PlayerActor)
{
	if (ZoneName.IsNone() || !ZoneDatas.Contains(ZoneName) || !PlayerActor) return;

	ZoneDatas[ZoneName].IncludedPlayerList.AddUnique(PlayerActor);
}

void UGMapManager::PlayerExitedZone(FName ZoneName, AActor* PlayerActor)
{
	if (ZoneName.IsNone() || !ZoneDatas.Contains(ZoneName) || !PlayerActor) return;

	ZoneDatas[ZoneName].IncludedPlayerList.Remove(PlayerActor);
}

void UGMapManager::GhostEnteredZone(FName ZoneName, AActor* GhostActor)
{
	if (ZoneName.IsNone() || !ZoneDatas.Contains(ZoneName) || !GhostActor) return;

	ZoneDatas[ZoneName].bIncludeGhost = true;
}

void UGMapManager::GhostExitedZone(FName ZoneName, AActor* GhostActor)
{
	if (ZoneName.IsNone() || !ZoneDatas.Contains(ZoneName) || !GhostActor) return;

	ZoneDatas[ZoneName].bIncludeGhost = false;
}

const FGZoneData* UGMapManager::GetZoneData(FName ZoneName) const
{
	return ZoneDatas.Find(ZoneName);
}



bool UGMapManager::IsActorInZoneType(const AActor* Actor, EGZoneType ZoneType) const
{
	if (!Actor) return false;

	for (const TPair<FName, FGZoneData>& Pair : ZoneDatas)
	{
		const FGZoneData& Data = Pair.Value;
		if (Data.Type != ZoneType) continue;

		if (Data.IncludedPlayerList.Contains(const_cast<AActor*>(Actor)))
		{
			return true;
		}
	}
	return false;
}

TArray<FName> UGMapManager::GetActorZoneIds(const AActor* Actor) const
{
	TArray<FName> Out;
	if (!Actor) return Out;

	for (const TPair<FName, FGZoneData>& Pair : ZoneDatas)
	{
		const FGZoneData& Data = Pair.Value;
		if (Data.IncludedPlayerList.Contains(const_cast<AActor*>(Actor)))
		{
			Out.Add(Pair.Key);
		}
	}
	return Out;
}


const TArray<FGZoneData> UGMapManager::GetZoneDatasByAttribute(EGZoneAttribute ZoneAttribute) const
{
	TArray<FGZoneData> TargetZones;

	for (const TPair<FName, FGZoneData>& Pair : ZoneDatas)
	{
		const FGZoneData& Data = Pair.Value;
		if (Data.Attribute != ZoneAttribute) continue;

		TargetZones.Add(Data);
	}

	return TargetZones;
}
