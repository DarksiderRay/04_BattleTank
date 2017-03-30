// Fill out your copyright notice in the Description page of Project Settings.

#include "BattleTank.h"
#include "TankPlayerController.h"
#include "TankAimingComponent.h"
#include "Tank.h"	// So we can implement OnDeath

void ATankPlayerController::BeginPlay()
{
	Super::BeginPlay();
	auto AimingComponent = GetPawn()->FindComponentByClass<UTankAimingComponent>();
	if (ensure(AimingComponent))
	{
		FoundAimingComponent(AimingComponent);
	}
}

void ATankPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	AimTowardsCrosshair();
}

void ATankPlayerController::SetPawn(APawn * InPawn)
{
	Super::SetPawn(InPawn);
	if (InPawn)
	{
		auto PossessedTank = Cast<ATank>(InPawn);
		if (!ensure(PossessedTank)) { return; }

		// Subscribe our local method to the tank's death event
		PossessedTank->OnDeath.AddUniqueDynamic(this, &ATankPlayerController::OnPossessedTankDeath);
	}
}

void ATankPlayerController::AimTowardsCrosshair()
{
	if (!GetPawn()) return; // e.g. if not possessing
	auto AimingComponent = GetPawn()->FindComponentByClass<UTankAimingComponent>();
	if (!ensure(AimingComponent)) return;

	FVector HitLocation; // out parameter
	bool bGotHitLocation = GetSightRayHitLocation(HitLocation);
	// Get world location if linetrace through crosshair
	if (bGotHitLocation)	// Has "side-effect", is going to line trace
	{
		AimingComponent->AimAt(HitLocation);
	}
}

// Get world location if linetrace through crosshair, true if it hit landscape
bool ATankPlayerController::GetSightRayHitLocation(FVector& HitLocation) const
{
	// Find the crosshair position
	int32 ViewportSizeX, ViewportSizeY;
	GetViewportSize(ViewportSizeX, ViewportSizeY);

	auto ScreenLocation = FVector2D(ViewportSizeX * CrossHairXLocation, ViewportSizeY * CrossHairYLocation);

	// "De-project" the screen position of the crosshair to a world direction
	FVector LookDirection;
	if (GetLookDirection(ScreenLocation, LookDirection))
	{
		// Line-trace along that look direction, and see what it hit (up to max range)
		return GetLookVectorHitLocation(LookDirection, HitLocation);
	}


	return false;
}

bool ATankPlayerController::GetLookDirection(FVector2D ScreenLocation, FVector& LookDirection) const
{
	FVector CameraWorldLocation;	// To be discarded
	return DeprojectScreenPositionToWorld(
		ScreenLocation.X,
		ScreenLocation.Y,
		CameraWorldLocation,
		LookDirection);
}

bool ATankPlayerController::GetLookVectorHitLocation(FVector LookDirection, FVector& HitLocation) const
{
	FHitResult HitResult;
	auto StartLocation = PlayerCameraManager->GetCameraLocation();
	auto EndLocation = StartLocation + (LookDirection * LineTraceRange);
	if (GetWorld()->LineTraceSingleByChannel(
			HitResult,
			StartLocation,
			EndLocation,
			ECollisionChannel::ECC_Visibility))
	{
		HitLocation = HitResult.Location;
		return true;
	}
	HitLocation = FVector(0);
	return false;
}

void ATankPlayerController::OnPossessedTankDeath()
{
	StartSpectatingOnly();
}
