// Copyright 2019 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_BASE_BIT_FIELD_H_
#define V8_BASE_BIT_FIELD_H_

#include <stdint.h>

#include "src/base/macros.h"

namespace v8 {
namespace base {

// ----------------------------------------------------------------------------
// BitField is a help template for encoding and decode bitfield with
// unsigned content.
// Instantiate them via 'using', which is cheaper than deriving a new class:
// using MyBitField = base::BitField<MyEnum, 4, 2>;
// The BitField class is final to enforce this style over derivation.

template <class T, int shift, int size, class U = uint32_t>
class BitField final {
 public:
  STATIC_ASSERT(std::is_unsigned<U>::value);
  STATIC_ASSERT(shift < 8 * sizeof(U));  // Otherwise shifts by {shift} are UB.
  STATIC_ASSERT(size < 8 * sizeof(U));   // Otherwise shifts by {size} are UB.
  STATIC_ASSERT(shift + size <= 8 * sizeof(U));
  STATIC_ASSERT(size > 0);

  using FieldType = T;

  // A type U mask of bit field.  To use all bits of a type U of x bits
  // in a bitfield without compiler warnings we have to compute 2^x
  // without using a shift count of x in the computation.
  static constexpr int kShift = shift;
  static constexpr int kSize = size;
  static constexpr U kMask = ((U{1} << kShift) << kSize) - (U{1} << kShift);
  static constexpr int kLastUsedBit = kShift + kSize - 1;
  static constexpr U kNumValues = U{1} << kSize;

  // Value for the field with all bits set.
  // If clang complains
  // "constexpr variable 'kMax' must be initialized by a constant expression"
  // on this line, then you're creating a BitField for an enum with more bits
  // than needed for the enum values. Either reduce the BitField size,
  // or give the enum an explicit underlying type.
  static constexpr T kMax = static_cast<T>(kNumValues - 1);

  template <class T2, int size2>
  using Next = BitField<T2, kShift + kSize, size2, U>;

  // Tells whether the provided value fits into the bit field.
  static constexpr bool is_valid(T value) {
    return (static_cast<U>(value) & ~static_cast<U>(kMax)) == 0;
  }

  // Returns a type U with the bit field value encoded.
  static constexpr U encode(T value) {
    DCHECK(is_valid(value));
    return static_cast<U>(value) << kShift;
  }

  // Returns a type U with the bit field value updated.
  static constexpr U update(U previous, T value) {
    return (previous & ~kMask) | encode(value);
  }

  // Extracts the bit field from the value.
  static constexpr T decode(U value) {
    return static_cast<T>((value & kMask) >> kShift);
  }
};

template <class T, int shift, int size>
using BitField8 = BitField<T, shift, size, uint8_t>;

template <class T, int shift, int size>
using BitField16 = BitField<T, shift, size, uint16_t>;

template <class T, int shift, int size>
using BitField64 = BitField<T, shift, size, uint64_t>;

// Helper macros for defining a contiguous sequence of bit fields. Example:
// (backslashes at the ends of respective lines of this multi-line macro
// definition are omitted here to please the compiler)
//
// #define MAP_BIT_FIELD1(V, _)
//   V(IsAbcBit, bool, 1, _)
//   V(IsBcdBit, bool, 1, _)
//   V(CdeBits, int, 5, _)
//   V(DefBits, MutableMode, 1, _)
//
// DEFINE_BIT_FIELDS(MAP_BIT_FIELD1)
// or
// DEFINE_BIT_FIELDS_64(MAP_BIT_FIELD1)
//
#define DEFINE_BIT_FIELD_RANGE_TYPE(Name, Type, Size, _) \
  k##Name##Start, k##Name##End = k##Name##Start + Size - 1,

#define DEFINE_BIT_RANGES(LIST_MACRO)                               \
  struct LIST_MACRO##_Ranges {                                      \
    enum { LIST_MACRO(DEFINE_BIT_FIELD_RANGE_TYPE, _) kBitsCount }; \
  };

#define DEFINE_BIT_FIELD_TYPE(Name, Type, Size, RangesName) \
  using Name = base::BitField<Type, RangesName::k##Name##Start, Size>;

#define DEFINE_BIT_FIELD_64_TYPE(Name, Type, Size, RangesName) \
  using Name = base::BitField64<Type, RangesName::k##Name##Start, Size>;

#define DEFINE_BIT_FIELDS(LIST_MACRO) \
  DEFINE_BIT_RANGES(LIST_MACRO)       \
  LIST_MACRO(DEFINE_BIT_FIELD_TYPE, LIST_MACRO##_Ranges)

#define DEFINE_BIT_FIELDS_64(LIST_MACRO) \
  DEFINE_BIT_RANGES(LIST_MACRO)          \
  LIST_MACRO(DEFINE_BIT_FIELD_64_TYPE, LIST_MACRO##_Ranges)

// ----------------------------------------------------------------------------
// BitSetComputer is a help template for encoding and decoding information for
// a variable number of items in an array.
//
// To encode boolean data in a smi array you would use:
//  using BoolComputer = BitSetComputer<bool, 1, kSmiValueSize, uint32_t>;
//
template <class T, int kBitsPerItem, int kBitsPerWord, class U>
class BitSetComputer {
 public:
  static const int kItemsPerWord = kBitsPerWord / kBitsPerItem;
  static const int kMask = (1 << kBitsPerItem) - 1;

  // The number of array elements required to embed T information for each item.
  static int word_count(int items) {
    if (items == 0) return 0;
    return (items - 1) / kItemsPerWord + 1;
  }

  // The array index to look at for item.
  static int index(int base_index, int item) {
    return base_index + item / kItemsPerWord;
  }

  // Extract T data for a given item from data.
  static T decode(U data, int item) {
    return static_cast<T>((data >> shift(item)) & kMask);
  }

  // Return the encoding for a store of value for item in previous.
  static U encode(U previous, int item, T value) {
    int shift_value = shift(item);
    int set_bits = (static_cast<int>(value) << shift_value);
    return (previous & ~(kMask << shift_value)) | set_bits;
  }

  static int shift(int item) { return (item % kItemsPerWord) * kBitsPerItem; }
};

}  // namespace base
}  // namespace v8

#endif  // V8_BASE_BIT_FIELD_H_
