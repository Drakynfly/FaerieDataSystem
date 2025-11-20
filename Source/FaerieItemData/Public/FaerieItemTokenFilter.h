// Copyright Guy (Drakynfly) Lundvall. All Rights Reserved.

#pragma once

#include "FaerieItemDataConcepts.h"
#include "LoopUtils.h"
#include "Containers/BitArray.h"
#include "Templates/Casts.h"

class UFaerieItem;
class UFaerieItemDataLibrary;
class UFaerieItemToken;

namespace Faerie::Token
{
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

	class IFilter;

	template <CItemToken FilterClass, bool Const>
	class TIterator_Masked : Private::FIteratorAccess
	{
		friend IFilter;

		using ElementType = std::conditional_t<Const, const FilterClass, FilterClass>;

	public:
		TIterator_Masked(const UFaerieItem* Item, const TBitArray<>& TokenBits)
		  : Item(Item), TokenBits(TokenBits), Iterator(this->TokenBits)
		{
			AddWriteLock(Item);
		}

		TIterator_Masked(const TIterator_Masked& Other)
		  : Item(Other.Item), TokenBits(Other.TokenBits), Iterator(this->TokenBits)
		{
			AddWriteLock(Item);
		}

		~TIterator_Masked()
		{
			RemoveWriteLock(Item);
		}

		[[nodiscard]] ElementType* operator*() const
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

		UE_REWRITE TIterator_Masked& operator++()
		{
			++Iterator;
			return *this;
		}

		UE_REWRITE explicit operator bool() const
		{
			return static_cast<bool>(Iterator);
		}

		[[nodiscard]] UE_REWRITE bool operator!=(EIteratorType) const
		{
			// As long as we are valid, then we have not ended.
			return static_cast<bool>(*this);
		}

	protected:
		const UFaerieItem* Item;
		const TBitArray<> TokenBits;
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
	struct TFilterTraits
	{
		static constexpr EFilterFlags GrantFlags = EFilterFlags::None;
		static constexpr EFilterFlags RemoveFlags = EFilterFlags::None;
	};

	template <typename T, EFilterFlags Flags>
	consteval EFilterFlags CombineFilterFlags()
	{
		return (Flags & ~TFilterTraits<T>::RemoveFlags) | TFilterTraits<T>::GrantFlags;
	}

	class IFilter : Private::FIteratorAccess
	{
		// Let this library use BlueprintOnlyAccess;
		friend UFaerieItemDataLibrary;

	public:
		FAERIEITEMDATA_API IFilter(const UFaerieItem* Item);

		// Construct with an existing bit array.
		IFilter(const UFaerieItem* Item, const TBitArray<>& TokenBits)
		  : Item(Item), TokenBits(TokenBits) {}

	protected:
		IFilter& Invert_Impl()
		{
			TokenBits.BitwiseNOT();
			return *this;
		}

		// Removes tokens from filter not of the given class.
		FAERIEITEMDATA_API IFilter& ByClass_Impl(const TSubclassOf<UFaerieItemToken>& Class);

		// Run a filter type by its virtual Passes implementation.
		IFilter& ByVirtual_Impl(ITokenFilterType& Type);

		// Run a filter type by directly calling the Passes implementation on a typed instance.
		template <CTokenFilterType T>
		IFilter& ByTemplate_Impl(T& Type)
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
		IFilter& ByStatic_Impl()
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
		bool CompareTokens(const IFilter& OtherFilter) const;

		UE_REWRITE bool IsEmpty() const { return TokenBits.IsEmpty(); }
		UE_REWRITE int32 Num() const { return TokenBits.CountSetBits(); }

		// Create an array with the filters set of tokens.
		FAERIEITEMDATA_API TArray<const UFaerieItemToken*> Emit() const;

		// Create an array with the filters set of tokens.
		[[nodiscard]] UE_REWRITE TArray<const UFaerieItemToken*> operator*() const { return Emit(); }

		[[nodiscard]] UE_REWRITE auto begin() const { return TIterator_Masked<UFaerieItemToken, true>(Item, TokenBits); }
		[[nodiscard]] UE_REWRITE EIteratorType end () const { return End; }

	private:
		TArray<UFaerieItemToken*> BlueprintOnlyAccess() const;

	protected:
		const UFaerieItem* Item;
		TBitArray<> TokenBits;
	};

	template <CItemToken FilterClass, EFilterFlags Flags>
	class TFilter : public IFilter
	{
	public:
		static constexpr bool Const = !EnumHasAnyFlags(Flags, EFilterFlags::MutableOnly);
		using ElementType = std::conditional_t<Const, const FilterClass, FilterClass>;

		template <typename T> using TReturnType_CombineFlags =
			std::conditional_t<
				CombineFilterFlags<T, Flags>() == Flags,
				TFilter&,
				TFilter<FilterClass, CombineFilterFlags<T, Flags>()>>;

		template <typename T> using TReturnType_FilterClass =
			std::conditional_t<
				std::is_same_v<FilterClass, T>,
				TFilter&,
				TFilter<T, Flags>>;

		using IFilter::IFilter;

		UE_REWRITE TFilter& Invert()
		{
			Invert_Impl();
			return *this;
		}

		// Removes tokens from filter of the given class.
		// Only allows running the filter if it is more specific than the class we are already filtered by.
		template<
			CItemToken T
			UE_REQUIRES(TIsDerivedFrom<T, FilterClass>::Value)
		>
		[[nodiscard]] auto ByClass()
		{
			ByClass_Impl(T::StaticClass());
			return TFilter<T, Flags>(Item, TokenBits);
		}

		// Removes tokens from filter of the given class.
		// Only allows running the filter if it is more specific than the class we are already filtered by.
		template<
			CItemToken T
			UE_REQUIRES(TIsDerivedFrom<T, FilterClass>::Value)
		>
		[[nodiscard]] TReturnType_FilterClass<T> ByClass(const TSubclassOf<T>& Class)
		{
			ByClass_Impl(Class);

			if constexpr (std::is_same_v<FilterClass, T>)
			{
				return *this;
			}
			else
			{
				return TFilter<T, Flags>(Item, TokenBits);
			}
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
		[[nodiscard]] TReturnType_CombineFlags<T> By()
		{
			if constexpr (EnumHasAnyFlags(TFilterTraits<T>::TypeFlags, EFilterFlags::Static))
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
				return TFilter<FilterClass, CombineFilterFlags<T, Flags>()>(Item, TokenBits);
			}
		}


		// Removes tokens from filter that fail the Filter Type
		template <CTokenFilterType T, typename... TArgs>
		[[nodiscard]] TReturnType_CombineFlags<T> By(TArgs&&... Args)
		{
			T TypeStruct(Args...);
			ByTemplate_Impl(TypeStruct);

			if constexpr (CombineFilterFlags<T, Flags>() == Flags)
			{
				return *this;
			}
			else
			{
				return TFilter<FilterClass, CombineFilterFlags<T, Flags>()>(Item, TokenBits);
			}
		}

		UE_REWRITE auto begin() const
		{
			return TIterator_Masked<FilterClass, Const>(Item, TokenBits);
		}
	};

	// Forward declare the default parameters of the template
	template <CItemToken FilterClass = UFaerieItemToken, EFilterFlags Flags = EFilterFlags::None>
	class TFilter;

	// Create a filter to select tokens from an Item
	FAERIEITEMDATA_API TFilter<> Filter(const UFaerieItem* Item);
}