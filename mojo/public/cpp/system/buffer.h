// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file provides a C++ wrapping around the Mojo C API for shared buffers,
// replacing the prefix of "Mojo" with a "mojo" namespace, and using more
// strongly-typed representations of |MojoHandle|s.
//
// Please see "mojo/public/c/system/buffer.h" for complete documentation of the
// API.

#ifndef MOJO_PUBLIC_CPP_SYSTEM_BUFFER_H_
#define MOJO_PUBLIC_CPP_SYSTEM_BUFFER_H_

#include <stdint.h>

#include <memory>

#include "base/compiler_specific.h"
#include "base/logging.h"
#include "mojo/public/c/system/buffer.h"
#include "mojo/public/cpp/system/handle.h"

namespace mojo {
namespace internal {

struct Unmapper {
  void operator()(void* buffer) {
    MojoResult result = MojoUnmapBuffer(buffer);
    DCHECK_EQ(MOJO_RESULT_OK, result);
  }
};

}  // namespace internal

using ScopedSharedBufferMapping = std::unique_ptr<void, internal::Unmapper>;

class SharedBufferHandle;

typedef ScopedHandleBase<SharedBufferHandle> ScopedSharedBufferHandle;

// A strongly-typed representation of a |MojoHandle| referring to a shared
// buffer.
class SharedBufferHandle : public Handle {
 public:
  enum class AccessMode {
    READ_WRITE,
    READ_ONLY,
  };

  SharedBufferHandle() {}
  explicit SharedBufferHandle(MojoHandle value) : Handle(value) {}

  // Copying and assignment allowed.

  // Creates a new SharedBufferHandle. Returns an invalid handle on failure.
  static ScopedSharedBufferHandle Create(uint64_t num_bytes);

  // Clones this shared buffer handle. If |access_mode| is READ_ONLY or this is
  // a read-only handle, the new handle will be read-only. On failure, this will
  // return an empty result.
  ScopedSharedBufferHandle Clone(AccessMode = AccessMode::READ_WRITE) const;

  // Maps |size| bytes of this shared buffer. On failure, this will return a
  // null mapping.
  ScopedSharedBufferMapping Map(uint64_t size) const;

  // Maps |size| bytes of this shared buffer, starting |offset| bytes into the
  // buffer. On failure, this will return a null mapping.
  ScopedSharedBufferMapping MapAtOffset(uint64_t size, uint64_t offset) const;
};

static_assert(sizeof(SharedBufferHandle) == sizeof(Handle),
              "Bad size for C++ SharedBufferHandle");
static_assert(sizeof(ScopedSharedBufferHandle) == sizeof(SharedBufferHandle),
              "Bad size for C++ ScopedSharedBufferHandle");

// Creates a shared buffer. See |MojoCreateSharedBuffer()| for complete
// documentation.
inline MojoResult CreateSharedBuffer(
    const MojoCreateSharedBufferOptions* options,
    uint64_t num_bytes,
    ScopedSharedBufferHandle* shared_buffer) {
  DCHECK(shared_buffer);
  SharedBufferHandle handle;
  MojoResult rv =
      MojoCreateSharedBuffer(options, num_bytes, handle.mutable_value());
  // Reset even on failure (reduces the chances that a "stale"/incorrect handle
  // will be used).
  shared_buffer->reset(handle);
  return rv;
}

// Duplicates a handle to a buffer, most commonly so that the buffer can be
// shared with other applications. See |MojoDuplicateBufferHandle()| for
// complete documentation.
//
// TODO(ggowan): Rename this to DuplicateBufferHandle since it is making another
// handle to the same buffer, not duplicating the buffer itself.
//
// TODO(vtl): This (and also the functions below) are templatized to allow for
// future/other buffer types. A bit "safer" would be to overload this function
// manually. (The template enforces that the in and out handles be of the same
// type.)
template <class BufferHandleType>
inline MojoResult DuplicateBuffer(
    BufferHandleType buffer,
    const MojoDuplicateBufferHandleOptions* options,
    ScopedHandleBase<BufferHandleType>* new_buffer) {
  DCHECK(new_buffer);
  BufferHandleType handle;
  MojoResult rv = MojoDuplicateBufferHandle(
      buffer.value(), options, handle.mutable_value());
  // Reset even on failure (reduces the chances that a "stale"/incorrect handle
  // will be used).
  new_buffer->reset(handle);
  return rv;
}

// Maps a part of a buffer (specified by |buffer|, |offset|, and |num_bytes|)
// into memory. See |MojoMapBuffer()| for complete documentation.
template <class BufferHandleType>
inline MojoResult MapBuffer(BufferHandleType buffer,
                            uint64_t offset,
                            uint64_t num_bytes,
                            void** pointer,
                            MojoMapBufferFlags flags) {
  DCHECK(buffer.is_valid());
  return MojoMapBuffer(buffer.value(), offset, num_bytes, pointer, flags);
}

// Unmaps a part of a buffer that was previously mapped with |MapBuffer()|.
// See |MojoUnmapBuffer()| for complete documentation.
inline MojoResult UnmapBuffer(void* pointer) {
  DCHECK(pointer);
  return MojoUnmapBuffer(pointer);
}

}  // namespace mojo

#endif  // MOJO_PUBLIC_CPP_SYSTEM_BUFFER_H_