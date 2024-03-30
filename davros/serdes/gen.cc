
#include "davros/serdes/gen.h"
#include "absl/container/flat_hash_set.h"
#include "absl/strings/str_format.h"
#include "davros/descriptor.h"
#include <fstream>

namespace davros::serdes {

std::string Generator::Namespace(bool prefix_colon_colon) {
  std::string ns;
  if (namespace_.empty()) {
    return ns;
  }
  if (prefix_colon_colon) {
    ns = "::";
  }
  ns += namespace_;
  if (!prefix_colon_colon) {
    ns += "::";
  }
  return ns;
}

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

  std::ofstream out(header.string());
  if (!out) {
    return absl::InternalError(
        absl::StrFormat("Unable to create %s", header.string()));
  }
  if (absl::Status status = GenerateHeader(msg, out); !status.ok()) {
    return status;
  }
  std::cout << "Generated header file " << header.string() << std::endl;

  {
    std::ofstream out(source.string());
    if (!out) {
      return absl::InternalError(
          absl::StrFormat("Unable to create %s", source.string()));
    }
    if (absl::Status status = GenerateSource(msg, out); !status.ok()) {
      return status;
    }
    std::cout << "Generated source file " << source.string() << std::endl;
  }

  return absl::OkStatus();
}

static int EnumCSize(const Message &msg) {
  // Look for the biggest constant type.

  int size = 0;
  for (auto & [ name, c ] : msg.Constants()) {
    switch (c->Type()) {
    case FieldType::kInt8:
    case FieldType::kUint8:
      size = std::max(size, 1);
      break;
    case FieldType::kInt16:
    case FieldType::kUint16:
      size = std::max(size, 2);
      break;
    case FieldType::kInt32:
    case FieldType::kUint32:
      size = std::max(size, 4);
      break;
    case FieldType::kInt64:
    case FieldType::kUint64:
      size = std::max(size, 8);
      break;
    default:
      break;
    }
  }
  return size;
}

static std::string EnumCType(const Message &msg) {
  int size = EnumCSize(msg);

  switch (size) {
  case 0:
  default:
    return "uint8_t";
  case 1:
    return "uint8_t";
  case 2:
    return "uint16_t";
  case 4:
    return "uint32_t";
  case 8:
    return "uint64_t";
  }
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
    return "davros::Time";
  case FieldType::kDuration:
    return "davros::Duration";
  case FieldType::kString:
    return "std::string";
  case FieldType::kBool:
    return "uint8_t";
  case FieldType::kMessage:
    std::cerr << "Can't use message field type here\n";
    return "<message>";
  case FieldType::kUnknown:
    std::cerr << "Unknown field type " << int(type) << std::endl;
    abort();
  }
}

static std::string SanitizeFieldName(const std::string &name) { return name; }

std::string
Generator::MessageFieldTypeName(const Message &msg,
                                std::shared_ptr<MessageField> field) {
  std::string name;
  if (field->MsgPackage().empty()) {
    return msg.Package()->Name() + "::" + Namespace(false) + field->MsgName();
  }
  return field->MsgPackage() + "::" + Namespace(false) + field->MsgName();
}

static std::string
MessageFieldIncludeFile(const Message &msg,
                        std::shared_ptr<MessageField> field) {
  if (field->MsgPackage().empty()) {
    return "serdes/" + msg.Package()->Name() + "/" + field->MsgName() + ".h";
  }
  return "serdes/" + field->MsgPackage() + "/" + field->MsgName() + ".h";
}

std::shared_ptr<Field> Generator::ResolveField(std::shared_ptr<Field> field) {
  if (field->IsArray()) {
    auto array = std::static_pointer_cast<ArrayField>(field);
    return array->Base();
  }
  return field;
}

// Magic helper templates for std::visit.
// See https://en.cppreference.com/w/cpp/utility/variant/visit
template <class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template <class... Ts> overloaded(Ts...)->overloaded<Ts...>;

