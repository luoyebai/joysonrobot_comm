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
#include "dds_topic_channel.hpp"

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
    void init(int32_t domain_id, const std::string& network_interface = "");

    /**
     * @brief  Use json to initialize DDS factory model
     *
     * @param config json
     */
    void init(const nlohmann::json& config);

    DdsTopicPtr getTopic(const std::string& topic_name) const {
        auto it = topic_map_.find(topic_name);
        if (it != topic_map_.end()) {
            return it->second;
        }
        return nullptr;
    }

    void closeWriter(const std::string& channel_name) {
        publisher_->delete_datawriter(publisher_->lookup_datawriter(channel_name));
    }

    void closeReader(const std::string& channel_name) {
        subscriber_->delete_datareader(subscriber_->lookup_datareader(channel_name));
    }

    void closeTopic(const DdsTopicPtr& topic) { participant_->delete_topic(topic.get()); }

    // static
    template <typename MSG, typename = not_dynamic_data_t<MSG>>
    DdsTopicChannelPtr<MSG> createTopicChannel(const std::string& topic_name) {
        DdsTypeSupport type_support(new MSG());
        DdsTopicChannelPtr<MSG> topic_channel = std::make_shared<DdsTopicChannel<MSG>>();
        if (participant_ != nullptr) {
            auto topic = getTopic(topic_name);
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
            topic_channel->setTopic(topic);
            return topic_channel;
        }
        fmt::print(stderr, "Failed to create participant.\n");
        return nullptr;
    }

    // dynamic
    template <typename MSG, typename = is_dynamic_data_t<MSG>>
    DdsTopicChannelPtr<MSG> createTopicChannel(const std::string& topic_name,
                                               const DdsDynamicTypeBuilder::_ref_type& type_builder) {
        DdsTypeSupport type_support(new DdsDynamicPubSubType(type_builder->build()));
        DdsTopicChannelPtr<MSG> topic_channel = std::make_shared<DdsTopicChannel<MSG>>();
        if (participant_ != nullptr) {
            auto topic = getTopic(topic_name);
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
            topic_channel->setTopic(topic);
            return topic_channel;
        }
        fmt::print(stderr, "Failed to create participant.\n");
        return nullptr;
    }

    template <typename MSG>
    void setWriter(DdsTopicChannelPtr<MSG> topic_channel) {
        topic_channel->setWriter(publisher_, writer_qos_);
    }

    template <typename MSG, typename = not_dynamic_data_t<MSG>>
    void setReader(DdsTopicChannelPtr<MSG> topic_channel, const std::function<void(const void*)>& handler) {
        DdsReaderCallback cb(handler);
        topic_channel->setReader(subscriber_, reader_qos_, cb);
    }

    template <typename MSG, typename = is_dynamic_data_t<MSG>>
    void setReader(DdsTopicChannelPtr<MSG> topic_channel, DdsDynamicTypeBuilder::_ref_type type_builder,
                   const std::function<void(const void*)>& handler) {
        DdsReaderCallback cb(handler);
        topic_channel->setReader(subscriber_, reader_qos_, type_builder, cb);
    }

   private:
    DdsParticipantPtr participant_;
    // dds communication entity
    DdsPublisherPtr publisher_;
    DdsSubscriberPtr subscriber_;
    // topic map
    std::map<std::string, DdsTopicPtr> topic_map_;
    // config
    DdsDomainParticipantQos participant_qos_;
    DdsTopicQos topic_qos_;
    // dds
    DdsPublisherQos publisher_qos_;
    DdsSubscriberQos subscriber_qos_;
    DataWriterQos writer_qos_;
    DataReaderQos reader_qos_;
};

using DdsFactoryModelPtr = std::shared_ptr<DdsFactoryModel>;

}  // namespace jsr::common::dds
