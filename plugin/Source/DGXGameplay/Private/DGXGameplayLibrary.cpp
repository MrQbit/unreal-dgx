#include "DGXGameplayLibrary.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"
#include "Engine/World.h"
#include "Engine/EngineTypes.h"
#include "CollisionQueryParams.h"
#include "Kismet/GameplayStatics.h"

FVector UDGXGameplayLibrary::GetCameraForward(APawn* Pawn)
{
	if (!Pawn) { return FVector::ForwardVector; }
	const FRotator YawOnly(0.f, Pawn->GetControlRotation().Yaw, 0.f);
	return YawOnly.Vector();
}

FVector UDGXGameplayLibrary::GetCameraRight(APawn* Pawn)
{
	if (!Pawn) { return FVector::RightVector; }
	const FRotator YawOnly(0.f, Pawn->GetControlRotation().Yaw, 0.f);
	return FRotationMatrix(YawOnly).GetUnitAxis(EAxis::Y);
}

AActor* UDGXGameplayLibrary::FireHitscan(AActor* Shooter, float Range, float Damage)
{
	if (!Shooter) { return nullptr; }
	UWorld* World = Shooter->GetWorld();
	if (!World) { return nullptr; }

	FVector Start; FRotator ViewRot;
	if (APawn* Pawn = Cast<APawn>(Shooter))
	{
		Pawn->GetActorEyesViewPoint(Start, ViewRot);
		ViewRot = Pawn->GetControlRotation();
	}
	else
	{
		Start = Shooter->GetActorLocation();
		ViewRot = Shooter->GetActorRotation();
	}
	const FVector End = Start + ViewRot.Vector() * Range;

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Shooter);
	if (World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
	{
		if (AActor* HitActor = Hit.GetActor())
		{
			UGameplayStatics::ApplyDamage(HitActor, Damage, Shooter->GetInstigatorController(), Shooter, nullptr);
			return HitActor;
		}
	}
	return nullptr;
}

void UDGXGameplayLibrary::ChasePlayer(AActor* Enemy, float Speed)
{
	if (!Enemy) { return; }
	UWorld* World = Enemy->GetWorld();
	if (!World) { return; }
	APawn* Player = UGameplayStatics::GetPlayerPawn(Enemy, 0);
	if (!Player) { return; }

	FVector Dir = Player->GetActorLocation() - Enemy->GetActorLocation();
	Dir.Z = 0.f;                         // horizontal chase only
	Dir = Dir.GetSafeNormal();
	Enemy->AddActorWorldOffset(Dir * Speed * World->GetDeltaSeconds(), /*bSweep=*/true);
}
