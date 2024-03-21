#include "davros/zeros/message.h"
#include "davros/zeros/payload_buffer.h"
#include "toolbelt/hexdump.h"
#include <gtest/gtest.h>
#include <sstream>
#include "davros/zeros/runtime.h"

using PayloadBuffer = davros::zeros::PayloadBuffer;
using BufferOffset = davros::zeros::BufferOffset;
using VectorHeader = davros::zeros::VectorHeader;

TEST(BufferTest, Simple) {
  char *buffer = (char *)malloc(4096);
  PayloadBuffer *pb = new (buffer) PayloadBuffer(4096);
  pb->Dump(std::cout);
  toolbelt::Hexdump(pb, 64);

  void *addr = PayloadBuffer::Allocate(&pb, 32, 4);
  memset(addr, 0xda, 32);
  pb->Dump(std::cout);
  std::cout << "Allocated " << addr << std::endl;
  toolbelt::Hexdump(pb, pb->hwm);
  free(buffer);
}

TEST(BufferTest, TwoAllocs) {
  char *buffer = (char *)malloc(4096);
  PayloadBuffer *pb = new (buffer) PayloadBuffer(4096);
  pb->Dump(std::cout);
  toolbelt::Hexdump(pb, 64);

  void *addr = PayloadBuffer::Allocate(&pb, 32, 4);
  memset(addr, 0xda, 32);
  pb->Dump(std::cout);
  std::cout << "Allocated " << addr << std::endl;
  toolbelt::Hexdump(pb, pb->hwm);

  addr = PayloadBuffer::Allocate(&pb, 64, 4);
  memset(addr, 0xda, 64);
  pb->Dump(std::cout);
  std::cout << "Allocated " << addr << std::endl;
  toolbelt::Hexdump(pb, pb->hwm);
  free(buffer);
}

TEST(BufferTest, Free) {
  char *buffer = (char *)malloc(4096);
  PayloadBuffer *pb = new (buffer) PayloadBuffer(4096);
  pb->Dump(std::cout);
  toolbelt::Hexdump(pb, 64);

  void *addr1 = PayloadBuffer::Allocate(&pb, 32, 4);
  memset(addr1, 0xda, 32);
  pb->Dump(std::cout);
  std::cout << "Allocated " << addr1 << std::endl;
  toolbelt::Hexdump(pb, pb->hwm);

  void *addr2 = PayloadBuffer::Allocate(&pb, 64, 4);
  memset(addr2, 0xda, 64);

  pb->Free(addr1);

  pb->Dump(std::cout);
  std::cout << "Allocated " << addr2 << std::endl;
  toolbelt::Hexdump(pb, pb->hwm);
  free(buffer);
}

TEST(BufferTest, FreeThenAlloc) {
  char *buffer = (char *)malloc(4096);
  PayloadBuffer *pb = new (buffer) PayloadBuffer(4096);
  pb->Dump(std::cout);
  toolbelt::Hexdump(pb, 64);

  void *addr1 = PayloadBuffer::Allocate(&pb, 32, 4);
  memset(addr1, 0xda, 32);
  pb->Dump(std::cout);
  std::cout << "Allocated " << addr1 << std::endl;
  toolbelt::Hexdump(pb, pb->hwm);

  void *addr2 = PayloadBuffer::Allocate(&pb, 64, 4);
  memset(addr2, 0xda, 64);

  pb->Free(addr1);

  // 20 bytes fits into the free block.
  void *addr3 = PayloadBuffer::Allocate(&pb, 20, 4);
  memset(addr3, 0xda, 20);

  pb->Dump(std::cout);
  std::cout << "Allocated " << addr2 << std::endl;
  toolbelt::Hexdump(pb, pb->hwm);
  free(buffer);
}

