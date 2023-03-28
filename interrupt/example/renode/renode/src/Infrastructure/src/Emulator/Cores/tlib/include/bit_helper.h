/*
 * Copyright (c) Antmicro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BIT_HELPER_H_
#define BIT_HELPER_H_

#include "infrastructure.h"

// Used by deposit32.
static inline uint64_t deposit64(uint64_t dst_val, uint8_t start, uint8_t length, uint64_t val);

/* Deposits 'length' bits from 'val' into 'dst_val' at 'start' bit. */
static inline uint32_t deposit32(uint32_t dst_val, uint8_t start, uint8_t length, uint32_t val)
{
    tlib_assert(start + length <= 32);
    return deposit64(dst_val, start, length, val);
}

/* Deposits 'length' bits from 'val' into 'dst_val' at 'start' bit. */
static inline uint64_t deposit64(uint64_t dst_val, uint8_t start, uint8_t length, uint64_t val)
{
    tlib_assert(start + length <= 64);

    // Number with only relevant bits ('start' to 'start+length-1') set.
    uint64_t relevant_bits = (UINT64_MAX >> (64 - length)) << start;

    // Shift value into the start bit and reset the irrelevant bits.
    val = (val << start) & relevant_bits;

    // Reset the relevant bits in the destination value.
    dst_val &= ~relevant_bits;

    // Return the above values merged.
    return dst_val | val;
}

/* Extracts 'length' bits of 'value' at 'start' bit. */
static inline uint32_t extract32(uint32_t value, uint8_t start, uint8_t length)
{
    tlib_assert(start + length <= 32);
    return (value >> start) & (UINT32_MAX >> (32 - length));
}

/* Extracts 'length' bits of 'value' at 'start' bit. */
static inline uint64_t extract64(uint64_t value, uint8_t start, uint8_t length)
{
    tlib_assert(start + length <= 64);
    return (value >> start) & (UINT64_MAX >> (64 - length));
}

static inline int32_t sextract32(uint32_t value, uint8_t start, uint8_t length)
{
    int32_t result = (int32_t)extract32(value, start, length);
    if (result >> (length - 1)) {
        // Set all the bits from 'length' to bit 31.
        result |= UINT32_MAX << length;
    }
    return result;
}

static inline int64_t sextract64(uint64_t value, uint8_t start, uint8_t length)
{
    int64_t result = (int64_t)extract64(value, start, length);
    if (result >> (length - 1)) {
        // Set all the bits from 'length' to bit 63.
        result |= UINT64_MAX << length;
    }
    return result;
}

#endif  // BIT_HELPER_H_
