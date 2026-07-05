#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "PongPaddle.generated.h"

class UStaticMeshComponent;
class APongBall;

UCLASS()
class APongPaddle : public APawn
{
	GENERATED_BODY()
public:
	APongPaddle();
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* Input) override;

	void MoveInput(float Value);      // player axis
	void SetAI(bool bAI, APongBall* InBall) { bIsAI = bAI; Ball = InBall; }
	float GetHalfLen() const { return HalfLen; }

private:
	UPROPERTY() UStaticMeshComponent* Mesh = nullptr;
	UPROPERTY() APongBall* Ball = nullptr;
	bool  bIsAI = false;
	float AxisInput = 0.f;
	float Speed = 1100.f;
	float HalfLen = 90.f;
};
