#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "PongGameMode.generated.h"

class APongPaddle;
class APongBall;

UCLASS()
class APongGameMode : public AGameModeBase
{
	GENERATED_BODY()
public:
	APongGameMode();
	virtual void BeginPlay() override;

	void OnScore(bool bLeftScored);

private:
	UPROPERTY() APongPaddle* Left = nullptr;
	UPROPERTY() APongPaddle* Right = nullptr;
	UPROPERTY() APongBall*   Ball = nullptr;
	int32 ScoreL = 0;
	int32 ScoreR = 0;

	void SpawnWall(float Y);
};
