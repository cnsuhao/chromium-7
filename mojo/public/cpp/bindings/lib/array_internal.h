// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_PUBLIC_CPP_BINDINGS_LIB_ARRAY_INTERNAL_H_
#define MOJO_PUBLIC_CPP_BINDINGS_LIB_ARRAY_INTERNAL_H_

#include <stddef.h>
#include <stdint.h>
#include <new>
#include <utility>
#include <vector>

#include "base/logging.h"
#include "mojo/public/c/system/macros.h"
#include "mojo/public/cpp/bindings/lib/bindings_internal.h"
#include "mojo/public/cpp/bindings/lib/buffer.h"
#include "mojo/public/cpp/bindings/lib/serialization_util.h"
#include "mojo/public/cpp/bindings/lib/template_util.h"
#include "mojo/public/cpp/bindings/lib/validate_params.h"
#include "mojo/public/cpp/bindings/lib/validation_context.h"
#include "mojo/public/cpp/bindings/lib/validation_errors.h"
#include "mojo/public/cpp/bindings/lib/validation_util.h"

namespace mojo {
namespace internal {

// std::numeric_limits<uint32_t>::max() is not a compile-time constant (until
// C++11).
const uint32_t kMaxUint32 = 0xFFFFFFFF;

template <typename K, typename V>
class Map_Data;

std::string MakeMessageWithArrayIndex(const char* message,
                                      size_t size,
                                      size_t index);

std::string MakeMessageWithExpectedArraySize(const char* message,
                                             size_t size,
                                             size_t expected_size);

template <typename T>
struct ArrayDataTraits {
  typedef T StorageType;
  typedef T& Ref;
  typedef T const& ConstRef;

  static const uint32_t kMaxNumElements =
      (kMaxUint32 - sizeof(ArrayHeader)) / sizeof(StorageType);

  static uint32_t GetStorageSize(uint32_t num_elements) {
    DCHECK(num_elements <= kMaxNumElements);
    return sizeof(ArrayHeader) + sizeof(StorageType) * num_elements;
  }
  static Ref ToRef(StorageType* storage, size_t offset) {
    return storage[offset];
  }
  static ConstRef ToConstRef(const StorageType* storage, size_t offset) {
    return storage[offset];
  }
};

template <typename P>
struct ArrayDataTraits<P*> {
  typedef Pointer<P> StorageType;
  typedef P*& Ref;
  typedef P* const& ConstRef;

  static const uint32_t kMaxNumElements =
      (kMaxUint32 - sizeof(ArrayHeader)) / sizeof(StorageType);

  static uint32_t GetStorageSize(uint32_t num_elements) {
    DCHECK(num_elements <= kMaxNumElements);
    return sizeof(ArrayHeader) + sizeof(StorageType) * num_elements;
  }
  static Ref ToRef(StorageType* storage, size_t offset) {
    return storage[offset].ptr;
  }
  static ConstRef ToConstRef(const StorageType* storage, size_t offset) {
    return storage[offset].ptr;
  }
};

// Specialization of Arrays for bools, optimized for space. It has the
// following differences from a generalized Array:
// * Each element takes up a single bit of memory.
// * Accessing a non-const single element uses a helper class |BitRef|, which
// emulates a reference to a bool.
template <>
struct ArrayDataTraits<bool> {
  // Helper class to emulate a reference to a bool, used for direct element
  // access.
  class BitRef {
   public:
    ~BitRef();
    BitRef& operator=(bool value);
    BitRef& operator=(const BitRef& value);
    operator bool() const;

   private:
    friend struct ArrayDataTraits<bool>;
    BitRef(uint8_t* storage, uint8_t mask);
    BitRef();
    uint8_t* storage_;
    uint8_t mask_;
  };

  // Because each element consumes only 1/8 byte.
  static const uint32_t kMaxNumElements = kMaxUint32;

  typedef uint8_t StorageType;
  typedef BitRef Ref;
  typedef bool ConstRef;

  static uint32_t GetStorageSize(uint32_t num_elements) {
    return sizeof(ArrayHeader) + ((num_elements + 7) / 8);
  }
  static BitRef ToRef(StorageType* storage, size_t offset) {
    return BitRef(&storage[offset / 8], 1 << (offset % 8));
  }
  static bool ToConstRef(const StorageType* storage, size_t offset) {
    return (storage[offset / 8] & (1 << (offset % 8))) != 0;
  }
};

// What follows is code to support the serialization of Array_Data<T>. There
// are four interesting cases: arrays of primitives, arrays of handles,
// arrays of objects and arrays of unions.
// Arrays of objects are represented as arrays of pointers to objects. Arrays
// of unions are inlined so they are not pointers, but comparing with primitives
// they require more work for serialization/validation.

template <typename T, bool is_union>
struct ArraySerializationHelper;

template <typename T>
struct ArraySerializationHelper<T, false> {
  typedef typename ArrayDataTraits<T>::StorageType ElementType;

