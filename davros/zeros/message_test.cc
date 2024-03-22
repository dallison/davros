#include "davros/zeros/message.h"
#include "davros/zeros/payload_buffer.h"
#include "davros/zeros/runtime.h"
#include "toolbelt/hexdump.h"
#include <gtest/gtest.h>
#include <sstream>

using PayloadBuffer = davros::zeros::PayloadBuffer;
using BufferOffset = davros::zeros::BufferOffset;
using Message = davros::zeros::Message;
using VectorHeader = davros::zeros::VectorHeader;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winvalid-offsetof"

struct InnerMessage : public Message {
  InnerMessage(std::shared_ptr<PayloadBuffer *> buffer, BufferOffset offset)
      : Message(buffer, offset), str(offsetof(InnerMessage, str), 0),
        f(offsetof(InnerMessage, f), 8) {}
  static uint32_t FixedSize() { return 16; }
  davros::zeros::StringField str;
  davros::zeros::Uint64Field f;
};

struct TestMessage : public Message {
  TestMessage(std::shared_ptr<PayloadBuffer *> buffer, BufferOffset offset)
      : Message(buffer, offset), x(offsetof(TestMessage, x), 0),
        y(offsetof(TestMessage, y), 8), s(offsetof(TestMessage, s), 16),
        m(buffer, offsetof(TestMessage, m), 20),
        arr(offsetof(TestMessage, arr), 20 + InnerMessage::FixedSize()),
        vec(offsetof(TestMessage, vec),
            20 + InnerMessage::FixedSize() + 10 * sizeof(int32_t)) {}
  static uint32_t FixedSize() {
    return 4 + 4 + 8 + 4 + 4 + InnerMessage::FixedSize() +
           10 * sizeof(int32_t) + sizeof(VectorHeader);
  }
  davros::zeros::Uint32Field x;
  davros::zeros::Uint64Field y;
  davros::zeros::StringField s;
  davros::zeros::MessageField<InnerMessage> m;
  davros::zeros::PrimitiveArrayField<int32_t, 10> arr;
  davros::zeros::PrimitiveVectorField<int32_t> vec;
};
#pragma clang diagnostic pop

TEST(MessageTest, Basic) {
  char *buffer = (char *)malloc(4096);
  PayloadBuffer *pb = new (buffer) PayloadBuffer(4096);

  // Allocate space for a message containing an offset for the string.
  PayloadBuffer::AllocateMainMessage(&pb, TestMessage::FixedSize());

  TestMessage msg(std::make_shared<PayloadBuffer *>(pb), pb->message);
  msg.x = 1234;
  msg.y = 0xffff;

  pb->Dump(std::cout);
  toolbelt::Hexdump(pb, pb->hwm);

  uint32_t x = msg.x;
  ASSERT_EQ(1234, x);

  uint64_t y = msg.y;
  ASSERT_EQ(0xffff, y);
}

TEST(MessageTest, String) {
  char *buffer = (char *)malloc(4096);
  PayloadBuffer *pb = new (buffer) PayloadBuffer(4096);

  // Allocate space for a message containing an offset for the string.
  PayloadBuffer::AllocateMainMessage(&pb, TestMessage::FixedSize());

  TestMessage msg(std::make_shared<PayloadBuffer *>(pb), pb->message);
  msg.x = 0xffffffff;
  msg.y = 0xeeeeeeeeeeeeeeee;

  msg.s = "hello world";

  pb->Dump(std::cout);
  toolbelt::Hexdump(pb, pb->hwm);

  std::string_view s = msg.s;
  ASSERT_EQ("hello world", s);
}

TEST(MessageTest, Message) {
  char *buffer = (char *)malloc(4096);
  PayloadBuffer *pb = new (buffer) PayloadBuffer(4096);

  // Allocate space for a message containing an offset for the string.
  PayloadBuffer::AllocateMainMessage(&pb, TestMessage::FixedSize());

  TestMessage msg(std::make_shared<PayloadBuffer *>(pb), pb->message);
  msg.x = 0xffffffff;
  msg.y = 0xeeeeeeeeeeeeeeee;
  msg.s = "now is the time for all good men to come to the aid of the party";

  // NOTE use of -> here because you can't overload the . operator.
  msg.m->str = "hello world";

  // Uses the 'operator M&' overload in the MessageField to get access
  // to the actual message type.
  InnerMessage &m = msg.m;
  m.f = 0xaaaaaaaaaaaaaaaa;

  pb->Dump(std::cout);
  toolbelt::Hexdump(pb, pb->hwm);

  std::string_view s = msg.m->str;
  ASSERT_EQ("hello world", s);

  ASSERT_EQ(0xaaaaaaaaaaaaaaaa, uint64_t(msg.m->f));
}

TEST(MessageTest, Array) {
  char *buffer = (char *)malloc(4096);
  PayloadBuffer *pb = new (buffer) PayloadBuffer(4096);

  // Allocate space for a message containing an offset for the string.
  PayloadBuffer::AllocateMainMessage(&pb, TestMessage::FixedSize());

  TestMessage msg(std::make_shared<PayloadBuffer *>(pb), pb->message);
  for (int i = 0; i < 10; i++) {
    msg.arr[i] = i + 1;
  }

  pb->Dump(std::cout);
  toolbelt::Hexdump(pb, pb->hwm);

  int x = 1;
  for (auto &i : msg.arr) {
    ASSERT_EQ(x, i);
    x++;
  }
}

TEST(MessageTest, BasicVector) {
  char *buffer = (char *)malloc(4096);
  PayloadBuffer *pb = new (buffer) PayloadBuffer(4096);

  // Allocate space for a message containing an offset for the string.
  PayloadBuffer::AllocateMainMessage(&pb, TestMessage::FixedSize());

  TestMessage msg(std::make_shared<PayloadBuffer *>(pb), pb->message);
  msg.vec.push_back(0xffee);

  pb->Dump(std::cout);
  toolbelt::Hexdump(pb, pb->hwm);

  ASSERT_EQ(1, msg.vec.size());
  ASSERT_EQ(0xffee, msg.vec[0]);
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}