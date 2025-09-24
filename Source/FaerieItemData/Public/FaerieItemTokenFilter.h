// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemDataConcepts.h"
#include "GameplayTagContainer.h"
#include "LoopUtils.h"

struct FGameplayTag;
struct FGameplayTagContainer;
struct FGameplayTagQuery;
class UFaerieItem;
class UFaerieItemDataLibrary;
class UFaerieItemToken;

namespace Faerie::Token
{
	class ITokenFilter;

	namespace Private
	{
		class FAERIEITEMDATA_API FIteratorAccess
		{
		protected:
			static void AddWriteLock(const UFaerieItem* Item);
			static void RemoveWriteLock(const UFaerieItem* Item);

			static UFaerieItemToken* ResolveToken(const UFaerieItem* Item, int32 Index);
			static const UFaerieItemToken* ConstResolveToken(const UFaerieItem* Item, int32 Index);
		};
	}

	template <CItemToken FilterClass, bool Const>
	class TTokenIterator_Masked : Private::FIteratorAccess
	{
		friend ITokenFilter;

		using ElementType = std::conditional_t<Const, const FilterClass, FilterClass>;

	public:
		TTokenIterator_Masked(const UFaerieItem* Item, const TBitArray<>& TokenBits)
		  : Item(Item), TokenBits(TokenBits), Iterator(this->TokenBits)
		{
			AddWriteLock(Item);
		}

		~TTokenIterator_Masked()
		{
			RemoveWriteLock(Item);
		}

		FORCEINLINE ElementType* operator*() const
		{
			if constexpr (Const)
			{
				return CastChecked<FilterClass>(ConstResolveToken(Item, Iterator.GetIndex()));
			}
			else
			{
				return CastChecked<FilterClass>(ResolveToken(Item, Iterator.GetIndex()));
			}
		}

		TTokenIterator_Masked& operator++()
		{
			++Iterator;
			return *this;
		}

		FORCEINLINE explicit operator bool() const
		{
			return static_cast<bool>(Iterator);
		}

		[[nodiscard]] FORCEINLINE bool operator!=(EIteratorType) const
		{
			// As long we are valid, then we have not ended.
			return static_cast<bool>(*this);
		}

	protected:
		const UFaerieItem* Item;
		const TBitArray<>& TokenBits;
		TConstSetBitIterator<> Iterator;
	};

	struct ITokenFilterType
	{
		virtual ~ITokenFilterType() = default;
		virtual bool Passes(const UFaerieItemToken* Token) = 0;
	};

	template <typename T>
	concept CTokenFilterType = TIsDerivedFrom<typename TRemoveReference<T>::Type, ITokenFilterType>::Value;

	enum class EFilterFlags : uint32
	{
		None = 0,

		// @todo not yet supported
		ImmutableOnly = 1 << 0,

		// This filter is restricted to emitting mutable tokens
		MutableOnly = 1 << 1,

		// This filter can be invoked statically
		Static = 1 << 2
	};
	ENUM_CLASS_FLAGS(EFilterFlags)

	template <typename T>
	struct TFilterProperties
	{
		static constexpr EFilterFlags GrantFlags = EFilterFlags::None;
		static constexpr EFilterFlags RemoveFlags = EFilterFlags::None;
	};

	template <typename T, EFilterFlags Flags>
	consteval EFilterFlags CombineFilterFlags()
	{
		return (Flags & ~TFilterProperties<T>::RemoveFlags) | TFilterProperties<T>::GrantFlags;
	}

	struct FAERIEITEMDATA_API FMutableFilter final : ITokenFilterType
	{
		virtual bool Passes(const UFaerieItemToken* Token) override { return StaticPasses(Token); }
		static bool StaticPasses(const UFaerieItemToken* Token);
	};

	template <>
	struct TFilterProperties<FMutableFilter>
	{
		static constexpr EFilterFlags TypeFlags = EFilterFlags::Static;
		static constexpr EFilterFlags GrantFlags = EFilterFlags::MutableOnly;
		static constexpr EFilterFlags RemoveFlags = EFilterFlags::ImmutableOnly;
	};

