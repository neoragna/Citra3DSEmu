// Copyright 2015 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <cmath>
#include <cstdint>
#include <cstring>

#include "common/common_types.h"

namespace Pica {

/**
 * Class for storing and operating on Fixed point types using:
 *
 * - 12 integer bits
 * - 4 fraction bits
 * - Negative values in two's complement
 *
 * @todo Verify on HW where and how this is used.
 * @todo Create a template for more fixed point types and use this elsewhere too.
 */
class Fix12P4 final {
public:
    constexpr Fix12P4() = default;
    constexpr Fix12P4(const Fix12P4&) = default;
    constexpr Fix12P4(Fix12P4&&) = default;
    constexpr explicit Fix12P4(int16_t raw) noexcept : value(raw) {}

    Fix12P4& operator=(const Fix12P4&) = default;
    Fix12P4& operator=(Fix12P4&&) = default;

    static constexpr Fix12P4 FromInt(int16_t int_val, uint16_t frac_val = 0) noexcept {
        return Fix12P4{static_cast<int16_t>(((int_val * 16) & IntMask()) | (frac_val & FracMask()))};
    }

    static Fix12P4 FromFloat(float float_val) noexcept {
        return Fix12P4{static_cast<int16_t>(::round(float_val * 16.0f))};
    }

    static constexpr Fix12P4 Zero() noexcept {
        return Fix12P4{static_cast<int16_t>(0)};
    }

    static constexpr int16_t FracMask() noexcept {
        return static_cast<int16_t>(0xF);
    }

    static constexpr int16_t IntMask() noexcept {
        return static_cast<int16_t>(~0xF);
    }

    constexpr int16_t Int() const noexcept {
        return static_cast<int16_t>((value & IntMask()) / 16);
    }

    constexpr uint16_t Frac() const noexcept {
        return static_cast<uint16_t>(value & FracMask());
    }

    constexpr Fix12P4 Ceil() const noexcept {
        return Fix12P4{static_cast<int16_t>(value + FracMask())}.Floor();
    }

    constexpr Fix12P4 Floor() const noexcept {
        return Fix12P4{static_cast<int16_t>(value & IntMask())};
    }

    constexpr explicit operator int16_t() const noexcept {
        return value;
    }

    constexpr Fix12P4 operator+() const noexcept {
        return Fix12P4{static_cast<int16_t>(+value)};
    }

    constexpr Fix12P4 operator-() const noexcept {
        return Fix12P4{static_cast<int16_t>(-value)};
    }

    constexpr Fix12P4 operator+(const Fix12P4& other) const noexcept {
        return Fix12P4{static_cast<int16_t>(value + other.value)};
    }

    constexpr Fix12P4 operator-(const Fix12P4& other) const noexcept {
        return Fix12P4{static_cast<int16_t>(value - other.value)};
    }

    constexpr Fix12P4 operator*(const Fix12P4& other) const noexcept {
        return Fix12P4{Multiply(value, other.value)};
    }

    constexpr Fix12P4 operator/(const Fix12P4& other) const noexcept {
        return Fix12P4{Divide(value, other.value)};
    }

    Fix12P4& operator+=(const Fix12P4& other) noexcept {
        value += other.value;
        return *this;
    }

    Fix12P4& operator-=(const Fix12P4& other) noexcept {
        value -= other.value;
        return *this;
    }

    Fix12P4& operator*=(const Fix12P4& other) noexcept {
        value = Multiply(value, other.value);
        return *this;
    }

    Fix12P4& operator/=(const Fix12P4& other) noexcept {
        value = Divide(value, other.value);
        return *this;
    }

    friend constexpr bool operator<(const Fix12P4& left, const Fix12P4& right) noexcept {
        return left.value < right.value;
    }

    friend constexpr bool operator>(const Fix12P4& left, const Fix12P4& right) noexcept {
        return operator<(right, left);
    }

    friend constexpr bool operator>=(const Fix12P4& left, const Fix12P4& right) noexcept {
        return !operator<(left, right);
    }

    friend constexpr bool operator<=(const Fix12P4& left, const Fix12P4& right) noexcept {
        return !operator<(right, left);
    }

    friend constexpr bool operator==(const Fix12P4& left, const Fix12P4& right) noexcept {
        return left.value == right.value;
    }

    friend constexpr bool operator!=(const Fix12P4& left, const Fix12P4& right) noexcept {
        return !operator==(left, right);
    }

private:
    static constexpr int16_t Multiply(int16_t left, int16_t right) noexcept {
        return static_cast<int16_t>((left * right) / 16);
    }

