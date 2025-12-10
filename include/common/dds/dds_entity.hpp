/**
 * @file dds_entity.hpp
 * @author TanJiachun (jiachun.tan@joysonrobot.com)
 * @brief DDS entity declare for Joyson Robot Robotics SDK.
 * @version 0.1
 * @date 2025-07-07
 *
 * @copyright Copyright 2025 Joyson Robotics, Inc. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * See the LICENSE file or http://www.apache.org/licenses/LICENSE-2.0 for details.
 *
 * Modifications:
 * - Changed namespace to `jsr::common::dds`
 * - Use fmt replace std::cout
 * - Add some Alias
 */

#pragma once

// STD
#include <memory>
// FMT
#define FMT_HEADER_ONLY
#include "fmt/core.h"
// FASTDDS
#include "fastdds/dds/domain/DomainParticipant.hpp"
#include "fastdds/dds/domain/DomainParticipantFactory.hpp"
#include "fastdds/dds/publisher/DataWriter.hpp"
#include "fastdds/dds/publisher/Publisher.hpp"
#include "fastdds/dds/subscriber/DataReader.hpp"
#include "fastdds/dds/subscriber/DataReaderListener.hpp"
#include "fastdds/dds/subscriber/SampleInfo.hpp"
#include "fastdds/dds/subscriber/Subscriber.hpp"
#include "fastdds/dds/subscriber/qos/DataReaderQos.hpp"
#include "fastdds/dds/topic/Topic.hpp"
#include "fastdds/rtps/transport/UDPv4TransportDescriptor.hpp"
// dynamic type
#include "fastdds/dds/xtypes/dynamic_types/DynamicData.hpp"
#include "fastdds/dds/xtypes/dynamic_types/DynamicDataFactory.hpp"
#include "fastdds/dds/xtypes/dynamic_types/DynamicType.hpp"
#include "fastdds/dds/xtypes/dynamic_types/DynamicTypeBuilderFactory.hpp"
#include "fastdds/dds/xtypes/dynamic_types/DynamicTypeMember.hpp"
#include "fastdds/dds/xtypes/utils.hpp"

// DDS
#include "common/dds/dds_callback.hpp"