	struct FAERIEITEMDATA_API FImmutableFilter final : ITokenFilterType
	{
		virtual bool Passes(const UFaerieItemToken* Token) override { return StaticPasses(Token); }
		static bool StaticPasses(const UFaerieItemToken* Token);
	};

	template <>
	struct TFilterProperties<FImmutableFilter>
	{
		static constexpr EFilterFlags FilterFlags = EFilterFlags::Static;
		static constexpr EFilterFlags GrantFlags = EFilterFlags::ImmutableOnly;
		static constexpr EFilterFlags RemoveFlags = EFilterFlags::MutableOnly;
	};

	struct FAERIEITEMDATA_API FTagFilter final : ITokenFilterType
	{
		FTagFilter(const FGameplayTag& Tag, const bool Exact = false)
		  : Tag(Tag), Exact(Exact) {}

		virtual bool Passes(const UFaerieItemToken* Token) override;

	protected:
		const FGameplayTag Tag;
		const bool Exact;
	};

	struct FAERIEITEMDATA_API FTagsFilter final : ITokenFilterType
	{
		FTagsFilter(const FGameplayTagContainer& Tags, const bool All = false, const bool Exact = false)
		  : Tags(Tags), All(All), Exact(Exact) {}

		virtual bool Passes(const UFaerieItemToken* Token) override;

	protected:
		const FGameplayTagContainer Tags;
		const bool All;
		const bool Exact;
	};

	struct FAERIEITEMDATA_API FTagQueryFilter final : ITokenFilterType
	{
		FTagQueryFilter(const FGameplayTagQuery& Query)
		  : Query(Query) {}

		virtual bool Passes(const UFaerieItemToken* Token) override;

	protected:
		const FGameplayTagQuery Query;
	};

	class ITokenFilter : Private::FIteratorAccess
	{
		// Let this library use BlueprintOnlyAccess;
		friend UFaerieItemDataLibrary;

	public:
		FAERIEITEMDATA_API ITokenFilter(const UFaerieItem* Item);

		// Construct with an existing bit array.
		ITokenFilter(const UFaerieItem* Item, const TBitArray<>& TokenBits)
		  : Item(Item), TokenBits(TokenBits) {}

	protected:
		ITokenFilter& Invert_Impl()
		{
			TokenBits.BitwiseNOT();
			return *this;
		}

		// Removes tokens from filter not of the given class.
		FAERIEITEMDATA_API ITokenFilter& ByClass_Impl(const TSubclassOf<UFaerieItemToken>& Class);

		// Run a filter type by its virtual Passes implementation.
		ITokenFilter& ByVirtual_Impl(ITokenFilterType& Type);

		// Run a filter type by directly calling the Passes implementation on a typed instance.
		template <CTokenFilterType T>
		ITokenFilter& ByTemplate_Impl(T& Type)
		{
			for (TConstSetBitIterator<> It(TokenBits); It; ++It)
			{
				const UFaerieItemToken* Token = ConstResolveToken(Item, It.GetIndex());
				//if (!ensureAlways(IsValid(Token))) continue;
				if (!Type.T::Passes(Token))
				{
					TokenBits.AccessCorrespondingBit(It) = false;
				}
			}
			return *this;
		}

		// Run a filter type by invoking the static version of its Passes Implementation
		template <CTokenFilterType T>
		ITokenFilter& ByStatic_Impl()
		{
			for (TConstSetBitIterator<> It(TokenBits); It; ++It)
			{
				const UFaerieItemToken* Token = ConstResolveToken(Item, It.GetIndex());
				//if (!ensureAlways(IsValid(Token))) continue;
				if (!T::StaticPasses(Token))
				{
					TokenBits.AccessCorrespondingBit(It) = false;
				}
			}
			return *this;
		}

	public:
		bool CompareTokens(const ITokenFilter& OtherFilter) const;

		bool IsEmpty() const { return TokenBits.IsEmpty(); }
		int32 Num() const { return TokenBits.CountSetBits(); }

