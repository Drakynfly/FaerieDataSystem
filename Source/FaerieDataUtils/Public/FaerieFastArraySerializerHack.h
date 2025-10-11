// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "Net/Serialization/FastArraySerializer.h"

namespace Faerie::Hacks
{
	/** Struct containing common header data that is written / read when serializing Fast Arrays. */
	struct FFaerieFastArraySerializerHeader
	{
		/** The current ArrayReplicationKey. */
		int32 ArrayReplicationKey;

		/** The previous ArrayReplicationKey. */
		int32 BaseReplicationKey;

		/** The number of changed elements (adds or removes). */
		int32 NumChanged;

		/**
		 * The list of deleted elements.
		 * When writing, this will be treated as IDs that are translated to indices prior to serialization.
		 * When reading, this will be actual indices.
		 */
		TArray<int32, TInlineAllocator<8>> DeletedIndices;
	};

	template<typename Type, typename SerializerType>
	struct TFaerieFastArraySerializeHelper
	{
		/** Array element type struct. */
		UScriptStruct* Struct;

		/** Set of array elements we're serializing. */
		TArray<Type>& Items;

		/** The actual FFastArraySerializer struct we're serializing. */
		SerializerType& ArraySerializer;

		/** Cached DeltaSerialize params. */
		FNetDeltaSerializeInfo& Parms;

		void ConditionalRebuildItemMap()
		{
			if ((Parms.bUpdateUnmappedObjects || Parms.Writer == NULL) && ArraySerializer.ItemMap.Num() != Items.Num())
			{
				//SCOPE_CYCLE_COUNTER(STAT_NetSerializeFastArray_BuildMap);
				//UE_LOG(LogNetFastTArray, Log, TEXT("FastArrayDeltaSerialize: Recreating Items map. Struct: %s, Items.Num: %d Map.Num: %d"), *Struct->GetOwnerStruct()->GetName(), Items.Num(), ArraySerializer.ItemMap.Num());
				ArraySerializer.ItemMap.Empty();
				for (int32 i = 0; i < Items.Num(); ++i)
				{
					if (Items[i].ReplicationID == INDEX_NONE)
					{
						if (Parms.Writer)
						{
							//UE_LOG(LogNetFastTArray, Warning, TEXT("FastArrayDeltaSerialize: Item with uninitialized ReplicationID. Struct: %s, ItemIndex: %i"), *Struct->GetOwnerStruct()->GetName(), i);
						}
						else
						{
							// This is benign for clients, they may add things to their local array without assigning a ReplicationID
							continue;
						}
					}

					ArraySerializer.ItemMap.Add(Items[i].ReplicationID, i);
				}
			}
		}

		bool ReadDeltaHeader(FFaerieFastArraySerializerHeader& Header) const
		{
			FBitReader& Reader = *Parms.Reader;

			//---------------
			// Read header
			//---------------

			Reader << Header.ArrayReplicationKey;
			Reader << Header.BaseReplicationKey;

			int32 NumDeletes = 0;
			Reader << NumDeletes;

			UE_LOG(LogNetFastTArray, Log, TEXT("Received [%d/%d]."), Header.ArrayReplicationKey, Header.BaseReplicationKey);

			const int32 MaxNumDeleted = FFastArraySerializer::GetMaxNumberOfAllowedDeletionsPerUpdate();
			if (NumDeletes > MaxNumDeleted)
			{
				UE_LOG(LogNetFastTArray, Warning, TEXT("NumDeletes > GetMaxNumberOfAllowedDeletionsPerUpdate: %d > %d. (Read)"), NumDeletes, MaxNumDeleted);
				Reader.SetError();
				return false;
			}

			Reader << Header.NumChanged;

			const int32 MaxNumChanged = FFastArraySerializer::GetMaxNumberOfAllowedChangesPerUpdate();
			if (Header.NumChanged > MaxNumChanged)
			{
				UE_LOG(LogNetFastTArray, Warning, TEXT("NumChanged > GetMaxNumberOfAllowedChangesPerUpdate: %d > %d. (Read)"), Header.NumChanged, MaxNumChanged);
				Reader.SetError();
				return false;
			}

			UE_LOG(LogNetFastTArray, Log, TEXT("Read NumChanged: %d NumDeletes: %d."), Header.NumChanged, NumDeletes);

			//---------------
			// Read deleted elements
			//---------------
			if (NumDeletes > 0)
			{
				for (int32 i = 0; i < NumDeletes; ++i)
				{
					int32 ElementID = 0;
					Reader << ElementID;

					int32* ElementIndexPtr = ArraySerializer.ItemMap.Find(ElementID);
					if (ElementIndexPtr)
					{
						int32 DeleteIndex = *ElementIndexPtr;
						Header.DeletedIndices.Add(DeleteIndex);
						UE_LOG(LogNetFastTArray, Log, TEXT("   Adding ElementID: %d for deletion"), ElementID);
					}
					else
					{
						UE_LOG(LogNetFastTArray, Log, TEXT("   Couldn't find ElementID: %d for deletion!"), ElementID);
					}
				}
			}

			return true;
		}

