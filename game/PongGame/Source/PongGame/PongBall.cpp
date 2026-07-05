#include "PongBall.h"
#include "PongPaddle.h"
#include "PongGameMode.h"
#include "PongField.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

APongBall::APongBall()
{
	PrimaryActorTick.bCanEverTick = true;
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (Sphere.Succeeded()) { Mesh->SetStaticMesh(Sphere.Object); }
	Mesh->SetWorldScale3D(FVector(PongField::BallRadius * 2.f / 100.f));
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

float APongBall::FastRand01()
{
	// xorshift — deterministic, no engine RNG needed (Math random is fine too, this avoids seeding).
	RngState ^= RngState << 13; RngState ^= RngState >> 17; RngState ^= RngState << 5;
	return (RngState & 0xFFFFFF) / float(0x1000000);
}

void APongBall::ResetBall(int32 Dir)
{
	SetActorLocation(FVector::ZeroVector);
	const float Angle = (FastRand01() - 0.5f) * (PI * 0.5f);           // +/- 45 deg
	Velocity = FVector((Dir >= 0 ? 1.f : -1.f) * FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * PongField::BallSpeed;
}

void APongBall::Tick(float Dt)
{
	Super::Tick(Dt);
	FVector P = GetActorLocation() + Velocity * Dt;

	// bounce off top / bottom walls
	if (P.Y >  PongField::HalfHeight - PongField::BallRadius) { P.Y =  PongField::HalfHeight - PongField::BallRadius; Velocity.Y = -Velocity.Y; }
	if (P.Y < -PongField::HalfHeight + PongField::BallRadius) { P.Y = -PongField::HalfHeight + PongField::BallRadius; Velocity.Y = -Velocity.Y; }

	// paddle bounce (simple: near a paddle's X and within its Y span, moving toward it)
	auto TryPaddle = [&](APongPaddle* Pad, float PadX, bool bLeftSide)
	{
		if (!Pad) return;
		const float py = Pad->GetActorLocation().Y;
		const bool bTowards = bLeftSide ? (Velocity.X < 0.f) : (Velocity.X > 0.f);
		if (bTowards && FMath::Abs(P.X - PadX) < (PongField::BallRadius + 15.f)
			&& FMath::Abs(P.Y - py) < (Pad->GetHalfLen() + PongField::BallRadius))
		{
			Velocity.X = -Velocity.X;
			Velocity.Y += (P.Y - py) * 4.f;                 // add "english" based on hit offset
			Velocity = Velocity.GetSafeNormal() * PongField::BallSpeed;
			P.X = PadX + (bLeftSide ? 1.f : -1.f) * (PongField::BallRadius + 15.f);
		}
	};
	TryPaddle(Left,  -PongField::PaddleX, true);
	TryPaddle(Right,  PongField::PaddleX, false);

	// scoring
	if (P.X >  PongField::HalfWidth) { if (Mode) Cast<APongGameMode>(Mode)->OnScore(true);  return; }
	if (P.X < -PongField::HalfWidth) { if (Mode) Cast<APongGameMode>(Mode)->OnScore(false); return; }

	P.Z = 0.f;
	SetActorLocation(P);
}
