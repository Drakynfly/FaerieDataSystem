// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "UObject/Object.h"
#include "Net/Core/PushModel/PushModelMacros.h"
#include "NetSupportedObject.generated.h"

class AActor;

/**
 * A simple replicated UObject.
 */
UCLASS(Abstract)
class FAERIEITEMDATA_API UNetSupportedObject : public UObject
{
	GENERATED_BODY()

	// We have to add this ourself, because Unreal cannot detect that a class is supposed to be replicated when it has no replicated properties.
	REPLICATED_BASE_CLASS(UNetSupportedObject)

public:
	// Enable full networking
	virtual bool IsSupportedForNetworking() const override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

#if UE_WITH_IRIS
	// Register replication fragments
	virtual void RegisterReplicationFragments(UE::Net::FFragmentRegistrationContext& Context, UE::Net::EFragmentRegistrationFlags RegistrationFlags) override;
#endif

	// This should be called during ReadyForReplication / BeginPlay, or a similar "startup" location, to allow subclasses
	// to initialize their themselves/any subobjects. Usually this means adding subobjects for replication to Actor.
	virtual void InitializeNetObject(TNotNull<AActor*> Actor) {}

	// This should be called during EndPlay, or a similar "shutdown" location, to allow subclasses to deinitialize
	// themselves/any subobjects, or when switching owners (if pairing with a new InitializeNetObject call).
	virtual void DeinitializeNetObject(TNotNull<AActor*> Actor) {}
};