#include "PongPaddle.h"
#include "PongBall.h"
#include "PongField.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InputComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

APongPaddle::APongPaddle()
{
	PrimaryActorTick.bCanEverTick = true;
	HalfLen = PongField::PaddleHalfLen;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (Cube.Succeeded()) { Mesh->SetStaticMesh(Cube.Object); }
	// 1 uu cube -> paddle: thin in X, long in Y, short in Z
	Mesh->SetWorldScale3D(FVector(0.3f, PongField::PaddleHalfLen * 2.f / 100.f, 0.6f));
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AutoPossessPlayer = EAutoReceiveInput::Disabled;
}

void APongPaddle::MoveInput(float Value) { AxisInput = FMath::Clamp(Value, -1.f, 1.f); }

void APongPaddle::SetupPlayerInputComponent(UInputComponent* Input)
{
	Super::SetupPlayerInputComponent(Input);
	if (Input) { Input->BindAxis(TEXT("PaddleMove"), this, &APongPaddle::MoveInput); }
}

void APongPaddle::Tick(float Dt)
{
	Super::Tick(Dt);
	FVector P = GetActorLocation();
	float Target = AxisInput;
	if (bIsAI && Ball) // simple AI: track the ball's Y
	{
		const float dy = Ball->GetActorLocation().Y - P.Y;
		Target = FMath::Clamp(dy / 60.f, -1.f, 1.f);
	}
	P.Y = FMath::Clamp(P.Y + Target * Speed * Dt, -PongField::HalfHeight + HalfLen, PongField::HalfHeight - HalfLen);
	P.X = (P.X >= 0.f) ? PongField::PaddleX : -PongField::PaddleX;
	P.Z = 0.f;
	SetActorLocation(P);
}