		template<typename GuidMapType>
		void PostReceiveCleanup(
			FFaerieFastArraySerializerHeader& Header,
			TArray<int32, TInlineAllocator<8>>& ChangedIndices,
			TArray<int32, TInlineAllocator<8>>& AddedIndices,
			GuidMapType& GuidMap)
		{
			if (Items.Num() > 0)
			{
				// Cache Old Position Data
				TMap<int32, int32> OldIndexToReplicationIDMap;
				OldIndexToReplicationIDMap.Reserve(Items.Num());
				for (int32 i = 0; i < Items.Num(); ++i)
				{
					OldIndexToReplicationIDMap.Add(i, Items[i].ReplicationID);
				}
				
				Algo::SortBy(Items, &Type::Key);

				// Build a Map For Replication id to new index
				TMap<int32, int32> NewIndexMap;
				NewIndexMap.Reserve(Items.Num());
				for (int32 i = 0; i < Items.Num(); ++i)
				{
					if (Items[i].ReplicationID != INDEX_NONE)
					{
						NewIndexMap.Add(Items[i].ReplicationID, i);
					}
				}
				
				auto UpdateIndices = [&](TArray<int32, TInlineAllocator<8>>& Indices)
				{
					for (int32& Idx : Indices)
					{
						if (OldIndexToReplicationIDMap.Contains(Idx)) // Ensure the old index is valid
						{
							const int32 ReplicationID = OldIndexToReplicationIDMap[Idx];
							if (NewIndexMap.Contains(ReplicationID))
							{
								Idx = NewIndexMap[ReplicationID];
							}
						}
					}
				};

				UpdateIndices(ChangedIndices);
				UpdateIndices(AddedIndices);
				UpdateIndices(Header.DeletedIndices);
				
				ArraySerializer.ItemMap = MoveTemp(NewIndexMap);
			}

			
			//CSV_SCOPED_TIMING_STAT(Networking, FastArray_Apply);

			// ---------------------------------------------------------
			// Look for implicit deletes that would happen due to Naks
			// ---------------------------------------------------------

			// If we're sending data completely reliably, there's no need to do this.
			if (!Parms.bInternalAck)
			{
				for (int32 idx = 0; idx < Items.Num(); ++idx)
				{
					Type& Item = Items[idx];
					if (Item.MostRecentArrayReplicationKey < Header.ArrayReplicationKey && Item.MostRecentArrayReplicationKey > Header.BaseReplicationKey)
					{
						// Make sure this wasn't an explicit delete in this bunch (otherwise we end up deleting an extra element!)
						if (!Header.DeletedIndices.Contains(idx))
						{
							// This will happen in normal conditions in network replays.
							//UE_LOG(LogNetFastTArray, Log, TEXT("Adding implicit delete for ElementID: %d. MostRecentArrayReplicationKey: %d. Current Payload: [%d/%d]"),
							//	Item.ReplicationID, Item.MostRecentArrayReplicationKey, Header.ArrayReplicationKey, Header.BaseReplicationKey);

							Header.DeletedIndices.Add(idx);
						}
					}
				}
			}

			// Increment keys so that a client can re-serialize the array if needed, such as for client replay recording.
			// Must check the size of DeleteIndices instead of NumDeletes to handle implicit deletes.
			if (Header.DeletedIndices.Num() > 0 || Header.NumChanged > 0)
			{
				ArraySerializer.IncrementArrayReplicationKey();
			}

			// ---------------------------------------------------------
			// Invoke all callbacks: removed -> added -> changed
			// ---------------------------------------------------------

			int32 PreRemoveSize = Items.Num();
			int32 FinalSize = PreRemoveSize - Header.DeletedIndices.Num();
			for (int32 idx : Header.DeletedIndices)
			{
				if (Items.IsValidIndex(idx))
				{
					// Remove the deleted element's tracked GUID references
					if (GuidMap.Remove(Items[idx].ReplicationID) > 0)
					{
						Parms.bGuidListsChanged = true;
					}

					// Call the delete callbacks now, actually remove them at the end
					Items[idx].PreReplicatedRemove(ArraySerializer);
				}
			}
			ArraySerializer.PreReplicatedRemove(Header.DeletedIndices, FinalSize);

			if (PreRemoveSize != Items.Num())
			{
				//UE_LOG(LogNetFastTArray, Error, TEXT("Item size changed after PreReplicatedRemove! PremoveSize: %d  Item.Num: %d"),
				//	PreRemoveSize, Helper.Items.Num());
			}

			for (int32 idx : AddedIndices)
			{
				Items[idx].PostReplicatedAdd(ArraySerializer);
			}
			ArraySerializer.PostReplicatedAdd(AddedIndices, FinalSize);

			for (int32 idx : ChangedIndices)
			{
				Items[idx].PostReplicatedChange(ArraySerializer);
			}
			ArraySerializer.PostReplicatedChange(ChangedIndices, FinalSize);

			if (PreRemoveSize != Items.Num())
			{
				//UE_LOG(LogNetFastTArray, Error, TEXT("Item size changed after PostReplicatedAdd/PostReplicatedChange! PremoveSize: %d  Item.Num: %d"),
				//	PreRemoveSize, Items.Num());
			}

			if (Header.DeletedIndices.Num() > 0)
			{
				Header.DeletedIndices.Sort();
				for (int32 i = Header.DeletedIndices.Num() - 1; i >= 0; --i)
				{
					int32 DeleteIndex = Header.DeletedIndices[i];
					if (Items.IsValidIndex(DeleteIndex))
					{
						Items.RemoveAt(DeleteIndex, 1, EAllowShrinking::No);

						//UE_LOG(LogNetFastTArray, Log, TEXT("   Deleting: %d"), DeleteIndex);
					}
				}

				// Clear the map now that the indices are all shifted around. This kind of sucks, we could use slightly better data structures here I think.
				// This will force the ItemMap to be rebuilt for the current Items array
				ArraySerializer.ItemMap.Empty();
			}
		}

