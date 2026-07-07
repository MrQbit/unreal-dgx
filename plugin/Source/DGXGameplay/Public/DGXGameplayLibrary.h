// Copyright MrQbit. Runtime FPS gameplay helpers, called from generated Blueprint graphs so the
// hard logic lives in tested C++ (and ships in the packaged game) while recipes just wire input->call.
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "DGXGameplayLibrary.generated.h"

UCLASS()
class DGXGAMEPLAY_API UDGXGameplayLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Yaw-only forward vector from the pawn's control rotation — camera-relative ground movement
	 *  (pitch zeroed so looking up/down doesn't drive you into the floor/sky). */
	UFUNCTION(BlueprintPure, Category = "DGX|FPS")
	static FVector GetCameraForward(APawn* Pawn);

	/** Yaw-only right vector from the pawn's control rotation. */
	UFUNCTION(BlueprintPure, Category = "DGX|FPS")
	static FVector GetCameraRight(APawn* Pawn);

	/** Hitscan from the shooter's eye viewpoint: line-trace out to Range on the Visibility channel and
	 *  apply Damage to whatever is hit. Returns the hit actor (or null). */
	UFUNCTION(BlueprintCallable, Category = "DGX|FPS")
	static AActor* FireHitscan(AActor* Shooter, float Range = 10000.f, float Damage = 20.f);
};