    static constexpr int16_t Divide(int16_t left, int16_t right) noexcept {
        return static_cast<int16_t>((left * 16) / right);
    }

    int16_t value = 0;
};


/**
 * Template class for converting arbitrary Pica float types to IEEE 754 32-bit single-precision
 * floating point.
 *
 * When decoding, format is as follows:
 *  - The first `M` bits are the mantissa
 *  - The next `E` bits are the exponent
 *  - The last bit is the sign bit
 *
 * @todo Verify on HW if this conversion is sufficiently accurate.
 */
template<unsigned M, unsigned E>
struct Float {
public:
    static Float<M, E> FromFloat32(float val) {
        Float<M, E> ret;
        ret.value = val;
        return ret;
    }

    static Float<M, E> FromRaw(u32 hex) {
        Float<M, E> res;

        const int width = M + E + 1;
        const int bias = 128 - (1 << (E - 1));
        const int exponent = (hex >> M) & ((1 << E) - 1);
        const unsigned mantissa = hex & ((1 << M) - 1);

        if (hex & ((1 << (width - 1)) - 1))
            hex = ((hex >> (E + M)) << 31) | (mantissa << (23 - M)) | ((exponent + bias) << 23);
        else
            hex = ((hex >> (E + M)) << 31);

        std::memcpy(&res.value, &hex, sizeof(float));

        return res;
    }

    static Float<M, E> Zero() {
        return FromFloat32(0.f);
    }

    // Not recommended for anything but logging
    float ToFloat32() const {
        return value;
    }

    Float<M, E> operator * (const Float<M, E>& flt) const {
        if ((this->value == 0.f && !std::isnan(flt.value)) ||
            (flt.value == 0.f && !std::isnan(this->value)))
            // PICA gives 0 instead of NaN when multiplying by inf
            return Zero();
        return Float<M, E>::FromFloat32(ToFloat32() * flt.ToFloat32());
    }

    Float<M, E> operator / (const Float<M, E>& flt) const {
        return Float<M, E>::FromFloat32(ToFloat32() / flt.ToFloat32());
    }

    Float<M, E> operator + (const Float<M, E>& flt) const {
        return Float<M, E>::FromFloat32(ToFloat32() + flt.ToFloat32());
    }

    Float<M, E> operator - (const Float<M, E>& flt) const {
        return Float<M, E>::FromFloat32(ToFloat32() - flt.ToFloat32());
    }

    Float<M, E>& operator *= (const Float<M, E>& flt) {
        if ((this->value == 0.f && !std::isnan(flt.value)) ||
            (flt.value == 0.f && !std::isnan(this->value)))
            // PICA gives 0 instead of NaN when multiplying by inf
            *this = Zero();
        else value *= flt.ToFloat32();
        return *this;
    }

    Float<M, E>& operator /= (const Float<M, E>& flt) {
        value /= flt.ToFloat32();
        return *this;
    }

    Float<M, E>& operator += (const Float<M, E>& flt) {
        value += flt.ToFloat32();
        return *this;
    }

    Float<M, E>& operator -= (const Float<M, E>& flt) {
        value -= flt.ToFloat32();
        return *this;
    }

    Float<M, E> operator - () const {
        return Float<M, E>::FromFloat32(-ToFloat32());
    }

    bool operator < (const Float<M, E>& flt) const {
        return ToFloat32() < flt.ToFloat32();
    }

    bool operator > (const Float<M, E>& flt) const {
        return ToFloat32() > flt.ToFloat32();
    }

    bool operator >= (const Float<M, E>& flt) const {
        return ToFloat32() >= flt.ToFloat32();
    }

    bool operator <= (const Float<M, E>& flt) const {
        return ToFloat32() <= flt.ToFloat32();
    }

    bool operator == (const Float<M, E>& flt) const {
        return ToFloat32() == flt.ToFloat32();
    }

    bool operator != (const Float<M, E>& flt) const {
        return ToFloat32() != flt.ToFloat32();
    }

private:
    static const unsigned MASK = (1 << (M + E + 1)) - 1;
    static const unsigned MANTISSA_MASK = (1 << M) - 1;
    static const unsigned EXPONENT_MASK = (1 << E) - 1;

    // Stored as a regular float, merely for convenience
    // TODO: Perform proper arithmetic on this!
    float value;
};

using float24 = Float<16, 7>;
using float20 = Float<12, 7>;
using float16 = Float<10, 5>;

} // namespace Pica
