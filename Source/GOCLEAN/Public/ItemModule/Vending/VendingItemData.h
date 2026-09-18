// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "VendingItemData.generated.h"


/**
 * 벤딩 아이템 DataTable Row
 *
 * 로비 구매 UI / 인게임 벤딩 머신 / 아이템 지급 시스템에서
 * 공통으로 사용할 벤딩 아이템의 기본 데이터
 */
USTRUCT(BlueprintType)
struct GOCLEAN_API FVendingItemData : public FTableRowBase
{
    GENERATED_BODY()


    // ========================================
    // Identity
    // ========================================

    // 벤딩 시스템에서 사용하는 고유 ID
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vending|Identity")
    int32 VendingItemId = -1;


    // 화면에 표시할 아이템 이름
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vending|Identity")
    FText ItemName;


    // ========================================
    // UI
    // ========================================

    // 로비 / 벤딩 머신에서 사용할 아이콘
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vending|UI")
    TObjectPtr<UTexture2D> Icon = nullptr;


    // 아이템 설명
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vending|UI",
        meta = (MultiLine = "true"))
    FText Description;


    // UI에서 정렬할 순서
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vending|UI")
    int32 SortOrder = 0;


    // ========================================
    // Purchase
    // ========================================

    // 구매 가격
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vending|Purchase",
        meta = (ClampMin = "0"))
    int32 Price = 0;


    // 해당 종류를 한 번 구매했을 때 지급되는 실제 아이템 개수
    //
    // 무한대면 -1
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vending|Purchase")
    int32 GrantedQuantity = 1;


    // 현재 벤딩 시스템에서 판매할지 여부
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vending|Purchase")
    bool bAvailableInVending = true;


    // ========================================
    // Install
    // ========================================

    // 바닥 등에 설치할 수 있는 아이템인지
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vending|Install")
    bool bInstallable = false;


    // 설치 가능한 최대 개수
    //
    // 설치 불가능한 아이템이면 0
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vending|Install",
        meta = (ClampMin = "0"))
    int32 MaxInstallCount = 0;


    // ========================================
    // Existing Item System Link
    // ========================================

    // 기존 DT_Equipment의 아이템 ID
    //
    // 아직 기존 아이템 시스템과 연결되지 않았다면 -1
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vending|Link")
    int32 EquipmentItemId = -1;


    // ========================================
    // Ghost
    // ========================================

    // 관련 귀신.
    // 아직 귀신 데이터 구조가 확정되지 않았으므로
    // 이름/ID 용도로 FName 사용.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vending|Ghost")
    TArray<FName> RelatedGhosts;
};