  static void EncodePointers(const ArrayHeader* header,
                             ElementType* elements) {}

  static void DecodePointers(const ArrayHeader* header,
                             ElementType* elements) {}

  static bool ValidateElements(const ArrayHeader* header,
                               const ElementType* elements,
                               ValidationContext* validation_context,
                               const ContainerValidateParams* validate_params) {
    DCHECK(!validate_params->element_is_nullable)
        << "Primitive type should be non-nullable";
    DCHECK(!validate_params->element_validate_params)
        << "Primitive type should not have array validate params";

    if (!validate_params->validate_enum_func)
      return true;

    // Enum validation.
    for (uint32_t i = 0; i < header->num_elements; ++i) {
      if (!validate_params->validate_enum_func(elements[i], validation_context))
        return false;
    }
    return true;
  }
};

template <>
struct ArraySerializationHelper<Handle_Data, false> {
  typedef ArrayDataTraits<Handle_Data>::StorageType ElementType;

  static void EncodePointers(const ArrayHeader* header,
                             ElementType* elements) {}

  static void DecodePointers(const ArrayHeader* header,
                             ElementType* elements) {}

  static bool ValidateElements(const ArrayHeader* header,
                               const ElementType* elements,
                               ValidationContext* validation_context,
                               const ContainerValidateParams* validate_params) {
    DCHECK(!validate_params->element_validate_params)
        << "Handle type should not have array validate params";

    for (uint32_t i = 0; i < header->num_elements; ++i) {
      if (!validate_params->element_is_nullable && !elements[i].is_valid()) {
        ReportValidationError(
            validation_context,
            VALIDATION_ERROR_UNEXPECTED_INVALID_HANDLE,
            MakeMessageWithArrayIndex(
                "invalid handle in array expecting valid handles",
                header->num_elements,
                i).c_str());
        return false;
      }
      if (!validation_context->ClaimHandle(elements[i])) {
        ReportValidationError(validation_context,
                              VALIDATION_ERROR_ILLEGAL_HANDLE);
        return false;
      }
    }
    return true;
  }
};

template <typename P>
struct ArraySerializationHelper<P*, false> {
  typedef typename ArrayDataTraits<P*>::StorageType ElementType;

  static void EncodePointers(const ArrayHeader* header, ElementType* elements) {
    for (uint32_t i = 0; i < header->num_elements; ++i)
      Encode(&elements[i]);
  }

  static void DecodePointers(const ArrayHeader* header, ElementType* elements) {
    for (uint32_t i = 0; i < header->num_elements; ++i)
      Decode(&elements[i]);
  }

  static bool ValidateElements(const ArrayHeader* header,
                               const ElementType* elements,
                               ValidationContext* validation_context,
                               const ContainerValidateParams* validate_params) {
    for (uint32_t i = 0; i < header->num_elements; ++i) {
      if (!validate_params->element_is_nullable && !elements[i].offset) {
        ReportValidationError(
            validation_context,
            VALIDATION_ERROR_UNEXPECTED_NULL_POINTER,
            MakeMessageWithArrayIndex("null in array expecting valid pointers",
                                      header->num_elements,
                                      i).c_str());
        return false;
      }
      if (!ValidateEncodedPointer(&elements[i].offset)) {
        ReportValidationError(validation_context,
                              VALIDATION_ERROR_ILLEGAL_POINTER);
        return false;
      }
      if (!ValidateCaller<P>::Run(DecodePointerRaw(&elements[i].offset),
                                  validation_context,
                                  validate_params->element_validate_params)) {
        return false;
      }
    }
    return true;
  }

 private:
  template <typename T>
  struct ValidateCaller {
    static bool Run(const void* data,
                    ValidationContext* validation_context,
                    const ContainerValidateParams* validate_params) {
      DCHECK(!validate_params)
          << "Struct type should not have array validate params";

      return T::Validate(data, validation_context);
    }
  };

  template <typename Key, typename Value>
  struct ValidateCaller<Map_Data<Key, Value>> {
    static bool Run(const void* data,
                    ValidationContext* validation_context,
                    const ContainerValidateParams* validate_params) {
      return Map_Data<Key, Value>::Validate(data, validation_context,
                                            validate_params);
    }
  };

  template <typename T>
  struct ValidateCaller<Array_Data<T>> {
    static bool Run(const void* data,
                    ValidationContext* validation_context,
                    const ContainerValidateParams* validate_params) {
      return Array_Data<T>::Validate(data, validation_context, validate_params);
    }
  };
};

template <typename U>
struct ArraySerializationHelper<U, true> {
  typedef typename ArrayDataTraits<U>::StorageType ElementType;

  static void EncodePointers(const ArrayHeader* header, ElementType* elements) {
    for (uint32_t i = 0; i < header->num_elements; ++i)
      elements[i].EncodePointers();
  }