namespace jsr::common::dds {
namespace fdds = eprosima::fastdds;

// Entity
using DdsDomainParticipant = fdds::dds::DomainParticipant;
using DdsDomainParticipantFactory = fdds::dds::DomainParticipantFactory;
using DdsPublisher = fdds::dds::Publisher;
using DdsSubscriber = fdds::dds::Subscriber;
using DdsWriter = fdds::dds::DataWriter;
using DdsReader = fdds::dds::DataReader;
using DdsTopic = fdds::dds::Topic;
// Ptr
using DdsTopicPtr = std::shared_ptr<DdsTopic>;
using DdsTopicDataTypePtr = std::shared_ptr<fdds::dds::TopicDataType>;
using DdsParticipantPtr = std::shared_ptr<DdsDomainParticipant>;
using DdsPublisherPtr = std::shared_ptr<DdsPublisher>;
using DdsSubscriberPtr = std::shared_ptr<DdsSubscriber>;
using DdsWriterPtr = std::shared_ptr<DdsWriter>;
using DdsReaderPtr = std::shared_ptr<DdsReader>;
// Qos
using DdsDomainParticipantQos = fdds::dds::DomainParticipantQos;
using DdsTopicQos = fdds::dds::TopicQos;
using DdsPublisherQos = fdds::dds::PublisherQos;
using DdsSubscriberQos = fdds::dds::SubscriberQos;
using DataWriterQos = fdds::dds::DataWriterQos;
using DataReaderQos = fdds::dds::DataReaderQos;
using DdsUDPv4TransportDescriptor = fdds::rtps::UDPv4TransportDescriptor;
// CallbackPtr
using DdsReaderCallbackPtr = std::shared_ptr<DdsReaderCallback>;
// SampleInfo
using DdsSampleInfo = fdds::dds::SampleInfo;

// Type
using DdsTypeSupport = fdds::dds::TypeSupport;
// MemberId
constexpr auto DDS_MEMBER_ID_INVALID = fdds::dds::MEMBER_ID_INVALID;

using DdsTypeObject = fdds::dds::xtypes::TypeObject;
using DdsTypeDescriptor = fdds::dds::TypeDescriptor;
using DdsMemberDescriptor = fdds::dds::MemberDescriptor;
using DdsDynamicTypeMember = fdds::dds::DynamicTypeMember;
using DdsDynamicType = fdds::dds::DynamicType;
using DdsDynamicTypeBuilder = fdds::dds::DynamicTypeBuilder;
using DdsDynamicTypeBuilderFactory = fdds::dds::DynamicTypeBuilderFactory;
using DdsDynamicData = fdds::dds::DynamicData;
using DdsDynamicDataFactory = fdds::dds::DynamicDataFactory;
using DdsDynamicPubSubType = fdds::dds::DynamicPubSubType;

#define DDS_TOPIC_QOS_DEFAULT fdds::dds::TOPIC_QOS_DEFAULT
#define DDS_PARTICIPANT_QOS_DEFAULT fdds::dds::PARTICIPANT_QOS_DEFAULT
#define DDS_PUBLISHER_QOS_DEFAULT fdds::dds::PUBLISHER_QOS_DEFAULT
#define DDS_SUBSCRIBER_QOS_DEFAULT fdds::dds::SUBSCRIBER_QOS_DEFAULT
#define DDS_DATAWRITER_QOS_DEFAULT fdds::dds::DATAWRITER_QOS_DEFAULT
#define DDS_DATAREADER_QOS_DEFAULT fdds::dds::DATAREADER_QOS_DEFAULT
// Dds kind
constexpr auto DDS_BEST_EFFORT_RELIABILITY_QOS = fdds::dds::BEST_EFFORT_RELIABILITY_QOS;
constexpr auto DDS_RELIABLE_RELIABILITY_QOS = fdds::dds::RELIABLE_RELIABILITY_QOS;
using fdds::dds::TK_ALIAS;
using fdds::dds::TK_ANNOTATION;
using fdds::dds::TK_ARRAY;
using fdds::dds::TK_BITMASK;
using fdds::dds::TK_BITSET;
using fdds::dds::TK_BOOLEAN;
using fdds::dds::TK_BYTE;
using fdds::dds::TK_CHAR16;
using fdds::dds::TK_CHAR8;
using fdds::dds::TK_ENUM;
using fdds::dds::TK_FLOAT128;
using fdds::dds::TK_FLOAT32;
using fdds::dds::TK_FLOAT64;
using fdds::dds::TK_INT16;
using fdds::dds::TK_INT32;
using fdds::dds::TK_INT64;
using fdds::dds::TK_INT8;
using fdds::dds::TK_MAP;
using fdds::dds::TK_NONE;
using fdds::dds::TK_SEQUENCE;
using fdds::dds::TK_STRING16;
using fdds::dds::TK_STRING8;
using fdds::dds::TK_STRUCTURE;
using fdds::dds::TK_UINT16;
using fdds::dds::TK_UINT32;
using fdds::dds::TK_UINT64;
using fdds::dds::TK_UINT8;
using fdds::dds::TK_UNION;

template <typename T>
using not_dynamic_data_t = std::enable_if_t<!std::is_same_v<T, DdsDynamicData>>;
template <typename T>
using is_dynamic_data_t = std::enable_if_t<std::is_same_v<T, DdsDynamicData>>;
template <typename T>
inline constexpr bool IS_DYNAMIC_DATA_V = std::is_same_v<T, DdsDynamicData>;

/**
 * @brief DDS entity listener
 *
 * @tparam MSG message type
 */

template <typename MSG>
class DdsReaderListener : public fdds::dds::DataReaderListener {
   public:
    DdsReaderListener() = default;
    ~DdsReaderListener() override = default;

    /**
     * @brief Set the Callback object
     *
     * @param cb callback object
     */
    void SetCallback(const DdsReaderCallback& cb) {
        if (cb.HasMessageHandler()) {
            fmt::print(stderr, "Listener have the Callback\n");
            return;
        }
        cb_ = std::make_shared<DdsReaderCallback>(cb);
    }

    void SetDynamicTypeBuilder(DdsDynamicTypeBuilder::_ref_type type_builder) {
        type_builder_ = std::move(type_builder);
        return;
    }

    /**
     * @brief On data available, call the callback
     *
     * @param reader reader
     */
    void on_data_available(DdsReader* reader) override {
        if constexpr (IS_DYNAMIC_DATA_V<MSG>) {
            assert(type_builder_ != nullptr && "DdsReaderListener::on_data_available(): type builder is null");
            typename MSG::_ref_type st = DdsDynamicDataFactory::get_instance()->create_data(type_builder_->build());
            DdsSampleInfo info;
            if (reader->take_next_sample(&st, &info) == fdds::dds::RETCODE_OK) {
                if (info.valid_data) {
                    cb_->OnDataAvailable(&st);
                }
            }
        } else {
            MSG st;
            DdsSampleInfo info;
            if (reader->take_next_sample(&st, &info) == fdds::dds::RETCODE_OK) {
                if (info.valid_data) {
                    cb_->OnDataAvailable(&st);
                }
            }
        }
    }

   private:
    DdsReaderCallbackPtr cb_;
    // dynamic
    DdsDynamicTypeBuilder::_ref_type type_builder_;
};

template <typename MSG>
using DdsReaderListenerPtr = std::shared_ptr<DdsReaderListener<MSG>>;

}  // namespace jsr::common::dds