absl::Status Generator::GenerateHeader(const Message &msg, std::ostream &os) {
  os << "// File was generated by Davros "
        "(https://github.com/dallison/davros)\n";
  os << "// It's probably best not to modify it, but I can't stop you\n";
  os << "#pragma once\n";
  os << "#include \"" << (runtime_path_.empty() ? "" : (runtime_path_ + "/"))
     << "davros/serdes/runtime.h\"\n";
  os << "\n";
  // Include files for message fields
  absl::flat_hash_set<std::string> hdrs;
  for (auto field : msg.Fields()) {
    field = ResolveField(field);
    if (field->Type() == FieldType::kMessage) {
      auto msg_field = std::static_pointer_cast<MessageField>(field);
      std::string hdr = MessageFieldIncludeFile(msg, msg_field);
      if (hdrs.contains(hdr)) {
        continue;
      }
      hdrs.insert(hdr);
      os << "#include \"" << (msg_path_.empty() ? "" : (msg_path_ + "/")) << hdr
         << "\"\n";
    }
  }
  os << "\n";
  os << "namespace " << msg.Package()->Name() << Namespace(true) << " {\n";

  if (msg.IsEnum()) {
    // Enumeration.
    if (absl::Status status = GenerateEnum(msg, os); !status.ok()) {
      return status;
    }
  } else {
    if (absl::Status status = GenerateStruct(msg, os); !status.ok()) {
      return status;
    }
  }

  os << "}    // namespace " << msg.Package()->Name() << Namespace(true) << "\n";

  return absl::OkStatus();
}

absl::Status Generator::GenerateEnum(const Message &msg, std::ostream &os) {
  os << "enum class " << msg.Name() << " : " << EnumCType(msg) << " {\n";
  for (auto & [ name, c ] : msg.Constants()) {
    os << c->Name() << " = " << std::get<0>(c->Value()) << ",\n";
  }
  os << "};\n";
  return absl::OkStatus();
}

