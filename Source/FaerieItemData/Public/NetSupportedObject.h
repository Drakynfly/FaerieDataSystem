// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "UObject/Object.h"
#include "Net/Core/PushModel/PushModelMacros.h"
#include "NetSupportedObject.generated.h"


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

	// Implement this to add internal subobjects for replication
	virtual void AddSubobjectsForReplication(AActor* Actor) {}
};

/**
 * A simple replicated UObject. Must be owned by an actor, or it'll complain.
 */
UCLASS(Abstract)
class FAERIEITEMDATA_API UActorSubobjectBase : public UNetSupportedObject
{
	GENERATED_BODY()

public:
	//~ UObject
	virtual UWorld* GetWorld() const override;

	virtual int32 GetFunctionCallspace(UFunction* Function, FFrame* Stack) override;
	virtual bool CallRemoteFunction(UFunction* Function, void* Parms, struct FOutParmRec* OutParms, FFrame* Stack) override;

	virtual void PostInitProperties() override;
	//~ UObject

	AActor* GetOwner() const;
	AActor* GetOwnerChecked() const;
};