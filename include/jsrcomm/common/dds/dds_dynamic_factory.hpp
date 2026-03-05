#pragma once

// JSON
#include "serialization/json.hpp"
// DDS
#include "dds_entity.hpp"

namespace jsr::common::dds {

constexpr auto DDS_SEQUENCE_BOUND_SIZE = 100u;

using TypeRef = DdsDynamicType::_ref_type;
using TypeBuilderRef = DdsDynamicTypeBuilder::_ref_type;
using TypeDescriptorRef = DdsTypeDescriptor::_ref_type;
using MemberDescriptorRef = DdsMemberDescriptor::_ref_type;
using TypeMemberRef = DdsDynamicTypeMember::_ref_type;
using DataRef = DdsDynamicData::_ref_type;

class DdsDynamicFactory {
   public:
    static TypeBuilderRef createStructType(const std::string& name);
    static TypeBuilderRef createStructType(const std::string& name, TypeRef base_type);

    static void addPrimitiveMember(const TypeBuilderRef& builder, const std::string& name, fdds::dds::TypeKind tk,
                                   uint32_t id = 0);

    static void addSequenceMember(const TypeBuilderRef& builder, const std::string& name, fdds::dds::TypeKind elem_kind,
                                  uint32_t bound = DDS_SEQUENCE_BOUND_SIZE, uint32_t id = 0);

    static void addSequenceMember(const TypeBuilderRef& builder, const std::string& name, TypeRef elem_type,
                                  uint32_t bound = DDS_SEQUENCE_BOUND_SIZE, uint32_t id = 0);

    static void addArrayMember(const TypeBuilderRef& builder, const std::string& name, fdds::dds::TypeKind elem_kind,
                               const std::vector<uint32_t>& dims, uint32_t id = 0);

    static void addArrayMember(const TypeBuilderRef& builder, const std::string& name, TypeRef elem_type,
                               const std::vector<uint32_t>& dims, uint32_t id = 0);

    static void addCustomMember(const TypeBuilderRef& builder, const std::string& name, TypeRef type, uint32_t id = 0);

    static DataRef createData(TypeRef type);

    static TypeBuilderRef parseTypeFromIdl(const std::string& idl_path, const std::string& type_name,
                                            const std::string& include_dir = "");

    static std::string toRos2TypeName(const std::string& dds_name);

    static TypeBuilderRef cloneTypeWithRos2(const TypeRef& type);

    static TypeBuilderRef parseTypeFromIdlWithRos2(const std::string& idl_path, const std::string& type_name,
                                                    const std::string& include_dir = "");

    static void printTypeInfo(const TypeRef& type, const std::string& mem_name = "", size_t indent = 0,
                              uint32_t id = UINT32_MAX);

    static std::string idlSerialize(const TypeRef& type);

    static DataRef parseFromJson(const nlohmann::json& data_json, const TypeRef& type);

    static nlohmann::json toJson(const DataRef& data);
};

}  // namespace jsr::common::dds