// File was generated by Neutron (https://github.com/dallison/neutron)
// It's probably best not to modify it, but I can't stop you
#pragma once
#include "neutron/serdes/runtime.h"


namespace descriptor {
struct Field {
  static constexpr int16_t FIELD_PRIMITIVE = -2;
  static constexpr int16_t FIELD_VECTOR = -1;
  static constexpr uint8_t TYPE_BOOL = 13;
  static constexpr uint8_t TYPE_DURATION = 13;
  static constexpr uint8_t TYPE_FLOAT32 = 9;
  static constexpr uint8_t TYPE_FLOAT64 = 10;
  static constexpr uint8_t TYPE_INT16 = 3;
  static constexpr uint8_t TYPE_INT32 = 5;
  static constexpr uint8_t TYPE_INT64 = 7;
  static constexpr uint8_t TYPE_INT8 = 1;
  static constexpr uint8_t TYPE_MESSAGE = 14;
  static constexpr uint8_t TYPE_STRING = 11;
  static constexpr uint8_t TYPE_TIME = 12;
  static constexpr uint8_t TYPE_UINT16 = 4;
  static constexpr uint8_t TYPE_UINT32 = 6;
  static constexpr uint8_t TYPE_UINT64 = 8;
  static constexpr uint8_t TYPE_UINT8 = 2;

  int16_t index = {};
  std::string name = {};
  uint8_t type = {};
  int16_t array_size = {};
  std::string msg_package = {};
  std::string msg_name = {};

  static const char* Name() { return "Field"; }
  static const char* FullName() { return "descriptor/Field"; }
  absl::Status SerializeToArray(char* addr, size_t len, bool compact=false) const;
  absl::Status SerializeToBuffer(neutron::serdes::Buffer& buffer, bool compact = false) const;
  absl::Status DeserializeFromArray(const char* addr, size_t len, bool compact = false);
  absl::Status DeserializeFromBuffer(neutron::serdes::Buffer& buffer, bool compact = false);
  size_t SerializedSize() const;
  size_t CompactSerializedSize() const;
  void CompactSerializedSize(neutron::serdes::SizeAccumulator& acc) const;
  absl::Status WriteToBuffer(neutron::serdes::Buffer& buffer) const;
  absl::Status WriteCompactToBuffer(neutron::serdes::Buffer& buffer, bool internal = false) const;
  absl::Status ReadFromBuffer(neutron::serdes::Buffer& buffer);
  absl::Status ReadCompactFromBuffer(neutron::serdes::Buffer& buffer);
  static absl::Status Expand(const neutron::serdes::Buffer& src, neutron::serdes::Buffer& dest);
  static absl::Status Compact(const neutron::serdes::Buffer& src, neutron::serdes::Buffer& dest, bool internal = false);
  bool operator==(const Field& m) const;
  bool operator!=(const Field& m) const {
    return !this->operator==(m);
  }
  std::string DebugString() const;
  static constexpr unsigned char _descriptor[] = {
0x0a,0x64,0x65,0x73,0x63,0x72,0x69,0x70,0x74,0x6f,0x72,0x05,0x46,0x69,0x65,0x6c,0x64,
0x00,0x06,0x00,0x05,0x69,0x6e,0x64,0x65,0x78,0x03,0x7e,0xfa,0x00,0x01,0x04,0x6e,0x61,
0x6d,0x65,0x0b,0x7e,0xfa,0x00,0x02,0x04,0x74,0x79,0x70,0x65,0x02,0x7e,0xfa,0x00,0x03,
0x0a,0x61,0x72,0x72,0x61,0x79,0x5f,0x73,0x69,0x7a,0x65,0x03,0x7e,0xfa,0x00,0x04,0x0b,
0x6d,0x73,0x67,0x5f,0x70,0x61,0x63,0x6b,0x61,0x67,0x65,0x0b,0x7e,0xfa,0x00,0x05,0x08,
0x6d,0x73,0x67,0x5f,0x6e,0x61,0x6d,0x65,0x0b,0x7e
  };
  static absl::Span<const char> GetDescriptor() {
    return absl::Span<const char>(reinterpret_cast<const char*>(_descriptor), sizeof(_descriptor));
  }
};
inline std::ostream& operator<<(std::ostream& os, const Field& msg) {
  os << "index: ";
  os << msg.index << std::endl;
  os << "name: ";
  os << msg.name << std::endl;
  os << "type: ";
  os << static_cast<int>(msg.type) << std::endl;
  os << "array_size: ";
  os << msg.array_size << std::endl;
  os << "msg_package: ";
  os << msg.msg_package << std::endl;
  os << "msg_name: ";
  os << msg.msg_name << std::endl;
  return os;
}
}    // namespace descriptor
