#pragma once

// STD
#include <atomic>
#include <mutex>
// JSON
#include "serialization/json.hpp"
// DDS
#include "jsrcomm/common/dds/dds_factory_model.hpp"

namespace jsr::robot::channel {

template <typename MSG>
using Channel = jsr::common::dds::DdsTopicChannel<MSG>;

template <typename MSG>
using ChannelPtr = jsr::common::dds::DdsTopicChannelPtr<MSG>;

using TopicDataTypePtr = jsr::common::dds::DdsTopicDataTypePtr;
using TopicPtr = jsr::common::dds::DdsTopicPtr;

/**
 * @brief Factory for creating channels
 *
 */
class ChannelFactory {
   public:
    static ChannelFactory* instance() {
        static ChannelFactory instance;
        return &instance;
    }

    void init(int32_t domain_id, const std::string& network_interface = "");
    void init(const nlohmann::json& config);

    void closeWriter(const std::string& channel_name);
    void closeReader(const std::string& channel_name);
    void closeTopic(const TopicPtr& topic);

    template <typename MSG, typename = jsr::common::dds::not_dynamic_data_t<MSG>>
    ChannelPtr<MSG> createSendChannel(const std::string& name) {
        if (!initialized_) {
            throw std::runtime_error(
                "ChannelFactory not initialized! Must exec ChannelFactory::instance()->init before use!");
        }
        ChannelPtr<MSG> channel_ptr = dds_factory_model_->createTopicChannel<MSG>(name);
        dds_factory_model_->setWriter(channel_ptr);
        return channel_ptr;
    }

    template <typename MSG, typename = jsr::common::dds::not_dynamic_data_t<MSG>>
    ChannelPtr<MSG> createRecvChannel(const std::string& name, std::function<void(const void*)> handler) {
        if (!initialized_) {
            throw std::runtime_error(
                "ChannelFactory not initialized! Must exec ChannelFactory::instance()->init before use!");
        }
        auto channel_ptr = dds_factory_model_->createTopicChannel<MSG>(name);
        dds_factory_model_->setReader(channel_ptr, handler);
        return channel_ptr;
    }

    // dynamic
    ChannelPtr<jsr::common::dds::DdsDynamicData> createSendChannel(
        const std::string& name, const jsr::common::dds::DdsDynamicTypeBuilder::_ref_type& type_builder) {
        if (!initialized_) {
            throw std::runtime_error(
                "ChannelFactory not initialized! Must exec ChannelFactory::instance()->init before use!");
        }
        auto channel_ptr = dds_factory_model_->createTopicChannel<jsr::common::dds::DdsDynamicData>(name, type_builder);
        dds_factory_model_->setWriter(channel_ptr);
        return channel_ptr;
    }

    ChannelPtr<jsr::common::dds::DdsDynamicData> createRecvChannel(
        const std::string& name, const jsr::common::dds::DdsDynamicTypeBuilder::_ref_type& type_builder,
        const std::function<void(const void*)>& handler) {
        if (!initialized_) {
            throw std::runtime_error(
                "ChannelFactory not initialized! Must exec ChannelFactory::instance()->init before use!");
        }
        auto channel_ptr = dds_factory_model_->createTopicChannel<jsr::common::dds::DdsDynamicData>(name, type_builder);
        dds_factory_model_->setReader(channel_ptr, type_builder, handler);
        return channel_ptr;
    }

   private:
    bool initialized_ = false;
    std::mutex mutex_;
    jsr::common::dds::DdsFactoryModelPtr dds_factory_model_{nullptr};
};

}  // namespace jsr::robot::channel