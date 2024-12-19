#pragma once

#include <limits>
#include <concepts>
#include <type_traits>

namespace Blt {

template<int64_t TNum, int64_t TDemon = 1>
struct Ratio
{
  using Type                                  = Ratio<TNum, TDemon>;
  constexpr static inline int64_t Numerator   = TNum;
  constexpr static inline int64_t Denominator = TDemon;
};

namespace Detail {
  template<typename T>
  constexpr T abs(const T &value)
  {
    return value < 0 ? -value : value;
  }
  constexpr auto gcd(int64_t first, int64_t second) -> int64_t
  {
    if (first == 0 and second == 0) {
      return 1;// avoids division by 0 in ratio_less
    }

    first  = Detail::abs(first);
    second = Detail::abs(second);

    while (second != 0) {
      const int64_t temp = first;
      first              = second;
      second             = temp % second;
    }

    return first;
  }
  template<int64_t TMultiplier, int64_t TMultiplicand>
  constexpr inline bool is_multiply_overflow = []() {
    return Detail::abs(TMultiplier)
           <= ::std::numeric_limits<int64_t>::max() / (TMultiplicand == 0 ? 1 : Detail::abs(TMultiplicand));
  }();

  template<int64_t TMultiplier, int64_t TMultiplicand>
  struct multiple : std::integral_constant<int64_t, TMultiplier * TMultiplicand>
  {
    static_assert(is_multiply_overflow<TMultiplier, TMultiplicand>, "integer arithmetic overflow");
  };


  template<typename TMultiplier, typename TMultiplicand>
  struct ratio_multiply
  {

    constexpr static int64_t greatest_common_divisor_1 = gcd(TMultiplier::Numerator, TMultiplicand::Denominator);
    constexpr static int64_t greatest_common_divisor_2 = gcd(TMultiplicand::Numerator, TMultiplier::Denominator);

    using numerator = Detail::multiple<
      TMultiplier::Numerator / greatest_common_divisor_1,
      TMultiplicand::Numerator / greatest_common_divisor_2>;
    using denominator = Detail::multiple<
      TMultiplier::Denominator / greatest_common_divisor_2,
      TMultiplicand::Denominator / greatest_common_divisor_1>;
    using type = Ratio<numerator::value, denominator::value>;
  };
}// namespace Detail


template<typename TRatio>
using RatioInvert = Ratio<TRatio::Denominator, TRatio::Numerator>;

template<typename TMultiplier, typename TMultiplicand>
using RatioMultiply = Detail::ratio_multiply<TMultiplier, TMultiplicand>::type;

template<typename TDivisor, typename TDividend>
using RatioDivide = RatioMultiply<TDivisor, RatioInvert<TDividend>>;


///////////////////////////

template<typename T>
concept Numeric = std::integral<T> or std::floating_point<T>;

template<typename T>
concept StorageCapacity = requires(const T &obj) {
  typename T::RepresentType;
  typename T::PeriodType;
  requires(Numeric<typename T::RepresentType>);
  { obj.Count() } -> std::same_as<typename T::RepresentType>;
};

template<Numeric Rep, typename Period>
struct Capacity;

template<StorageCapacity TTo, StorageCapacity TFrom>
[[nodiscard]] constexpr auto capacityCast(const TFrom &capacity) noexcept -> TTo;


template<Numeric Rep, typename Period>
struct Capacity
{
public:
  using RepresentType = Rep;
  using PeriodType    = typename Period::Type;

  constexpr Capacity() = default;
  template<typename Rep2>
  constexpr explicit Capacity(const Rep2 &value) noexcept
    requires(
      std::convertible_to<const Rep2 &, RepresentType>
      and (std::floating_point<RepresentType> or not std::floating_point<Rep2>))
    : rep_(static_cast<RepresentType>(value))
  {}

  template<typename Rep2, typename Period2>
    requires(
      std::floating_point<RepresentType>
      or (RatioDivide<Period2, Period>::Denominator == 1 and not std::floating_point<Rep2>))
  constexpr explicit Capacity(const Capacity<Rep2, Period2> &capacity) noexcept
    : rep_(capacityCast<Capacity>(capacity).Count())
  {}

  [[nodiscard]] constexpr auto Count() const noexcept -> RepresentType { return rep_; }

  [[nodiscard]] constexpr auto operator+() const noexcept -> std::common_type_t<Capacity>
  {
    return std::common_type_t<Capacity>(*this);
  }

  [[nodiscard]] constexpr auto operator-() const noexcept -> std::common_type_t<Capacity>
  {
    return std::common_type_t<Capacity>(-rep_);
  }

  constexpr Capacity &operator++() noexcept
  {
    ++rep_;
    return *this;
  }

  constexpr Capacity operator++(int) noexcept { return Capacity(rep_++); }

  constexpr Capacity &operator--() noexcept
  {
    --rep_;
    return *this;
  }

  constexpr Capacity operator--(int) noexcept { return Capacity(rep_--); }

  constexpr Capacity &operator+=(const Capacity &other) noexcept
  {
    rep_ += other.rep_;
    return *this;
  }

  constexpr Capacity &operator-=(const Capacity &other) noexcept
  {
    rep_ -= other.rep_;
    return *this;
  }

  constexpr Capacity &operator*=(const RepresentType &other) noexcept
  {
    rep_ *= other;
    return *this;
  }

