// File was generated by Davros (https://github.com/dallison/davros)
// It's probably best not to modify it, but I can't stop you
#include "davros/serdes/runtime.h"

// Message field definitions.
#include "davros/descriptor/Field.h"

namespace descriptor {
struct Descriptor {

  std::string package = {};
  std::string name = {};
  std::vector<std::string> imports = {};
  std::vector<descriptor::Field> fields = {};

  static const char* Name() { return "Descriptor"; }
  static const char* FullName() { return "descriptor/Descriptor"; }
  absl::Status SerializeToArray(char* addr, size_t len) const;
  absl::Status SerializeToBuffer(davros::serdes::Buffer& buffer) const;
  absl::Status DeserializeFromArray(const char* addr, size_t len);
  absl::Status DeserializeFromBuffer(davros::serdes::Buffer& buffer);
  size_t SerializedLength() const;
  bool operator==(const Descriptor& m) const;
  bool operator!=(const Descriptor& m) const {
    return !this->operator==(m);
  }
  static constexpr unsigned char descriptor[] = {
0x0a,0x00,0x00,0x00,0x64,0x65,0x73,0x63,0x72,0x69,0x70,0x74,0x6f,0x72,0x0a,0x00,0x00,
0x00,0x44,0x65,0x73,0x63,0x72,0x69,0x70,0x74,0x6f,0x72,0x04,0x00,0x00,0x00,0x00,0x00,
0x07,0x00,0x00,0x00,0x70,0x61,0x63,0x6b,0x61,0x67,0x65,0x0b,0xfe,0xff,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x04,0x00,0x00,0x00,0x6e,0x61,0x6d,0x65,0x0b,0xfe,
0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x00,0x07,0x00,0x00,0x00,0x69,0x6d,
0x70,0x6f,0x72,0x74,0x73,0x0b,0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,
0x00,0x06,0x00,0x00,0x00,0x66,0x69,0x65,0x6c,0x64,0x73,0x0e,0xff,0xff,0x0a,0x00,0x00,
0x00,0x64,0x65,0x73,0x63,0x72,0x69,0x70,0x74,0x6f,0x72,0x06,0x00,0x00,0x00,0x66,0x69,
0x65,0x6c,0x64,0x73
  };
};
}    // namespace descriptor