		// Create an array with the filters set of tokens.
		FAERIEITEMDATA_API TArray<const UFaerieItemToken*> Emit() const;

		// Create an array with the filters set of tokens.
		TArray<const UFaerieItemToken*> operator*() const { return Emit(); }

		FORCEINLINE auto begin() const { return TTokenIterator_Masked<UFaerieItemToken, true>(Item, TokenBits); }
		FORCEINLINE EIteratorType end () const { return End; }

	private:
		TArray<TObjectPtr<UFaerieItemToken>> BlueprintOnlyAccess() const;

	protected:
		const UFaerieItem* Item;
		TBitArray<> TokenBits;
	};

	template <CItemToken FilterClass, EFilterFlags Flags>
	class TTokenFilter : public ITokenFilter
	{
	public:
		static constexpr bool Const = !EnumHasAnyFlags(Flags, EFilterFlags::MutableOnly);
		using ElementType = std::conditional_t<Const, const FilterClass, FilterClass>;

		template <typename T> using TReturnFilterType =
			std::conditional_t<
				CombineFilterFlags<T, Flags>() == Flags,
				TTokenFilter&,
				TTokenFilter<FilterClass, CombineFilterFlags<T, Flags>()>>;

		using ITokenFilter::ITokenFilter;

		TTokenFilter& Invert()
		{
			Invert_Impl();
			return *this;
		}

		// Removes tokens from filter not of the given class.
		TTokenFilter& ByClass(const TSubclassOf<UFaerieItemToken>& Class)
		{
			ByClass_Impl(Class);
			return *this;
		}

		// Removes tokens from filter of the given class.
		template<CItemToken T>
		[[nodiscard]] auto ByClass()
		{
			// Only bother running a filter if it is more specific than the class we are already filtered by.
			if constexpr (TIsDerivedFrom<T, FilterClass>::Value)
			{
				ByClass_Impl(T::StaticClass());
				return TTokenFilter<T, Flags>(Item, TokenBits);
			}
			else
			{
				return *this;
			}
		}

		template<CItemToken T>
		[[nodiscard]] TTokenFilter<T, Flags> ByClass(const TSubclassOf<T>& Class)
		{
			ByClass_Impl(Class);
			return TTokenFilter<T, Flags>(Item, TokenBits);
		}

		ElementType* At(const int32 Index) const
		{
			if constexpr (Const)
			{
				return CastChecked<FilterClass>(Private::FIteratorAccess::ConstResolveToken(Item, Index));
			}
			else
			{
				return CastChecked<FilterClass>(Private::FIteratorAccess::ResolveToken(Item, Index));
			}
		}

		// Removes tokens from filter that fail the Filter Type
		template <CTokenFilterType T>
		[[nodiscard]] TReturnFilterType<T> By()
		{
			if constexpr (EnumHasAnyFlags(TFilterProperties<T>::TypeFlags, EFilterFlags::Static))
			{
				ByStatic_Impl<T>();
			}
			else
			{
				T TypeStruct;
				ByTemplate_Impl(TypeStruct);
			}

			if constexpr (CombineFilterFlags<T, Flags>() == Flags)
			{
				return *this;
			}
			else
			{
				return TTokenFilter<FilterClass, CombineFilterFlags<T, Flags>()>(Item, TokenBits);
			}
		}


		// Removes tokens from filter that fail the Filter Type
		template <CTokenFilterType T, typename... TArgs>
		[[nodiscard]] TReturnFilterType<T> By(TArgs&&... Args)
		{
			T TypeStruct(Args...);
			ByTemplate_Impl(TypeStruct);

			if constexpr (CombineFilterFlags<T, Flags>() == Flags)
			{
				return *this;
			}
			else
			{
				return TTokenFilter<FilterClass, CombineFilterFlags<T, Flags>()>(Item, TokenBits);
			}
		}

		FORCEINLINE auto begin() const
		{
			return TTokenIterator_Masked<FilterClass, Const>(Item, TokenBits);
		}
	};

	using FTokenFilter = TTokenFilter<UFaerieItemToken, EFilterFlags::None>;
}