  friend constexpr auto operator/(const Capacity &first, const RepresentType &second) noexcept -> Capacity
  {
    return Capacity(first.rep_ / second);
  }

  friend constexpr auto operator/(const Capacity &first, const Capacity &second) noexcept -> RepresentType
  {
    return first.rep_ / second.rep_;
  }

  constexpr Capacity &operator/=(const RepresentType &other) noexcept
  {
    rep_ /= other;
    return *this;
  }

  constexpr Capacity &operator%=(const RepresentType &other) noexcept
  {
    rep_ %= other;
    return *this;
  }

  constexpr Capacity &operator%=(const Capacity &other) noexcept
  {
    rep_ %= other.Count();
    return *this;
  }
  friend constexpr auto operator==(const Capacity &first, const Capacity &second) noexcept -> bool
  {
    return first.rep_ == second.rep_;
  }
  friend constexpr auto operator<=>(const Capacity &first, const Capacity &second) noexcept -> decltype(auto)
  {
    return first.rep_ <=> second.rep_;
  }
  [[nodiscard]] static constexpr Capacity Zero() noexcept { return Capacity(RepresentType(0)); }

  [[nodiscard]] static constexpr Capacity Min() noexcept { return Capacity(std::numeric_limits<RepresentType>::min()); }

  [[nodiscard]] static constexpr Capacity Max() noexcept { return Capacity(std::numeric_limits<RepresentType>::max()); }

private:
  RepresentType rep_;
};

template<StorageCapacity TTo, StorageCapacity TFrom>
[[nodiscard]] constexpr auto capacityCast(const TFrom &capacity) noexcept -> TTo
{
  // convert Capacity to another Capacity; truncate
  using CalRatioType    = RatioDivide<typename TFrom::PeriodType, typename TTo::PeriodType>;
  using CommonRepType   = std::common_type_t<typename TTo::RepresentType, typename TFrom::RepresentType, uint64_t>;
  using ToRepresentType = typename TTo::RepresentType;

  constexpr bool numIsOne = CalRatioType::Numerator == 1;
  constexpr bool denIsOne = CalRatioType::Denominator == 1;

  // @TODO could be very wrong, need more testing
  if constexpr (denIsOne) {
    if constexpr (numIsOne) {
      return static_cast<TTo>(static_cast<ToRepresentType>(
        static_cast<CommonRepType>(capacity.Count()) / static_cast<CommonRepType>(CalRatioType::Denominator)));
    } else {
      return static_cast<TTo>(static_cast<ToRepresentType>(
        static_cast<CommonRepType>(capacity.Count()) * static_cast<CommonRepType>(CalRatioType::Numerator)
        / static_cast<CommonRepType>(CalRatioType::Denominator)));
    }
  } else {
    if constexpr (numIsOne) {
      return static_cast<TTo>(static_cast<ToRepresentType>(capacity.Count()));
    } else {

      return static_cast<TTo>(static_cast<ToRepresentType>(
        static_cast<CommonRepType>(capacity.Count()) / static_cast<CommonRepType>(CalRatioType::Numerator)));
    }
  }
}

template<typename TRep1, typename TPeriod1, typename TRep2, typename TPeriod2>
[[nodiscard]] constexpr auto
  operator+(const Capacity<TRep1, TPeriod1> &first, const Capacity<TRep2, TPeriod2> &second) noexcept
  -> std::common_type_t<Capacity<TRep1, TPeriod1>, Capacity<TRep2, TPeriod2>>
{
  using ResultType = std::common_type_t<Capacity<TRep1, TPeriod1>, Capacity<TRep2, TPeriod2>>;
  return ResultType(ResultType(first).Count() + ResultType(second).Count());
}
template<typename TRep1, typename TPeriod1, typename TRep2, typename TPeriod2>
[[nodiscard]] constexpr auto
  operator-(const Capacity<TRep1, TPeriod1> &first, const Capacity<TRep2, TPeriod2> &second) noexcept
  -> std::common_type_t<Capacity<TRep1, TPeriod1>, Capacity<TRep2, TPeriod2>>
{
  using ResultType = std::common_type_t<Capacity<TRep1, TPeriod1>, Capacity<TRep2, TPeriod2>>;
  return ResultType(ResultType(first).Count() - ResultType(second).Count());
}

template<typename TRep, typename TPeriod>
[[nodiscard]] constexpr auto Clamp(
  const Capacity<TRep, TPeriod> &capacity,
  const Capacity<TRep, TPeriod> &min,
  const Capacity<TRep, TPeriod> &max) noexcept -> Capacity<TRep, TPeriod>
{
  if (capacity.Count() < min.Count())
    return min;
  else if (capacity.Count() > max.Count())
    return max;
  else
    return capacity;
}

using CapacityBytes = Blt::Capacity<uint64_t, Ratio<1, 1>>;
using Kibibytes     = Blt::Capacity<uint64_t, Ratio<1024, 1>>;
using Mebibytes     = Blt::Capacity<uint64_t, Ratio<1024 * 1024, 1>>;
using Gibibytes     = Blt::Capacity<uint64_t, Ratio<1024 * 1024 * 1024, 1>>;

// These unit ratio is too big for Ratio's int64_t
// using Teribytes     = Blt::Capacity<uint64_t, Ratio<1024 * 1024 * 1024 * 1024, 1>>;
// using Pebibytes     = Blt::Capacity<uint64_t, Ratio<1024 * 1024 * 1024 * 1024 * 1024, 1>>;

}// namespace Blt