		/** Conditionally invoke PostReplicatedReceive method depending on if is defined or not */
		template<typename FastArrayType = SerializerType>
		inline typename TEnableIf<TModels_V<FFastArraySerializer::CPostReplicatedReceiveFuncable, FastArrayType, const FFastArraySerializer::FPostReplicatedReceiveParameters>, void>::Type CallPostReplicatedReceiveOrNot(int32 OldArraySize)
		{
			PRAGMA_DISABLE_DEPRECATION_WARNINGS
						FFastArraySerializer::FPostReplicatedReceiveParameters PostReceivedParameters = { OldArraySize, Parms.bOutHasMoreUnmapped };
			ArraySerializer.PostReplicatedReceive(PostReceivedParameters);
			PRAGMA_ENABLE_DEPRECATION_WARNINGS
		}

		template<typename FastArrayType = SerializerType>
		inline typename TEnableIf<!TModels_V<FFastArraySerializer::CPostReplicatedReceiveFuncable, FastArrayType, const FFastArraySerializer::FPostReplicatedReceiveParameters>, void>::Type CallPostReplicatedReceiveOrNot(int32) {}
	};

	template <typename Type, typename SerializerType>
	bool FastArrayDeltaSerialize_DeltaSerializeStructs_REDIRECT(TArray<Type>& Items, FNetDeltaSerializeInfo& Parms, SerializerType& ArraySerializer)
	{
		struct FFastArrayItemCallbackHelper
		{
			static void PreReplicatedRemove(void* FastArrayItem, const struct FFastArrayDeltaSerializeParams& Params)
			{
				static_cast<Type*>(FastArrayItem)->PreReplicatedRemove(static_cast<SerializerType&>(Params.ArraySerializer));
			}

			static void PostReplicatedAdd(void* FastArrayItem, const struct FFastArrayDeltaSerializeParams& Params)
			{
				static_cast<Type*>(FastArrayItem)->PostReplicatedAdd(static_cast<SerializerType&>(Params.ArraySerializer));
			}

			static void PostReplicatedChange(void* FastArrayItem, const struct FFastArrayDeltaSerializeParams& Params)
			{
				static_cast<Type*>(FastArrayItem)->PostReplicatedChange(static_cast<SerializerType&>(Params.ArraySerializer));
			}

			static void ReceivedItem(void* FastArrayItem, const FFastArrayDeltaSerializeParams& Params, const uint32 ReplicationID)
			{
				Type* Item = static_cast<Type*>(FastArrayItem);
				Item->ReplicationID = ReplicationID;
				Item->MostRecentArrayReplicationKey = Params.ReadArrayReplicationKey;
				Item->ReplicationKey++;
			}
		};

		//CONDITIONAL_SCOPE_CYCLE_COUNTER(STAT_NetSerializeFastArray_DeltaStruct, GUseDetailedScopeCounters);

		class UScriptStruct* InnerStruct = Type::StaticStruct();

		TFaerieFastArraySerializeHelper<Type, SerializerType> Helper{
			InnerStruct,
			Items,
			ArraySerializer,
			Parms
		};

		FFastArrayDeltaSerializeParams DeltaSerializeParams{
			Parms,
			ArraySerializer,
			FFastArrayItemCallbackHelper::PreReplicatedRemove,
			FFastArrayItemCallbackHelper::PostReplicatedAdd,
			FFastArrayItemCallbackHelper::PostReplicatedChange,
			FFastArrayItemCallbackHelper::ReceivedItem,
		};

		Helper.ConditionalRebuildItemMap();

		if (Parms.Reader)
		{
			//UE_LOG(LogNetFastTArray, Log, TEXT("FastArrayDeltaSerialize_DeltaSerializeStruct for %s. %s. Reading"), *InnerStruct->GetName(), *InnerStruct->GetOwnerStruct()->GetName());

			//-----------------------------
			// Loading
			//-----------------------------
			check(Parms.Reader);
			FBitReader& Reader = *Parms.Reader;

			FFaerieFastArraySerializerHeader Header;
			if (!Helper.ReadDeltaHeader(Header))
			{
				return false;
			}

			const int32 OldNumItems = Items.Num();
			TArray<int32, TInlineAllocator<8>> ChangedIndices;
			TArray<int32, TInlineAllocator<8>> AddedIndices;

			DeltaSerializeParams.ReadAddedElements = &AddedIndices;
			DeltaSerializeParams.ReadChangedElements = &ChangedIndices;
			DeltaSerializeParams.ReadNumChanged = Header.NumChanged;
			DeltaSerializeParams.ReadArrayReplicationKey = Header.ArrayReplicationKey;

			if (!Parms.NetSerializeCB->NetDeltaSerializeForFastArray(DeltaSerializeParams))
			{
				return false;
			}

			//---------------
			// Read Changed/New elements
			//---------------

			Helper.template PostReceiveCleanup<>(Header, ChangedIndices, AddedIndices, ArraySerializer.GuidReferencesMap_StructDelta);

			Helper.CallPostReplicatedReceiveOrNot(OldNumItems);
			return true;
		}

		return false;
	}

	// We are overriding this to redirect the PostReceiveCleanup call to one that doesn't use RemoveAtSwap
	template <typename Type, typename SerializerType>
	bool FastArrayDeltaSerialize(TArray<Type>& Items, FNetDeltaSerializeInfo& Parms, SerializerType& ArraySerializer)
	{
		if (EnumHasAllFlags(ArraySerializer.GetDeltaSerializationFlags(), EFastArraySerializerDeltaFlags::IsUsingDeltaSerialization))
		{
			if (FastArrayDeltaSerialize_DeltaSerializeStructs_REDIRECT(Items, Parms, ArraySerializer))
			{
				return true;
			}
		}

		return FFastArraySerializer::FastArrayDeltaSerialize<Type, SerializerType>(Items, Parms, ArraySerializer);
	}
}