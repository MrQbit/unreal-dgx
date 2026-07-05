#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PongBall.generated.h"

class UStaticMeshComponent;
class APongPaddle;
class APongGameMode;

UCLASS()
class APongBall : public AActor
{
	GENERATED_BODY()
public:
	APongBall();
	virtual void Tick(float DeltaSeconds) override;

	void Setup(APongPaddle* InLeft, APongPaddle* InRight, APongGameMode* InMode)
	{ Left = InLeft; Right = InRight; Mode = InMode; }
	void ResetBall(int32 Dir);

private:
	UPROPERTY() UStaticMeshComponent* Mesh = nullptr;
	UPROPERTY() APongPaddle* Left = nullptr;
	UPROPERTY() APongPaddle* Right = nullptr;
	UPROPERTY() APongGameMode* Mode = nullptr;
	FVector Velocity = FVector::ZeroVector;
	uint32  RngState = 0x1234567u;
	float   FastRand01();
};