TEST(BufferTest, String) {
  char *buffer = (char *)malloc(4096);
  PayloadBuffer *pb = new (buffer) PayloadBuffer(4096);

  // Allocate space for a message containing an offset for the string.
  PayloadBuffer::AllocateMainMessage(&pb, 32);

  void *addr = pb->ToAddress(pb->message);
  std::cout << "Messsage allocated at " << addr << std::endl;
  pb->Dump(std::cout);
  toolbelt::Hexdump(pb, pb->hwm);
  BufferOffset offset = pb->ToOffset(addr);

  char *s = PayloadBuffer::SetString(&pb, "foobar", offset);
  std::cout << "String allocated at " << (void *)s << std::endl;

  toolbelt::Hexdump(pb, pb->hwm);

  // Now put in a bigger string, replacing the old one.
  s = PayloadBuffer::SetString(&pb, "foobar has been replaced", offset);
  std::cout << "New string allocated at " << (void *)s << std::endl;

  toolbelt::Hexdump(pb, pb->hwm);

  std::string rs = pb->GetString(offset);
  ASSERT_EQ("foobar has been replaced", rs);
  free(buffer);
}

TEST(BufferTest, Vector) {
  char *buffer = (char *)malloc(4096);
  PayloadBuffer *pb = new (buffer) PayloadBuffer(4096);

  // Allocate space for a message containing the VectorHeader.
  PayloadBuffer::AllocateMainMessage(&pb, sizeof(VectorHeader));

  VectorHeader *hdr = pb->ToAddress<VectorHeader>(pb->message);
  std::cout << "Vector header: " << hdr << std::endl;
  pb->Dump(std::cout);
  toolbelt::Hexdump(pb, pb->hwm);

  PayloadBuffer::VectorPush<uint32_t>(&pb, hdr, 0x12345678);
  pb->Dump(std::cout);
  toolbelt::Hexdump(pb, pb->hwm);

  uint32_t v = pb->VectorGet<uint32_t>(hdr, 0);
  ASSERT_EQ(0x12345678, v);

  free(buffer);
}

TEST(BufferTest, VectorExpand) {
  char *buffer = (char *)malloc(4096);
  PayloadBuffer *pb = new (buffer) PayloadBuffer(4096);

  // Allocate space for a message containing the VectorHeader.
  PayloadBuffer::AllocateMainMessage(&pb, sizeof(VectorHeader));

  VectorHeader *hdr = pb->ToAddress<VectorHeader>(pb->message);
  std::cout << "Vector header: " << hdr << std::endl;
  toolbelt::Hexdump(pb, pb->hwm);

  pb->Dump(std::cout);

  for (int i = 0; i < 3; i++) {
    PayloadBuffer::VectorPush<uint32_t>(&pb, hdr, i + 1);
  }
  toolbelt::Hexdump(pb, pb->hwm);

  pb->Dump(std::cout);
  for (int i = 0; i < 3; i++) {
    uint32_t v = pb->VectorGet<uint32_t>(hdr, i);
    ASSERT_EQ(i + 1, v);
  }

  free(buffer);
}

TEST(BufferTest, VectorExpandMore) {
  char *buffer = (char *)malloc(4096);
  PayloadBuffer *pb = new (buffer) PayloadBuffer(4096);

  // Allocate space for a message containing the VectorHeader.
  PayloadBuffer::AllocateMainMessage(&pb, sizeof(VectorHeader));

  VectorHeader *hdr = pb->ToAddress<VectorHeader>(pb->message);
  std::cout << "Vector header: " << hdr << std::endl;
  toolbelt::Hexdump(pb, pb->hwm);

  pb->Dump(std::cout);

  for (int i = 0; i < 100; i++) {
    PayloadBuffer::VectorPush<uint32_t>(&pb, hdr, i + 1);
    uint32_t v = pb->VectorGet<uint32_t>(hdr, i);
    ASSERT_EQ(i + 1, v);
  }
  toolbelt::Hexdump(pb, pb->hwm);

 for (int i = 0; i < 100; i++) {
    uint32_t v = pb->VectorGet<uint32_t>(hdr, i);
    ASSERT_EQ(i + 1, v);
  }
  pb->Dump(std::cout);

  free(buffer);
}
int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}
