#pragma once

#include <stddef.h>

// SOH [Quest] Where the next draw writes into the streaming vertex buffer. The OpenGL backend keeps
// one large buffer and appends each draw to it, instead of a glBufferData (a new allocation in the
// driver) for each draw. When a draw does not fit at the end, the ring starts again at zero, and the
// backend orphans the buffer so that the GPU can still read the old data. Pure logic: no OpenGL, so
// the host unit tests can drive it.
class VertexRing {
  public:
    struct Slot {
        size_t offset; // Bytes from the start of the buffer. A multiple of the stride.
        bool wrapped;  // The ring started again at zero: orphan the buffer before this write.
        bool fits;     // False if the draw is larger than the ring.
    };

    explicit VertexRing(size_t capacity) : mCapacity(capacity) {
    }

    size_t Capacity() const {
        return mCapacity;
    }

    // bytes: size of the draw's vertex data. stride: bytes per vertex. The offset is a full vertex,
    // so the draw starts at vertex offset / stride, and the attribute pointers of the shader stay
    // relative to the start of the buffer.
    Slot Reserve(size_t bytes, size_t stride) {
        if (bytes > mCapacity) {
            return { 0, false, false };
        }
        size_t offset = (mHead + stride - 1) / stride * stride;
        bool wrapped = false;
        if (offset + bytes > mCapacity) {
            offset = 0;
            wrapped = true;
        }
        mHead = offset + bytes;
        return { offset, wrapped, true };
    }

  private:
    size_t mCapacity;
    size_t mHead = 0;
};
