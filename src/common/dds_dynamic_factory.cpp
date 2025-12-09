// STD
#include <filesystem>
#include <unordered_map>
// DDS
#include "common/dds/dds_dynamic_factory.hpp"

namespace jsr::common::dds {
TypeBuilderRef DdsDynamicFactory::CreateStructType(const std::string& name) {
    TypeDescriptorRef desc{fdds::dds::traits<DdsTypeDescriptor>::make_shared()};
    desc->kind(TK_STRUCTURE);
    desc->name(name);

    return DdsDynamicTypeBuilderFactory::get_instance()->create_type(desc);
}

TypeBuilderRef DdsDynamicFactory::CreateStructType(const std::string& name, TypeRef base_type) {
    TypeDescriptorRef desc{fdds::dds::traits<DdsTypeDescriptor>::make_shared()};
    desc->kind(TK_STRUCTURE);
    desc->name(name);
    desc->base_type(base_type);

    return DdsDynamicTypeBuilderFactory::get_instance()->create_type(desc);
}

void DdsDynamicFactory::AddPrimitiveMember(TypeBuilderRef builder, const std::string& name, fdds::dds::TypeKind tk,
                                           uint32_t id) {
    MemberDescriptorRef m{fdds::dds::traits<DdsMemberDescriptor>::make_shared()};
    m->name(name);
    if (id != 0) {
        m->id(id);
    }
    auto primitive_type = DdsDynamicTypeBuilderFactory::get_instance()->get_primitive_type(tk);
    m->type(primitive_type);
    builder->add_member(m);
}

void DdsDynamicFactory::AddSequenceMember(TypeBuilderRef builder, const std::string& name,
                                          fdds::dds::TypeKind elem_kind, uint32_t bound, uint32_t id) {
    auto elem_type = DdsDynamicTypeBuilderFactory::get_instance()->get_primitive_type(elem_kind);
    auto seq_type = DdsDynamicTypeBuilderFactory::get_instance()->create_sequence_type(elem_type, bound)->build();
    MemberDescriptorRef m{fdds::dds::traits<DdsMemberDescriptor>::make_shared()};
    m->name(name);
    if (id != 0) {
        m->id(id);
    }
    m->type(seq_type);
    builder->add_member(m);
}

void DdsDynamicFactory::AddSequenceMember(TypeBuilderRef builder, const std::string& name, TypeRef elem_type,
                                          uint32_t bound, uint32_t id) {
    auto seq_type = DdsDynamicTypeBuilderFactory::get_instance()->create_sequence_type(elem_type, bound)->build();
    MemberDescriptorRef m{fdds::dds::traits<DdsMemberDescriptor>::make_shared()};
    m->name(name);
    if (id != 0) {
        m->id(id);
    }
    m->type(seq_type);
    builder->add_member(m);
}

void DdsDynamicFactory::AddArrayMember(TypeBuilderRef builder, const std::string& name, fdds::dds::TypeKind elem_kind,
                                       const std::vector<uint32_t>& dims, uint32_t id) {
    auto elem_type = DdsDynamicTypeBuilderFactory::get_instance()->get_primitive_type(elem_kind);
    auto arr_type = DdsDynamicTypeBuilderFactory::get_instance()->create_array_type(elem_type, dims)->build();
    MemberDescriptorRef m{fdds::dds::traits<DdsMemberDescriptor>::make_shared()};
    m->name(name);
    if (id != 0) {
        m->id(id);
    }
    m->type(arr_type);
    builder->add_member(m);
}

void DdsDynamicFactory::AddArrayMember(TypeBuilderRef builder, const std::string& name, TypeRef elem_type,
                                       const std::vector<uint32_t>& dims, uint32_t id) {
    auto arr_type = DdsDynamicTypeBuilderFactory::get_instance()->create_array_type(elem_type, dims)->build();
    MemberDescriptorRef m{fdds::dds::traits<DdsMemberDescriptor>::make_shared()};
    m->name(name);
    if (id != 0) {
        m->id(id);
    }
    m->type(arr_type);
    builder->add_member(m);
}

void DdsDynamicFactory::AddCustomMember(TypeBuilderRef builder, const std::string& name, TypeRef type, uint32_t id) {
    MemberDescriptorRef m{fdds::dds::traits<DdsMemberDescriptor>::make_shared()};
    m->name(name);
    if (id != 0) {
        m->id(id);
    }
    m->type(type);

    builder->add_member(m);
}

DataRef DdsDynamicFactory::CreateData(TypeRef type) {
    return DdsDynamicDataFactory::get_instance()->create_data(type);
}

TypeBuilderRef DdsDynamicFactory::ParserTypeFromIdl(const std::string& idl_path, const std::string& type_name,
                                                    const std::string& include_dir) {
    namespace fs = std::filesystem;
    auto include_root_path = fs::path(include_dir);
    auto idl_file = fs::path(idl_path);
    if (!fs::exists(idl_file)) {
        throw std::runtime_error("IDL file not found: " + idl_path);
    }
    // Load the IDL file
    std::vector<std::string> include_paths;
    if (!include_dir.empty()) {
        include_paths.push_back(include_dir);
        for (const auto& entry : fs::recursive_directory_iterator(include_root_path)) {
            if (entry.is_regular_file()) {
                include_paths.push_back(entry.path().string());
            }
        }
    }
    // Retrieve the instance of the desired type
    TypeBuilderRef dyn_type_builder =
        DdsDynamicTypeBuilderFactory::get_instance()->create_type_w_uri(idl_path, type_name, include_paths);
    if (!dyn_type_builder) {
        throw std::runtime_error("Failed to parse IDL or create dynamic type: " + type_name);
    }

    return dyn_type_builder;
}

std::string DdsDynamicFactory::ToRos2TypeName(const std::string& dds_name) {
    const std::string ns_sep = "::";
    size_t pos = dds_name.rfind(ns_sep);
    if (pos == std::string::npos) {
        return dds_name;
    }
    std::string ns = dds_name.substr(0, pos);     // namespace
    std::string type = dds_name.substr(pos + 2);  // type name
    if (ns.size() >= 5 && ns.compare(ns.size() - 5, 5, "dds_") == 0)
        return dds_name;
    return ns + "::dds_::" + type + "_";
}

TypeBuilderRef DdsDynamicFactory::CloneTypeWithRos2(TypeRef type) {
    auto factory = DdsDynamicTypeBuilderFactory::get_instance();

    TypeDescriptorRef desc{fdds::dds::traits<DdsTypeDescriptor>::make_shared()};
    // shallow copy
    type->get_descriptor(desc);
    TypeBuilderRef new_type_builder;
    switch (desc->kind()) {
        case TK_ARRAY: {
            auto elem_type = desc->element_type();
            auto new_elem_type = CloneTypeWithRos2(elem_type)->build();
            desc->element_type(new_elem_type);
            new_type_builder = factory->create_type(desc);
        } break;
        case TK_SEQUENCE: {
            auto elem_type = desc->element_type();
            auto new_elem_type = CloneTypeWithRos2(elem_type)->build();
            desc->element_type(new_elem_type);
            new_type_builder = factory->create_type(desc);
        } break;
        case TK_MAP: {
            auto key_type = desc->key_element_type();
            auto value_type = desc->element_type();
            auto new_key_type = CloneTypeWithRos2(key_type)->build();
            auto new_value_type = CloneTypeWithRos2(value_type)->build();
            desc->key_element_type(new_key_type);
            desc->element_type(new_value_type);
            new_type_builder = factory->create_type(desc);
        } break;
        case TK_UNION: {
            auto discr_type = desc->discriminator_type();
            if (discr_type->get_kind() != TK_ENUM) {
                auto new_discr_type = CloneTypeWithRos2(discr_type)->build();
                desc->discriminator_type(new_discr_type);
            }
            // union don't need it
            // desc->name(ToRos2TypeName(desc->name().c_str()));
            new_type_builder = factory->create_type(desc);
            auto member_count = type->get_member_count();
            for (decltype(member_count) i = 1; i < member_count; i++) {
                TypeMemberRef mem;
                type->get_member(mem, i);
                MemberDescriptorRef mem_desc{fdds::dds::traits<DdsMemberDescriptor>::make_shared()};
                mem->get_descriptor(mem_desc);
                if (mem_desc->type()->get_kind() != TK_ENUM) {
                    auto mem_type = CloneTypeWithRos2(mem_desc->type())->build();
                    mem_desc->type(mem_type);
                }
                new_type_builder->add_member(mem_desc);
            }

        } break;
        case TK_STRUCTURE: {
            desc->name(ToRos2TypeName(desc->name().c_str()));
            new_type_builder = factory->create_type(desc);
            auto member_count = type->get_member_count();
            for (decltype(member_count) i = 0; i < member_count; i++) {
                TypeMemberRef mem;
                type->get_member(mem, i);
                MemberDescriptorRef mem_desc{fdds::dds::traits<DdsMemberDescriptor>::make_shared()};
                mem->get_descriptor(mem_desc);
                if (mem_desc->type()->get_kind() != TK_ENUM) {
                    auto mem_type = CloneTypeWithRos2(mem_desc->type())->build();
                    mem_desc->type(mem_type);
                }
                new_type_builder->add_member(mem_desc);
            }
        } break;
        default:
            new_type_builder = factory->create_type(desc);
            break;
    }

    return new_type_builder;
}

TypeBuilderRef DdsDynamicFactory::ParserTypeFromIdlWithRos2(const std::string& idl_path, const std::string& type_name,
                                                            const std::string& include_dir) {
    auto type_builder = ParserTypeFromIdl(idl_path, type_name, include_dir);
    return CloneTypeWithRos2(type_builder->build());
}

void DdsDynamicFactory::PrintTypeInfo(TypeRef type, const std::string& mem_name, size_t indent, int32_t id) {
    auto indent_print = [indent] {
        for (size_t i = 0; i < indent; ++i) {
            fmt::print("\t");
        }
    };
    indent_print();
    if (id != -1) {
        fmt::print("[{}]{}->{}[", id, mem_name.c_str(), type->get_name().c_str());
    } else {
        fmt::print("[*]{}->{}[", mem_name.c_str(), type->get_name().c_str());
    }

    TypeDescriptorRef desc{fdds::dds::traits<DdsTypeDescriptor>::make_shared()};
    type->get_descriptor(desc);

    static const std::unordered_map<fdds::dds::TypeKind, std::string> type_kind_map = {
        {TK_NONE, "NONE"},       {TK_BOOLEAN, "BOOLEAN"},   {TK_BYTE, "BYTE"},       {TK_INT16, "INT16"},
        {TK_INT32, "INT32"},     {TK_INT64, "INT64"},       {TK_UINT16, "UINT16"},   {TK_UINT32, "UINT32"},
        {TK_UINT64, "UINT64"},   {TK_FLOAT32, "FLOAT32"},   {TK_FLOAT64, "FLOAT64"}, {TK_FLOAT128, "FLOAT128"},
        {TK_INT8, "INT8"},       {TK_UINT8, "UINT8"},       {TK_CHAR8, "CHAR8"},     {TK_CHAR16, "CHAR16"},
        {TK_STRING8, "STRING8"}, {TK_STRING16, "STRING16"}, {TK_ALIAS, "ALIAS"},
    };

    auto kind = type->get_kind();
    switch (kind) {
        case TK_NONE:
        case TK_BOOLEAN:
        case TK_BYTE:
        case TK_INT8:
        case TK_INT16:
        case TK_INT32:
        case TK_INT64:
        case TK_UINT8:
        case TK_UINT16:
        case TK_UINT32:
        case TK_UINT64:
        case TK_FLOAT32:
        case TK_FLOAT64:
        case TK_FLOAT128:
        case TK_CHAR8:
        case TK_CHAR16:
        case TK_STRING8:
        case TK_STRING16:
        case TK_ALIAS:
            fmt::print("{}]\n", type_kind_map.at(kind));
            break;
        case TK_ENUM: {
            fmt::print("ENUM]");
            std::map<fdds::dds::ObjectName, TypeMemberRef> members;
            type->get_all_members_by_name(members);
            fmt::print("({})\n", members.size());
            size_t count = 0;
            for (auto& [name, mem] : members) {
                indent_print();
                MemberDescriptorRef mem_desc{fdds::dds::traits<DdsMemberDescriptor>::make_shared()};
                mem->get_descriptor(mem_desc);
                PrintTypeInfo(mem_desc->type(), mem->get_name().c_str(), indent + 1, count);
                ++count;
            }
        } break;
        case TK_BITMASK:
            fmt::print("BITMASK]\n");
            break;
        case TK_ANNOTATION:
            fmt::print("ANNOTATION]\n");
            break;
        case TK_STRUCTURE: {
            fmt::print("STRUCTURE]\n");
            indent_print();
            fmt::print(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
            auto mcount = type->get_member_count();
            for (decltype(mcount) id = 0; id < mcount; id++) {
                TypeMemberRef mem;
                type->get_member(mem, id);
                MemberDescriptorRef mem_desc{fdds::dds::traits<DdsMemberDescriptor>::make_shared()};
                mem->get_descriptor(mem_desc);
                PrintTypeInfo(mem_desc->type(), mem->get_name().c_str(), indent + 1, id);
            }
            indent_print();
            fmt::print("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n");

        } break;
        case TK_UNION: {
            fmt::print("UNION]\n");
            indent_print();
            fmt::print(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
            uint32_t mcount = type->get_member_count();
            for (uint32_t id = 0; id < mcount; id++) {
                TypeMemberRef mem;
                type->get_member(mem, id);
                MemberDescriptorRef mem_desc{fdds::dds::traits<DdsMemberDescriptor>::make_shared()};
                mem->get_descriptor(mem_desc);
                PrintTypeInfo(mem_desc->type(), mem->get_name().c_str(), indent + 1, id);
            }
            indent_print();
            fmt::print("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n");
        } break;
        case TK_BITSET:
            fmt::print("BITSET]\n");
            break;
        case TK_SEQUENCE: {
            fmt::print("SEQUENCE]\n");
            auto elem_type = desc->element_type();
            PrintTypeInfo(elem_type, "seq_data", indent + 1);
        } break;
        case TK_ARRAY: {
            fmt::print("ARRAY]\n");
            auto elem_type = desc->element_type();
            PrintTypeInfo(elem_type, "arr_data", indent + 1);
        } break;
        case TK_MAP: {
            fmt::print("MAP]\n");
            auto key_type = desc->key_element_type();
            auto elem_type = desc->element_type();
            PrintTypeInfo(key_type, "key", indent + 1);
            PrintTypeInfo(elem_type, "value", indent + 1);
        } break;
        default:
            fmt::print("UNKNOWN (0x{:X})]\n", static_cast<int>(kind));
            break;
    }
}

std::string DdsDynamicFactory::IdlSerialize(TypeRef type) {
    std::stringstream ss;
    fdds::dds::idl_serialize(type, ss);
    return ss.str();
}

DataRef DdsDynamicFactory::ParseFromJson(const nlohmann::json& data_json, TypeRef type) {
    DataRef data;
    fdds::dds::json_deserialize(data_json.dump(), type, fdds::dds::DynamicDataJsonFormat::EPROSIMA, data);
    return data;
}

nlohmann::json DdsDynamicFactory::ToJson(DataRef data) {
    std::stringstream json_ss;
    fdds::dds::json_serialize(data, fdds::dds::DynamicDataJsonFormat::EPROSIMA, json_ss);
    nlohmann::json data_json = nlohmann::json::parse(json_ss.str());
    return data_json;
}

}  // namespace jsr::common::dds