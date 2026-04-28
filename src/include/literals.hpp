#pragma once

#include <stddef.h>

#include <tuple>

namespace literals {
/*
 * M L T means the pwr_t of each unit
 * M := mass
 * L := length
 * T := time
 * Example:
 * <0, 0, 1> = s
 * <0, 1, -1> = m/s
 */
template <int M, int L, int T>
struct Quantity {
    float v;
    static constexpr std::tuple<int, int, int> dim() { return std::make_tuple(M, L, T); }
    explicit constexpr Quantity(float v) : v(v) {}
};

template <typename T>
struct is_quantity : std::false_type {};
template <int M, int L, int T>
struct is_quantity<Quantity<M, L, T>> : std::true_type {};
template <typename T>
inline constexpr bool is_quantity_v = is_quantity<std::decay_t<T>>::value;


using dim_less = Quantity<0, 0, 0>;

using mass_t = Quantity<1, 0, 0>; // kilogram
using leng_t = Quantity<0, 1, 0>; // metre
using dura_t = Quantity<0, 0, 1>; // second

using vel_t = Quantity<0, 1, -1>; // m/s
using acc_t = Quantity<0, 1, -2>; // m/s^2
using jrk_t = Quantity<0, 1, -3>; // m/s^3

using frc_t = Quantity<1, 1, -2>; // kg*m/s^2 (Newton)
using eng_t = Quantity<1, 2, -2>; // kg*m^2/s^2 (Joule)
using pwr_t = Quantity<1, 2, -3>; // kg*m^2/s^3 (Watt)

using avel_t = Quantity<0, 0, -1>; // rad/s

using frq_t = Quantity<0, 0, -1>; // 1/s (Hz)

/// * /
template <int M1, int L1, int T1, int M2, int L2, int T2>
constexpr auto operator*(Quantity<M1, L1, T1> lhs, Quantity<M2, L2, T2> rhs) -> Quantity<M1 + M2, L1 + L2, T1 + T2> {
    return Quantity<M1 + M2, L1 + L2, T1 + T2>(lhs.v * rhs.v);
}
template <int M1, int L1, int T1, int M2, int L2, int T2>
constexpr auto operator/(Quantity<M1, L1, T1> lhs, Quantity<M2, L2, T2> rhs) -> Quantity<M1 - M2, L1 - L2, T1 - T2> {
    return Quantity<M1 - M2, L1 - L2, T1 - T2>(lhs.v / rhs.v);
}

/// + -
template <int M, int L, int T>
constexpr auto operator+(Quantity<M, L, T> lhs, Quantity<M, L, T> rhs) -> Quantity<M, L, T> {
    return Quantity<M, L, T>(lhs.v + rhs.v);
}
template <int M, int L, int T>
constexpr auto operator-(Quantity<M, L, T> lhs, Quantity<M, L, T> rhs) -> Quantity<M, L, T> {
    return Quantity<M, L, T>(lhs.v - rhs.v);
}
template <int M, int L, int T>
constexpr auto operator-(Quantity<M, L, T> lhs) -> Quantity<M, L, T> {
    return Quantity<M, L, T>(-lhs.v);
}

/// * / scalar
template <int M, int L, int T>
constexpr auto operator*(Quantity<M, L, T> lhs, float rhs) -> Quantity<M, L, T> {
    return Quantity<M, L, T>(lhs.v * rhs);
}
template <int M, int L, int T>
constexpr auto operator/(Quantity<M, L, T> lhs, float rhs) -> Quantity<M, L, T> {
    return Quantity<M, L, T>(lhs.v / rhs);
}

template <int M, int L, int T>
constexpr auto operator*(float lhs, Quantity<M, L, T> rhs) -> Quantity<M, L, T> {
    return Quantity<M, L, T>(lhs * rhs.v);
}
template <int M, int L, int T>
constexpr auto operator/(float lhs, Quantity<M, L, T> rhs) -> Quantity<-M, -L, -T> {
    return Quantity<-M, -L, -T>(lhs / rhs.v);
}

// compare
template <int M, int L, int T>
constexpr auto operator==(Quantity<M, L, T> lhs, Quantity<M, L, T> rhs) -> bool {
    return lhs.v == rhs.v;
}
template <int M, int L, int T>
constexpr auto operator!=(Quantity<M, L, T> lhs, Quantity<M, L, T> rhs) -> bool {
    return lhs.v != rhs.v;
}
template <int M, int L, int T>
constexpr auto operator<(Quantity<M, L, T> lhs, Quantity<M, L, T> rhs) -> bool {
    return lhs.v < rhs.v;
}
template <int M, int L, int T>
constexpr auto operator>(Quantity<M, L, T> lhs, Quantity<M, L, T> rhs) -> bool {
    return lhs.v > rhs.v;
}
template <int M, int L, int T>
constexpr auto operator<=(Quantity<M, L, T> lhs, Quantity<M, L, T> rhs) -> bool {
    return lhs.v <= rhs.v;
}
template <int M, int L, int T>
constexpr auto operator>=(Quantity<M, L, T> lhs, Quantity<M, L, T> rhs) -> bool {
    return lhs.v >= rhs.v;
}

/// time
constexpr auto operator"" s(long double value) -> dura_t { return dura_t(static_cast<float>(value)); }
constexpr auto operator"" ms(long double value) -> dura_t { return dura_t(static_cast<float>(value) / 1000.); }
constexpr auto operator"" us(long double value) -> dura_t { return dura_t(static_cast<float>(value)) / 1000000.; }
constexpr auto operator"" s(unsigned long long value) -> dura_t { return dura_t(static_cast<float>(value)); }
constexpr auto operator"" ms(unsigned long long value) -> dura_t { return dura_t(static_cast<float>(value) / 1000.); }
constexpr auto operator"" us(unsigned long long value) -> dura_t { return dura_t(static_cast<float>(value)) / 1000000.; }

/// mass
constexpr auto operator"" kg(long double value) -> mass_t { return mass_t(static_cast<float>(value)); }
constexpr auto operator"" g(long double value) -> mass_t { return mass_t(static_cast<float>(value) / 1000.); }
constexpr auto operator"" kg(unsigned long long value) -> mass_t { return mass_t(static_cast<float>(value)); }
constexpr auto operator"" g(unsigned long long value) -> mass_t { return mass_t(static_cast<float>(value) / 1000.); }

/// distance
constexpr auto operator"" km(long double value) -> leng_t { return leng_t(static_cast<float>(value)) * 1000; }
constexpr auto operator"" m(long double value) -> leng_t { return leng_t(static_cast<float>(value)); }
constexpr auto operator"" mm(long double value) -> leng_t { return leng_t(static_cast<float>(value)) / 1000; }
constexpr auto operator"" km(unsigned long long value) -> leng_t { return leng_t(static_cast<float>(value)) * 1000; }
constexpr auto operator"" m(unsigned long long value) -> leng_t { return leng_t(static_cast<float>(value)); }
constexpr auto operator"" mm(unsigned long long value) -> leng_t { return leng_t(static_cast<float>(value)) / 1000; }

/// speed
constexpr auto operator"" m_s(long double value) -> vel_t { return vel_t(static_cast<float>(value)); }
constexpr auto operator"" m_s(unsigned long long value) -> vel_t { return vel_t(static_cast<float>(value)); }

/// Newton
constexpr auto operator"" kN(long double value) -> frc_t { return frc_t(static_cast<float>(value)) * 1000; }
constexpr auto operator"" N(long double value) -> frc_t { return frc_t(static_cast<float>(value)); }
constexpr auto operator"" mN(long double value) -> frc_t { return frc_t(static_cast<float>(value)) / 1000; }

constexpr auto operator"" kN(unsigned long long value) -> frc_t { return frc_t(static_cast<float>(value)) * 1000; }
constexpr auto operator"" N(unsigned long long value) -> frc_t { return frc_t(static_cast<float>(value)); }
constexpr auto operator"" mN(unsigned long long value) -> frc_t { return frc_t(static_cast<float>(value)) / 1000; }

/// eng_t
constexpr auto operator"" kJ(long double value) -> eng_t { return eng_t(static_cast<float>(value)) * 1000; }
constexpr auto operator"" J(long double value) -> eng_t { return eng_t(static_cast<float>(value)); }
constexpr auto operator"" mJ(long double value) -> eng_t { return eng_t(static_cast<float>(value)) / 1000; }
constexpr auto operator"" kJ(unsigned long long value) -> eng_t { return eng_t(static_cast<float>(value)) * 1000; }
constexpr auto operator"" J(unsigned long long value) -> eng_t { return eng_t(static_cast<float>(value)); }
constexpr auto operator"" mJ(unsigned long long value) -> eng_t { return eng_t(static_cast<float>(value)) / 1000; }

/// pwr_t
constexpr auto operator"" kW(long double value) -> pwr_t { return pwr_t(static_cast<float>(value)) * 1000; }
constexpr auto operator"" W(long double value) -> pwr_t { return pwr_t(static_cast<float>(value)); }
constexpr auto operator"" mW(long double value) -> pwr_t { return pwr_t(static_cast<float>(value)) / 1000; }
constexpr auto operator"" kW(unsigned long long value) -> pwr_t { return pwr_t(static_cast<float>(value)) * 1000; }
constexpr auto operator"" W(unsigned long long value) -> pwr_t { return pwr_t(static_cast<float>(value)); }
constexpr auto operator"" mW(unsigned long long value) -> pwr_t { return pwr_t(static_cast<float>(value)) / 1000; }

/// frq_t
constexpr auto operator"" Hz(long double value) -> frq_t { return frq_t(static_cast<float>(value)); }
constexpr auto operator"" Hz(unsigned long long value) -> frq_t { return frq_t(static_cast<float>(value)); }

/// rot_t
constexpr auto operator"" rad_s(long double value) -> avel_t { return avel_t(static_cast<float>(value)); }
constexpr auto operator"" rad_s(unsigned long long value) -> avel_t { return avel_t(static_cast<float>(value)); }

} // namespace literals