absl::Status Generator::GenerateStruct(const Message &msg, std::ostream &os) {
  os << "struct " << msg.Name() << " {\n";

  // Constants.
  for (auto & [ name, c ] : msg.Constants()) {
    if (c->Type() == FieldType::kString) {
      os << "  static inline constexpr const char " << c->Name() << "[] = ";
    } else {
      os << "  static constexpr " << FieldCType(c->Type()) << " " << c->Name()
         << " = ";
    }

    std::visit(overloaded{[&os](int64_t v) { os << v; },
                          [&os](double v) { os << v; },
                          [&os](std::string v) { os << '"' << v << '"'; }},
               c->Value());
    os << ";" << std::endl;
  }

  os << std::endl;

  for (auto &field : msg.Fields()) {
    if (field->Type() == FieldType::kMessage) {
      auto msg_field = std::static_pointer_cast<MessageField>(field);
      os << "  " << MessageFieldTypeName(msg, msg_field);
    } else if (field->IsArray()) {
      auto array = std::static_pointer_cast<ArrayField>(field);
      const char *container =
          array->IsFixedSize() ? "std::array" : "std::vector";
      os << "  " << container << "<";
      if (array->Base()->Type() == FieldType::kMessage) {
        auto msg_field = std::static_pointer_cast<MessageField>(array->Base());
        os << MessageFieldTypeName(msg, msg_field);
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
    os << " " << SanitizeFieldName(field->Name()) << " = {};\n";
  }
  os << "\n";
  os << "  static const char* Name() { return \"" << msg.Name() << "\"; }\n";
  os << "  static const char* FullName() { return \"" << msg.Package()->Name()
     << "/" << msg.Name() << "\"; }\n";
  os << "  absl::Status SerializeToArray(char* addr, size_t len) const;\n";
  os << "  absl::Status SerializeToBuffer(davros::serdes::Buffer& buffer) "
        "const;\n";
  os << "  absl::Status DeserializeFromArray(const char* addr, size_t "
        "len);\n";
  os << "  absl::Status DeserializeFromBuffer(davros::serdes::Buffer& "
        "buffer);\n";
  os << "  size_t SerializedLength() const;\n";
  os << "  bool operator==(const " << msg.Name() << "& m) const;\n";
  os << "  bool operator!=(const " << msg.Name() << "& m) const {\n";
  os << "    return !this->operator==(m);\n";
  os << "  }\n";
  os << "  std::string DebugString() const;\n";
  os << "  static constexpr unsigned char _descriptor[] = {\n";
  absl::StatusOr<descriptor::Descriptor> desc = MakeDescriptor(msg);
  if (!desc.ok()) {
    return desc.status();
  }
  if (absl::Status status = EncodeDescriptorAsHex(*desc, 80, true, os);
      !status.ok()) {
    return status;
  }
  os << "  };\n";
  os << "};\n";

  os << "inline std::ostream& operator<<(std::ostream& os, const " << msg.Name()
     << "& msg) {\n";
  for (auto &field : msg.Fields()) {
    os << "  os << \"" << field->Name() << ": \";\n";
    if (field->Type() == FieldType::kMessage) {
      auto msg_field = std::static_pointer_cast<MessageField>(field);
      if (msg_field->Msg()->IsEnum()) {
        os << "  os << int64_t(msg." << SanitizeFieldName(field->Name())
           << ") << std::endl;\n";
      } else {
        os << "  os << msg." << SanitizeFieldName(field->Name())
           << ".DebugString() << std::endl;\n";
      }
    } else if (field->IsArray()) {
      auto array = std::static_pointer_cast<ArrayField>(field);
      if (array->Base()->Type() == FieldType::kMessage) {
        auto msg_field = std::static_pointer_cast<MessageField>(array->Base());
        os << "  for (auto& m : msg." << SanitizeFieldName(field->Name())
           << ") {\n";
        if (msg_field->Msg()->IsEnum()) {
          os << "  os << " << EnumCType(*msg_field->Msg())
             << "(m) << std::endl;\n";
        } else {
          os << "    os << m.DebugString();\n";
        }
        os << "  }\n";
        os << "  os << std::endl;\n";
      } else {
        auto array = std::static_pointer_cast<ArrayField>(field);
        os << "  for (auto& m : msg." << SanitizeFieldName(array->Name())
           << ") {\n";
        os << "    os << m << std::endl;\n";
        os << "  }\n";
      }
    } else {
      os << "  os << msg." << SanitizeFieldName(field->Name())
         << " << std::endl;\n";
    }
  }
  os << "  return os;\n";
  os << "}\n";
  return absl::OkStatus();
}

absl::Status Generator::GenerateSource(const Message &msg, std::ostream &os) {
  os << "#include \"" << (msg_path_.empty() ? "" : (msg_path_ + "/"))
     << "serdes/" << msg.Package()->Name() << "/" << msg.Name() << ".h\"\n";

  if (msg.IsEnum()) {
    return absl::OkStatus();
  }
  os << "namespace " << msg.Package()->Name() << Namespace(true) <<" {\n";
  os << "absl::Status " << msg.Name()
     << "::SerializeToArray(char* addr, size_t len) const {\n";
  os << "  davros::serdes::Buffer buffer(addr, len);\n";
  os << "  return SerializeToBuffer(buffer);\n";
  os << "}\n\n";
  os << "absl::Status " << msg.Name()
     << "::DeserializeFromArray(const char* addr, size_t len) {\n";
  os << "  davros::serdes::Buffer buffer(const_cast<char*>(addr), len);\n";
  os << "  return DeserializeFromBuffer(buffer);\n";
  os << "}\n\n";

  if (absl::Status status = GenerateSerializer(msg, os); !status.ok()) {
    return status;
  }

  if (absl::Status status = GenerateDeserializer(msg, os); !status.ok()) {
    return status;
  }

  if (absl::Status status = GenerateLength(msg, os); !status.ok()) {
    return status;
  }

  os << "  bool " << msg.Name() << "::operator==(const " << msg.Name()
     << "& m) const {\n";
  for (auto &field : msg.Fields()) {
    os << "  if (this->" << SanitizeFieldName(field->Name()) << " != m."
       << SanitizeFieldName(field->Name()) << ") return false;\n";
  }
  os << "  return true;\n";
  os << "}\n\n";

  os << "std::string " << msg.Name() << "::DebugString() const {\n";
  os << "  std::stringstream s;\n";
  os << "  s << *this;\n";
  os << "  return s.str();\n";
  os << "}\n";
  os << "}    // namespace " << msg.Package()->Name() << Namespace(true) <<"\n";

  return absl::OkStatus();
}

absl::Status Generator::GenerateSerializer(const Message &msg,
                                           std::ostream &os) {
  os << "absl::Status " << msg.Name()
     << "::SerializeToBuffer(davros::serdes::Buffer& buffer) const {\n";
  for (auto &field : msg.Fields()) {
    if (field->Type() == FieldType::kMessage) {
      auto msg_field = std::static_pointer_cast<MessageField>(field);
      if (msg_field->Msg() == nullptr) {
        abort();
      }
      if (msg_field->Msg()->IsEnum()) {
        os << "  if (absl::Status status = buffer.Write("
           << EnumCType(*msg_field->Msg()) << "(this->"
           << SanitizeFieldName(field->Name()) << "))"
           << "; !status.ok()) return status;\n";
      } else {
        os << "  if (absl::Status status = this->"
           << SanitizeFieldName(field->Name()) << ".SerializeToBuffer(buffer)"
           << "; !status.ok()) return status;\n";
      }
    } else if (field->IsArray()) {
      auto array = std::static_pointer_cast<ArrayField>(field);
      if (array->Base()->Type() == FieldType::kMessage) {
        if (!array->IsFixedSize()) {
          os << "  if (absl::Status status = buffer.Write(uint32_t(this->"
             << SanitizeFieldName(field->Name())
             << ".size())); !status.ok()) return status;\n";
        }
        auto msg_field = std::static_pointer_cast<MessageField>(array->Base());
        os << "  for (auto& m : this->" << SanitizeFieldName(field->Name())
           << ") {\n";
        if (msg_field->Msg()->IsEnum()) {
          os << "  if (absl::Status status = buffer.Write("
             << EnumCType(*msg_field->Msg())
             << "(m)); "
                "!status.ok()) return status;\n";
        } else {
          os << "    if (absl::Status status = m.SerializeToBuffer(buffer)"
             << "; !status.ok()) return status;\n";
        }
        os << "  }\n";
      } else {
        auto array = std::static_pointer_cast<ArrayField>(field);
        os << "  if (absl::Status status = buffer.Write(this->"
           << SanitizeFieldName(field->Name())
           << "); !status.ok()) return status;\n";
      }
    } else {
      os << "  if (absl::Status status = buffer.Write(this->"
         << SanitizeFieldName(field->Name())
         << "); !status.ok()) return status;\n";
    }
  }
  os << "  return absl::OkStatus();\n";
  os << "}\n\n";
  return absl::OkStatus();
}

absl::Status Generator::GenerateDeserializer(const Message &msg,
                                             std::ostream &os) {
  os << "absl::Status " << msg.Name()
     << "::DeserializeFromBuffer(davros::serdes::Buffer& buffer) {\n";
  for (auto &field : msg.Fields()) {
    if (field->Type() == FieldType::kMessage) {
      auto msg_field = std::static_pointer_cast<MessageField>(field);
      if (msg_field->Msg()->IsEnum()) {
        os << "  {\n  " << EnumCType(*msg_field->Msg())
           << " v;\n  if (absl::Status status = buffer.Read(v);"
              "!status.ok()) return status;\n";
        os << "  this->" << SanitizeFieldName(field->Name())
           << " = static_cast<" << MessageFieldTypeName(msg, msg_field)
           << ">(v);\n";
        os << "  }\n";
      } else {
        os << "  if (absl::Status status = this->"
           << SanitizeFieldName(field->Name())
           << ".DeserializeFromBuffer(buffer); !status.ok()) return "
              "status;\n";
      }
    } else if (field->IsArray()) {
      auto array = std::static_pointer_cast<ArrayField>(field);
      if (array->Base()->Type() == FieldType::kMessage) {
        auto msg_field = std::static_pointer_cast<MessageField>(array->Base());
        os << "  {\n";
        if (array->IsFixedSize()) {
          os << "  int32_t size = " << array->Size() << ";\n";
        } else {
          os << "  int32_t size;\n";
          os << "  if (absl::Status status = buffer.Read(size); "
                "!status.ok()) "
                "return status;\n";
        }
        os << "  for (int32_t i = 0; i < size; i++) {\n";
        if (msg_field->Msg()->IsEnum()) {
          os << "    " << EnumCType(*msg_field->Msg()) << " v;\n";
          os << "    if (absl::Status status = buffer.Read(v); !status.ok()) "
                "return status;\n";
          os << "    this->" << SanitizeFieldName(field->Name())
             << " = static_cast<" << MessageFieldTypeName(msg, msg_field)
             << "(v);\n";
        } else {
          os << "    if (absl::Status status = this->"
             << SanitizeFieldName(field->Name())
             << "[i].DeserializeFromBuffer(buffer)"
             << "; !status.ok()) return status;\n";
          os << "  }\n";
        }
        os << "  }\n";
      } else {
        auto array = std::static_pointer_cast<ArrayField>(field);
        os << "  if (absl::Status status = buffer.Read(this->"
           << SanitizeFieldName(field->Name())
           << "); !status.ok()) return status;\n";
      }

    } else {
      os << "  if (absl::Status status = buffer.Read(this->"
         << SanitizeFieldName(field->Name())
         << "); !status.ok()) return status;\n";
    }
  }
  os << "  return absl::OkStatus();\n";
  os << "}\n\n";
  return absl::OkStatus();
}

absl::Status Generator::GenerateLength(const Message &msg, std::ostream &os) {
  os << "size_t " << msg.Name() << "::SerializedLength() const {\n";
  os << "  size_t length = 0;\n";
  for (auto &field : msg.Fields()) {
    if (field->Type() == FieldType::kMessage) {
      auto msg_field = std::static_pointer_cast<MessageField>(field);
      if (msg_field->Msg()->IsEnum()) {
        os << "  length += " << EnumCSize(*msg_field->Msg()) << ";\n";
      } else {
        os << "  length += this->" << SanitizeFieldName(field->Name())
           << ".SerializedLength();\n";
      }
    } else if (field->IsArray()) {
      auto array = std::static_pointer_cast<ArrayField>(field);
      if (array->Base()->Type() == FieldType::kMessage) {
        auto msg_field = std::static_pointer_cast<MessageField>(array->Base());
        if (msg_field->Msg()->IsEnum()) {
          os << "  length += this->" << SanitizeFieldName(field->Name())
             << ".size() * " << EnumCSize(*msg_field->Msg()) << ";\n";
        } else {
          os << "  length += " << (array->IsFixedSize() ? 0 : 4) << " + this->"
             << SanitizeFieldName(field->Name()) << ".size() * sizeof("
             << MessageFieldTypeName(msg, msg_field) << ");\n";
        }
      } else {
        os << "  length += " << (array->IsFixedSize() ? 0 : 4) << " + this->"
           << SanitizeFieldName(field->Name()) << ".size() * sizeof("
           << FieldCType(array->Base()->Type()) << ");\n";
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
  os << "}\n\n";
  return absl::OkStatus();
}

} // namespace davros::serdes
