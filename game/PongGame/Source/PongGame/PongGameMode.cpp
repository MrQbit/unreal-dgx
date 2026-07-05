#include "PongGameMode.h"
#include "PongPaddle.h"
#include "PongBall.h"
#include "PongField.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"

APongGameMode::APongGameMode()
{
	// No default pawn — we spawn and possess the player's paddle ourselves.
	DefaultPawnClass = nullptr;
	PlayerControllerClass = APlayerController::StaticClass();
}

void APongGameMode::SpawnWall(float Y)
{
	UWorld* W = GetWorld();
	AStaticMeshActor* Wall = W->SpawnActor<AStaticMeshActor>(FVector(0.f, Y, 0.f), FRotator::ZeroRotator);
	if (!Wall) return;
	Wall->SetMobility(EComponentMobility::Movable);
	if (UStaticMeshComponent* MC = Wall->GetStaticMeshComponent())
	{
		static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
		if (Cube.Succeeded()) { MC->SetStaticMesh(Cube.Object); }
		MC->SetWorldScale3D(FVector(PongField::HalfWidth * 2.f / 100.f, 0.2f, 0.4f));
		MC->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void APongGameMode::BeginPlay()
{
	Super::BeginPlay();
	UWorld* W = GetWorld();
	if (!W) return;

	// top-down camera
	ACameraActor* Cam = W->SpawnActor<ACameraActor>(FVector(0.f, 0.f, 1500.f), FRotator(-90.f, 0.f, 0.f));
	APlayerController* PC = W->GetFirstPlayerController();
	if (Cam && PC) { PC->SetViewTarget(Cam); }

	SpawnWall( PongField::HalfHeight);
	SpawnWall(-PongField::HalfHeight);

	Left  = W->SpawnActor<APongPaddle>(FVector(-PongField::PaddleX, 0.f, 0.f), FRotator::ZeroRotator);
	Right = W->SpawnActor<APongPaddle>(FVector( PongField::PaddleX, 0.f, 0.f), FRotator::ZeroRotator);
	if (Left)  { Left->SetAI(false, nullptr); }
	if (PC && Left) { PC->Possess(Left); }

	Ball = W->SpawnActor<APongBall>(FVector::ZeroVector, FRotator::ZeroRotator);
	if (Right && Ball) { Right->SetAI(true, Ball); }
	if (Ball) { Ball->Setup(Left, Right, this); Ball->ResetBall(1); }

	UE_LOG(LogTemp, Display, TEXT("PONG: ready — WASD/Arrows move the left paddle. First to score wins a point."));
}

void APongGameMode::OnScore(bool bLeftScored)
{
	if (bLeftScored) { ++ScoreL; } else { ++ScoreR; }
	UE_LOG(LogTemp, Display, TEXT("PONG: score  Left %d : %d Right"), ScoreL, ScoreR);
	if (Ball) { Ball->ResetBall(bLeftScored ? -1 : 1); }
}