  static void DecodePointers(const ArrayHeader* header, ElementType* elements) {
    for (uint32_t i = 0; i < header->num_elements; ++i)
      elements[i].DecodePointers();
  }

  static bool ValidateElements(const ArrayHeader* header,
                               const ElementType* elements,
                               ValidationContext* validation_context,
                               const ContainerValidateParams* validate_params) {
    for (uint32_t i = 0; i < header->num_elements; ++i) {
      if (!validate_params->element_is_nullable && elements[i].is_null()) {
        ReportValidationError(
            validation_context,
            VALIDATION_ERROR_UNEXPECTED_NULL_POINTER,
            MakeMessageWithArrayIndex("null in array expecting valid unions",
                                      header->num_elements, i)
                .c_str());
        return false;
      }
      if (!ElementType::Validate(elements + i, validation_context, true))
        return false;
    }
    return true;
  }
};

template <typename T>
class Array_Data {
 public:
  using Traits = ArrayDataTraits<T>;
  using StorageType = typename Traits::StorageType;
  using Ref = typename Traits::Ref;
  using ConstRef = typename Traits::ConstRef;
  using Helper = ArraySerializationHelper<T, IsUnionDataType<T>::value>;
  using Element = T;

  // Returns null if |num_elements| or the corresponding storage size cannot be
  // stored in uint32_t.
  static Array_Data<T>* New(size_t num_elements, Buffer* buf) {
    if (num_elements > Traits::kMaxNumElements)
      return nullptr;

    uint32_t num_bytes =
        Traits::GetStorageSize(static_cast<uint32_t>(num_elements));
    return new (buf->Allocate(num_bytes))
        Array_Data<T>(num_bytes, static_cast<uint32_t>(num_elements));
  }

  static bool Validate(const void* data,
                       ValidationContext* validation_context,
                       const ContainerValidateParams* validate_params) {
    if (!data)
      return true;
    if (!IsAligned(data)) {
      ReportValidationError(validation_context,
                            VALIDATION_ERROR_MISALIGNED_OBJECT);
      return false;
    }
    if (!validation_context->IsValidRange(data, sizeof(ArrayHeader))) {
      ReportValidationError(validation_context,
                            VALIDATION_ERROR_ILLEGAL_MEMORY_RANGE);
      return false;
    }
    const ArrayHeader* header = static_cast<const ArrayHeader*>(data);
    if (header->num_elements > Traits::kMaxNumElements ||
        header->num_bytes < Traits::GetStorageSize(header->num_elements)) {
      ReportValidationError(validation_context,
                            VALIDATION_ERROR_UNEXPECTED_ARRAY_HEADER);
      return false;
    }
    if (validate_params->expected_num_elements != 0 &&
        header->num_elements != validate_params->expected_num_elements) {
      ReportValidationError(
          validation_context,
          VALIDATION_ERROR_UNEXPECTED_ARRAY_HEADER,
          MakeMessageWithExpectedArraySize(
              "fixed-size array has wrong number of elements",
              header->num_elements,
              validate_params->expected_num_elements).c_str());
      return false;
    }
    if (!validation_context->ClaimMemory(data, header->num_bytes)) {
      ReportValidationError(validation_context,
                            VALIDATION_ERROR_ILLEGAL_MEMORY_RANGE);
      return false;
    }

    const Array_Data<T>* object = static_cast<const Array_Data<T>*>(data);
    return Helper::ValidateElements(&object->header_, object->storage(),
                                    validation_context, validate_params);
  }

  size_t size() const { return header_.num_elements; }

  Ref at(size_t offset) {
    DCHECK(offset < static_cast<size_t>(header_.num_elements));
    return Traits::ToRef(storage(), offset);
  }

  ConstRef at(size_t offset) const {
    DCHECK(offset < static_cast<size_t>(header_.num_elements));
    return Traits::ToConstRef(storage(), offset);
  }

  StorageType* storage() {
    return reinterpret_cast<StorageType*>(reinterpret_cast<char*>(this) +
                                          sizeof(*this));
  }

  const StorageType* storage() const {
    return reinterpret_cast<const StorageType*>(
        reinterpret_cast<const char*>(this) + sizeof(*this));
  }

  void EncodePointers() { Helper::EncodePointers(&header_, storage()); }
  void DecodePointers() { Helper::DecodePointers(&header_, storage()); }

 private:
  Array_Data(uint32_t num_bytes, uint32_t num_elements) {
    header_.num_bytes = num_bytes;
    header_.num_elements = num_elements;
  }
  ~Array_Data() = delete;

  internal::ArrayHeader header_;

  // Elements of type internal::ArrayDataTraits<T>::StorageType follow.
};
static_assert(sizeof(Array_Data<char>) == 8, "Bad sizeof(Array_Data)");

// UTF-8 encoded
using String_Data = Array_Data<char>;

}  // namespace internal
}  // namespace mojo

#endif  // MOJO_PUBLIC_CPP_BINDINGS_LIB_ARRAY_INTERNAL_H_