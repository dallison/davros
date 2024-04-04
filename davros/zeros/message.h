#pragma once

#include <stdint.h>
#include <memory>
#include "toolbelt/payload_buffer.h"

namespace davros::zeros {

// Payload buffers can move. All messages in a message tree must all use the
// same payload buffer. We hold a shared pointer to a pointer to the payload
// buffer.
//
//            +-------+
//            |       |
//            V       |
// +---------------+  |
// |               |  |
// | toolbelt::PayloadBuffer |  |
// |               |  |
// +---------------+  |
//                    |
//                    |
// +---------------+  |
// |     *         +--+
// +---------------+
//       ^ ^
//       | |
//       | +--------------------------+
//       +------------+   +--------+  |
//                    |   |        V  |
// +---------------+  |   |      +---+--------+
// |    buffer     +--+   |      |   buffer    |
// +---------------+      |      +-------------+
// |               |      |      |             |
// |   Message     |      |      |  Message    |
// |               |      |      |  Field      |
// |               +------+      |             |
// +---------------+             +-------------+

struct Message {
  Message() = default;
  Message(std::shared_ptr<toolbelt::PayloadBuffer *> pb, toolbelt::BufferOffset start)
      : buffer(pb), absolute_binary_offset(start) {}
  std::shared_ptr<toolbelt::PayloadBuffer *> buffer;
  toolbelt::BufferOffset absolute_binary_offset;

  // 'field' is the offset from the start of the message to the field (positive)
  // Subtract the field offset from the field to get the address of the
  // std::shared_ptr to the pointer to the toolbelt::PayloadBuffer.
  static toolbelt::PayloadBuffer *GetBuffer(const void *field, uint32_t offset) {
    const std::shared_ptr<toolbelt::PayloadBuffer *> *pb =
        reinterpret_cast<const std::shared_ptr<toolbelt::PayloadBuffer *> *>(
            reinterpret_cast<const char *>(field) - offset);
    return *pb->get();
  }

  static toolbelt::PayloadBuffer **GetBufferAddr(const void *field, uint32_t offset) {
    const std::shared_ptr<toolbelt::PayloadBuffer *> *pb =
        reinterpret_cast<const std::shared_ptr<toolbelt::PayloadBuffer *> *>(
            reinterpret_cast<const char *>(field) - offset);
    return pb->get();
  }

  static std::shared_ptr<toolbelt::PayloadBuffer *> GetSharedBuffer(void *field,
                                                          uint32_t offset) {
    std::shared_ptr<toolbelt::PayloadBuffer *> *pb =
        reinterpret_cast<std::shared_ptr<toolbelt::PayloadBuffer *> *>(
            reinterpret_cast<char *>(field) - offset);
    return *pb;
  }

  static toolbelt::BufferOffset GetMessageBinaryStart(const void *field,
                                            uint32_t offset) {
    const Message *msg = reinterpret_cast<const Message *>(
        reinterpret_cast<const char *>(field) - offset);
    return msg->absolute_binary_offset;
  }
};

}  // namespace davros::zeros
