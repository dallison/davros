#include "davros/serdes/other_msgs/Other.h"
#include "davros/serdes/runtime.h"
#include "davros/serdes/test_msgs/All.h"
#include "davros/zeros/other_msgs/Other.h"
#include "davros/zeros/runtime.h"
#include "davros/zeros/test_msgs/All.h"
#include "toolbelt/hexdump.h"
#include <gtest/gtest.h>

using PayloadBuffer = davros::zeros::PayloadBuffer;

TEST(Runtime, ZeroToSerdes) {
  char *buffer = (char *)malloc(4096);

  PayloadBuffer *pb = new (buffer) PayloadBuffer(4096);

  // Allocate space for a message containing an offset for the string.
  PayloadBuffer::AllocateMainMessage(&pb,
                                     other_msgs::zeros::Other::BinarySize());

  pb->Dump(std::cout);
  other_msgs::zeros::Other other(std::make_shared<PayloadBuffer *>(pb),
                                 pb->message);

  other.header->seq = 255;
  other.header->frame_id = "frame1";

  other.bar = "foobar";
  other.value = 1234;

  other.en = other_msgs::zeros::Enum::FOO;

  toolbelt::Hexdump(buffer, pb->Size());

  size_t length = other.SerializedSize();
  std::cout << length << std::endl;
  char *serdes_buffer = (char *)malloc(length);

  auto status = other.SerializeToArray(serdes_buffer, length);
  std::cerr << status << std::endl;
  ASSERT_TRUE(status.ok());
  toolbelt::Hexdump(serdes_buffer, length);

  // Deserialize from buffer and print it:
  other_msgs::Other sother;
  status = sother.DeserializeFromArray(serdes_buffer, length);
  ASSERT_TRUE(status.ok());
  std::cout << sother.DebugString();
  free(serdes_buffer);
  free(buffer);
}

TEST(Runtime, Pipeline) {
  // Build serializable message and serialize it.
  other_msgs::Other sother;

  sother.header.seq = 255;
  sother.header.frame_id = "frame1";
  sother.bar = "foobar";
  sother.value = 1234;
  size_t length1 = sother.SerializedSize();
  char *serdes_buffer1 = (char *)malloc(length1);

  auto status = sother.SerializeToArray(serdes_buffer1, length1);
  std::cerr << status << std::endl;
  ASSERT_TRUE(status.ok());

  // serdes_buffer1 contains the serialized sother message.
  std::cout << "Serialized message\n";
  toolbelt::Hexdump(serdes_buffer1, length1);

  // Build a zero-copy message from the serialized message.
  char *zbuffer = (char *)malloc(4096);
  PayloadBuffer *pb = new (zbuffer) PayloadBuffer(4096);
  PayloadBuffer::AllocateMainMessage(&pb,
                                     other_msgs::zeros::Other::BinarySize());

  std::cout << "binary size: " << other_msgs::zeros::Other::BinarySize()
            << std::endl;
  other_msgs::zeros::Other zother(std::make_shared<PayloadBuffer *>(pb),
                                  pb->message);

  status = zother.DeserializeFromArray(serdes_buffer1, length1);
  ASSERT_TRUE(status.ok());
  // zbuffer contains the zero copy message.
  std::cout << "Zero copy message\n";
  toolbelt::Hexdump(zbuffer, pb->Size());
  pb->Dump(std::cout);

  // Serialize zero-copy to serdes buffer.
  size_t length2 = zother.SerializedSize();
  char *serdes_buffer2 = (char *)malloc(length2);

  status = zother.SerializeToArray(serdes_buffer2, length2);
  std::cerr << status << std::endl;
  ASSERT_TRUE(status.ok());
  std::cout << "Serialized Zero copy message\n";
  toolbelt::Hexdump(serdes_buffer2, length2);

  // serdes_buffer2 contains the serialized zother message.

  // Deserialize into another message.
  other_msgs::Other sother2;
  status = sother2.DeserializeFromArray(serdes_buffer2, length2);
  ASSERT_TRUE(status.ok());

  // Compare original message and the one that passed through the serialization
  // pipeline.
  ASSERT_EQ(sother, sother2);

  free(serdes_buffer1);
  free(serdes_buffer2);
  free(zbuffer);
}

TEST(Runtime, AllZeroToSerdes) {
  char *buffer = (char *)malloc(8192);

  PayloadBuffer *pb = new (buffer) PayloadBuffer(8192);

  // Allocate space for a message containing an offset for the string.
  PayloadBuffer::AllocateMainMessage(&pb, test_msgs::zeros::All::BinarySize());

  test_msgs::zeros::All all(std::make_shared<PayloadBuffer *>(pb), pb->message);

  // Set the fields.
  all.i8 = 1;
  all.ui8 = 2;
  all.i16 = 3;
  all.ui16 = 4;
  all.i32 = 5;
  all.ui32 = 6;
  all.i64 = 7;
  all.ui64 = 8;
  all.f32 = 9;
  all.f64 = 10;
  all.s = "dave";
  all.t = {45, 67};
  all.d = {78, 89};

  all.n->foo = 1234;
  all.n->bar = "bar";

  all.e8 = test_msgs::zeros::Enum8::X1;
  all.e16 = test_msgs::zeros::Enum16::X2;
  all.e32 = test_msgs::zeros::Enum32::X3;
  all.e64 = test_msgs::zeros::Enum64::X1;
  
  toolbelt::Hexdump(buffer, pb->Size());

  size_t length = all.SerializedSize();
  std::cout << length << std::endl;
  char *serdes_buffer = (char *)malloc(length);

  auto status = all.SerializeToArray(serdes_buffer, length);
  std::cerr << status << std::endl;
  ASSERT_TRUE(status.ok());
  toolbelt::Hexdump(serdes_buffer, length);

  // Deserialize from buffer and print it:
  test_msgs::serdes::All sall;
  status = sall.DeserializeFromArray(serdes_buffer, length);
  ASSERT_TRUE(status.ok());
  std::cout << sall.DebugString();
  free(serdes_buffer);
  free(buffer);
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}