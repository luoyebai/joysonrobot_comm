/**
 * @file dds_factory_model.hpp
 * @author TanJiachun (jiachun.tan@joysonrobot.com)
 * @brief DDS factory model declare for Joyson Robot Robotics COMM.
 * @version 0.1
 * @date 2025-07-07
 *
 * @copyright Copyright 2025 Joyson Robotics, Inc. All Rights Reserved.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * See the LICENSE file or http://www.apache.org/licenses/LICENSE-2.0 for details.
 *
 * Modifications:
 * - Changed namespace from to `jsr::common::dds`
 * - Use fmt replace std::cout
 */

#pragma once

// STD
#include <map>
// JSON
#include "serialization/json.hpp"
// DDS
#include "common/dds/dds_topic_channel.hpp"

namespace jsr::common::dds {

/**
 * @brief DDS factory model
 *
 */
class DdsFactoryModel {
   public:
    DdsFactoryModel();
    DdsFactoryModel(const DdsFactoryModel&) = default;
    DdsFactoryModel(DdsFactoryModel&&) = default;
    DdsFactoryModel& operator=(const DdsFactoryModel&) = default;
    DdsFactoryModel& operator=(DdsFactoryModel&&) = default;
    ~DdsFactoryModel() = default;

    /**
     * @brief Initialize DDS factory model, create participant, publisher and subscriber, register type support and create topic.
     *
     * @param domain_id
     * @param network_interface
     */
    void Init(int32_t domain_id, const std::string& network_interface = "");

    /**
     * @brief  Use json to initialize DDS factory model
     *
     * @param config json
     */
    void Init(const nlohmann::json& config);

    void CloseWriter(const std::string& channel_name) {
        publisher_->delete_datawriter(publisher_->lookup_datawriter(channel_name));
    }

    void CloseReader(const std::string& channel_name) {
        subscriber_->delete_datareader(subscriber_->lookup_datareader(channel_name));
    }

    void CloseTopic(const DdsTopicPtr& topic) { participant_->delete_topic(topic.get()); }

    DdsTopicPtr GetTopic(const std::string& topic_name) {
        auto it = topic_map_.find(topic_name);
        if (it != topic_map_.end()) {
            return it->second;
        }
        return nullptr;
    }

    // static
    template <typename MSG, typename = not_dynamic_data_t<MSG>>
    DdsTopicChannelPtr<MSG> CreateTopicChannel(const std::string& topic_name) {
        DdsTypeSupport type_support(new MSG());
        DdsTopicChannelPtr<MSG> topic_channel = std::make_shared<DdsTopicChannel<MSG>>();
        if (participant_ != nullptr) {
            auto topic = GetTopic(topic_name);
            if (topic == nullptr) {
                type_support.register_type(participant_.get());
                topic = DdsTopicPtr(
                    participant_->create_topic(topic_name, type_support.get_type_name(), DDS_TOPIC_QOS_DEFAULT),
                    [](DdsTopic*) {});
                if (topic == nullptr) {
                    fmt::print(stderr, "Failed to create topic.\n");
                    return nullptr;
                }
                topic_map_[topic_name] = topic;
            }
            topic_channel->SetTopic(topic);
            return topic_channel;
        }
        fmt::print(stderr, "Failed to create participant.\n");
        return nullptr;
    }

    // dynamic
    template <typename MSG, typename = is_dynamic_data_t<MSG>>
    DdsTopicChannelPtr<MSG> CreateTopicChannel(const std::string& topic_name,
                                               const DdsDynamicTypeBuilder::_ref_type& type_builder) {
        DdsTypeSupport type_support(new DdsDynamicPubSubType(type_builder->build()));
        DdsTopicChannelPtr<MSG> topic_channel = std::make_shared<DdsTopicChannel<MSG>>();
        if (participant_ != nullptr) {
            auto topic = GetTopic(topic_name);
            if (topic == nullptr) {
                type_support.register_type(participant_.get());
                topic = DdsTopicPtr(
                    participant_->create_topic(topic_name, type_support.get_type_name(), DDS_TOPIC_QOS_DEFAULT),
                    [](DdsTopic*) {});
                if (topic == nullptr) {
                    fmt::print(stderr, "Failed to create topic.\n");
                    return nullptr;
                }
                topic_map_[topic_name] = topic;
            }
            topic_channel->SetTopic(topic);
            return topic_channel;
        }
        fmt::print(stderr, "Failed to create participant.\n");
        return nullptr;
    }

    template <typename MSG>
    void SetWriter(DdsTopicChannelPtr<MSG> topic_channel) {
        topic_channel->SetWriter(publisher_, writer_qos_);
    }

    template <typename MSG, typename = not_dynamic_data_t<MSG>>
    void SetReader(DdsTopicChannelPtr<MSG> topic_channel, const std::function<void(const void*)>& handler) {
        DdsReaderCallback cb(handler);
        topic_channel->SetReader(subscriber_, reader_qos_, cb);
    }

    template <typename MSG, typename = is_dynamic_data_t<MSG>>
    void SetReader(DdsTopicChannelPtr<MSG> topic_channel, DdsDynamicTypeBuilder::_ref_type type_builder,
                   const std::function<void(const void*)>& handler) {
        DdsReaderCallback cb(handler);
        topic_channel->SetReader(subscriber_, reader_qos_, type_builder, cb);
    }

   private:
    DdsParticipantPtr participant_;
    DdsPublisherPtr publisher_;
    DdsSubscriberPtr subscriber_;

    std::map<std::string, DdsTopicPtr> topic_map_;

    DdsDomainParticipantQos participant_qos_;
    DdsTopicQos topic_qos_;
    DdsPublisherQos publisher_qos_;
    DdsSubscriberQos subscriber_qos_;
    DataWriterQos writer_qos_;
    DataReaderQos reader_qos_;
};

using DdsFactoryModelPtr = std::shared_ptr<DdsFactoryModel>;

}  // namespace jsr::common::dds
