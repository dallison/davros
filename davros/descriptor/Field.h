// File was generated by Davros (https://github.com/dallison/davros)
// It's probably best not to modify it, but I can't stop you
#include "davros/serdes/runtime.h"

// Message field definitions.

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
  absl::Status SerializeToArray(char* addr, size_t len) const;
  absl::Status SerializeToBuffer(davros::Buffer& buffer) const;
  absl::Status DeserializeFromArray(const char* addr, size_t len);
  absl::Status DeserializeFromBuffer(davros::Buffer& buffer);
  size_t SerializedLength() const;
  bool operator==(const Field& m) const;
  bool operator!=(const Field& m) const {
    return !this->operator==(m);
  }
};
}    // namespace descriptor