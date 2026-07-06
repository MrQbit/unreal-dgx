// Copyright MrQbit. Batch GameForge assembly commandlet — see DGXForgeCommandlet.cpp.
#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "DGXForgeCommandlet.generated.h"

UCLASS()
class UDGXForgeCommandlet : public UCommandlet
{
	GENERATED_BODY()
public:
	virtual int32 Main(const FString& Params) override;
};
