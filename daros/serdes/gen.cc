
#include "daros/serdes/gen.h"
#include "absl/strings/str_format.h"
#include <fstream>

namespace daros::serdes {

absl::Status Generator::Generate(const Message &msg) {
  std::filesystem::path dir =
      root_ / std::filesystem::path(msg.Package()->Name());
  if (!std::filesystem::exists(dir) &&
      !std::filesystem::create_directories(dir)) {
    return absl::InternalError(
        absl::StrFormat("Unable to create directory %s", dir.string()));
  }

  std::filesystem::path header = dir / std::filesystem::path(msg.Name() + ".h");
  std::filesystem::path source =
      dir / std::filesystem::path(msg.Name() + ".cc");

  {
    std::ofstream out(header.string());
    if (!out) {
      return absl::InternalError(
          absl::StrFormat("Unable to create %s", header.string()));
    }
    if (absl::Status status = GenerateHeader(msg, out); !status.ok()) {
      return status;
    }
  }
  {
    std::ofstream out(source.string());
    if (!out) {
      return absl::InternalError(
          absl::StrFormat("Unable to create %s", source.string()));
    }
    if (absl::Status status = GenerateSource(msg, out); !status.ok()) {
      return status;
    }
  }
  return absl::OkStatus();
}

static std::string FieldCType(FieldType type) {
  switch (type) {
  case FieldType::kInt8:
    return "int8_t";
  case FieldType::kUint8:
    return "uint8_t";
  case FieldType::kInt16:
    return "int16_t";
  case FieldType::kUint16:
    return "uint16_t";
  case FieldType::kInt32:
    return "int32_t";
  case FieldType::kUint32:
    return "uint32_t";
  case FieldType::kInt64:
    return "int64_t";
  case FieldType::kUint64:
    return "uint64_t";
  case FieldType::kFloat32:
    return "float";
  case FieldType::kFloat64:
    return "double";
  case FieldType::kTime:
    return "daros::Time";
  case FieldType::kDuration:
    return "daros::Duration";
  case FieldType::kString:
    return "std::string";
  case FieldType::kBool:
    return "uint8_t";
  case FieldType::kMessage:
    [[fallthrough]];
  case FieldType::kUnknown:
    abort();
  }
}

static std::string SanitizeFieldName(const std::string &name) { return name; }

absl::Status Generator::GenerateHeader(const Message &msg, std::ostream &os) {
  os << "#include \"" << (runtime_path_.empty() ? "" : (runtime_path_ + "/"))
     << "daros/serdes/runtime.h\"\n";
  os << "\n";
  // Include files for message fields
  os << "// Imported headers.\n";
  for (auto &field : msg.Fields()) {
    if (field->Type() == FieldType::kMessage) {
      auto msg_field = std::static_pointer_cast<MessageField>(field);
      os << "#include \"" << (msg_path_.empty() ? "" : (msg_path_ + "/"))
         << msg_field->MsgPackage() << "/" << msg_field->MsgName() << ".h\"\n";
    }
  }
  os << "\n";
  os << "namespace " << msg.Package()->Name() << " {\n";
  os << "struct " << msg.Name() << " {\n";
  for (auto &field : msg.Fields()) {
    if (field->Type() == FieldType::kMessage) {
      auto msg_field = std::static_pointer_cast<MessageField>(field);
      os << "  " << msg_field->MsgPackage() << "::" << msg_field->MsgName();
    } else if (field->IsArray()) {
      auto array = std::static_pointer_cast<ArrayField>(field);
      const char *container =
          array->IsFixedSize() ? "std::array" : "std::vector";
      os << "  " << container << "<";
      if (array->Base()->Type() == FieldType::kMessage) {
        auto msg_field = std::static_pointer_cast<MessageField>(array->Base());
        os << msg_field->MsgPackage() << "::" << msg_field->MsgName();
      } else {
        os << FieldCType(array->Base()->Type());
      }
      if (array->IsFixedSize()) {
        os << ", " << array->Size();
      }
      os << ">";
    } else {
      os << "  " << FieldCType(field->Type());
    }
    os << " " << SanitizeFieldName(field->Name()) << ";\n";
  }
  os << "\n";
  os << "  absl::Status SerializeToArray(char* addr, size_t len) const;\n";
  os << "  absl::Status SerializeToBuffer(daros::Buffer& buffer) const;\n";
  os << "  absl::Status DeserializeFromArray(const char* addr, size_t len);\n";
  os << "  absl::Status DeserializeFromBuffer(daros::Buffer& buffer);\n";
  os << "  size_t SerializedLength() const;\n";
  os << "};\n";
  os << "}    // namespace " << msg.Package()->Name() << "\n";

  return absl::OkStatus();
}

absl::Status Generator::GenerateSource(const Message &msg, std::ostream &os) {
  os << "#include \"" << (msg_path_.empty() ? "" : (msg_path_ + "/"))
     << msg.Package()->Name() << "/" << msg.Name() << ".h\"\n";

  os << "namespace " << msg.Package()->Name() << " {\n";
  os << "absl::Status " << msg.Name()
     << "::SerializeToArray(char* addr, size_t len) const {\n";
  os << "  daros::Buffer buffer(addr, len);\n";
  os << "  return SerializeToBuffer(buffer);\n";
  os << "}\n\n";
  os << "absl::Status " << msg.Name()
     << "::DeserializeFromArray(const char* addr, size_t len) {\n";
  os << "  daros::Buffer buffer(addr, len);\n";
  os << "  return DeserializeFromBuffer(buffer);\n";
  os << "}\n\n";

  os << "absl::Status " << msg.Name()
     << "::SerializeToBuffer(daros::Buffer& buffer) const {\n";
  for (auto &field : msg.Fields()) {
    if (field->Type() == FieldType::kMessage) {
      auto msg_field = std::static_pointer_cast<MessageField>(field);
      os << "  if (absl::Status status = this->"
         << SanitizeFieldName(field->Name()) << ".SerializeToBuffer(buffer)"
         << "; !status.ok()) return status;\n";
    } else if (field->IsArray()) {
      auto array = std::static_pointer_cast<ArrayField>(field);
      os << "  if (absl::Status status = buffer.Write(this->"
         << SanitizeFieldName(field->Name())
         << "); !status.ok()) return status;\n";

    } else {
      os << "  if (absl::Status status = buffer.Write(this->"
         << SanitizeFieldName(field->Name())
         << "); !status.ok()) return status;\n";
    }
  }
  os << "  return absl::OkStatus();\n";
  os << "}\n\n";

  os << "absl::Status " << msg.Name()
     << "::DeserializeFromBuffer(daros::Buffer& buffer) {\n";
  for (auto &field : msg.Fields()) {
    if (field->Type() == FieldType::kMessage) {
      auto msg_field = std::static_pointer_cast<MessageField>(field);
      os << "  if (absl::Status status = this->"
         << SanitizeFieldName(field->Name())
         << ".DeserializeFromBuffer(buffer); !status.ok()) return status;\n";
    } else if (field->IsArray()) {
      auto array = std::static_pointer_cast<ArrayField>(field);
      os << "  if (absl::Status status = buffer.Read(this->"
         << SanitizeFieldName(field->Name())
         << "); !status.ok()) return status;\n";

    } else {
      os << "  if (absl::Status status = buffer.Read(this->"
         << SanitizeFieldName(field->Name())
         << "); !status.ok()) return status;\n";
    }
  }
  os << "  return absl::OkStatus();\n";
  os << "}\n\n";

  os << "size_t " << msg.Name() << "::SerializedLength() const {\n";
  os << "  size_t length = 0;\n";
  for (auto &field : msg.Fields()) {
    if (field->Type() == FieldType::kMessage) {
      auto msg_field = std::static_pointer_cast<MessageField>(field);
      os << "  length += this->" << SanitizeFieldName(field->Name())
         << ".SerializedLength();\n";
    } else if (field->IsArray()) {
      auto array = std::static_pointer_cast<ArrayField>(field);
      if (array->IsFixedSize()) {
        os << "  length += this->" << SanitizeFieldName(field->Name())
           << ".size() * sizeof(" << FieldCType(array->Base()->Type())
           << ");\n";
      } else {
        os << "  length += 4 + this->" << SanitizeFieldName(field->Name())
           << ".size() * sizeof(" << FieldCType(array->Base()->Type())
           << ");\n";
      }
    } else {
      if (field->Type() == FieldType::kString) {
        os << "  length += 4 + this->" << SanitizeFieldName(field->Name())
           << ".size();\n";
      } else {
        os << "  length += sizeof(this->" << SanitizeFieldName(field->Name())
           << ");\n";
      }
    }
  }
  os << "  return length;\n";
  os << "}\n";

  os << "}    // namespace " << msg.Package()->Name() << "\n";

  return absl::OkStatus();
} // namespace daros::serdes

} // namespace daros::serdes
