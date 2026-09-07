// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"

#include "GTypes/GMapTypes.h"

#include "GMapManager.generated.h"


class AGDoorway;


UCLASS()
class GOCLEAN_API UGMapManager : public UWorldSubsystem
{
	GENERATED_BODY()



	// constructor
public:
	UGMapManager();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// -------------------------------
	//		IN-DOORWAY
	// -------------------------------
	void PlayerEnteredZone(FName ZoneName, AActor* PlayerActor);
	void PlayerExitedZone(FName ZoneName, AActor* PlayerActor);

	void GhostEnteredZone(FName ZoneName, AActor* GhostActor);
	void GhostExitedZone(FName ZoneName, AActor* GhostActor);

	const FGZoneData* GetZoneData(FName ZoneName) const;

	const TArray<FGZoneData> GetZoneDatasByAttribute(EGZoneAttribute ZoneAttribute) const;

	
protected:
	UPROPERTY()
	TMap<FName, FGZoneData> ZoneDatas;

public:
	// 특정 Actor가 현재 어떤 ZoneType에 포함되어 있는지
	UFUNCTION(BlueprintCallable)
	bool IsActorInZoneType(const AActor* Actor, EGZoneType ZoneType) const;

	// 현재 Actor가 포함된 ZoneID 목록
	UFUNCTION(BlueprintCallable)
	TArray<FName> GetActorZoneIds(const AActor* Actor) const;

};
