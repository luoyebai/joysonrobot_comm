#pragma once

// STD
#include <atomic>
#include <mutex>

// DDS
#include "common/dds/dds_factory_model.hpp"
// JSON
#include "serialization/json.hpp"

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
    static ChannelFactory* Instance() {
        static ChannelFactory instance;
        return &instance;
    }

    void Init(int32_t domain_id, const std::string& network_interface = "");
    void Init(const nlohmann::json& config);

    void CloseWriter(const std::string& channel_name);
    void CloseReader(const std::string& channel_name);
    void CloseTopic(const TopicPtr& topic);

    template <typename MSG, typename = jsr::common::dds::not_dynamic_data_t<MSG>>
    ChannelPtr<MSG> CreateSendChannel(const std::string& name) {
        if (!initialized_) {
            throw std::runtime_error(
                "ChannelFactory not initialized! Must exec ChannelFactory::Instance()->Init before use!");
        }
        ChannelPtr<MSG> channel_ptr = dds_factory_model_->CreateTopicChannel<MSG>(name);
        dds_factory_model_->SetWriter(channel_ptr);
        return channel_ptr;
    }

    template <typename MSG, typename = jsr::common::dds::not_dynamic_data_t<MSG>>
    ChannelPtr<MSG> CreateRecvChannel(const std::string& name, std::function<void(const void*)> handler) {
        if (!initialized_) {
            throw std::runtime_error(
                "ChannelFactory not initialized! Must exec ChannelFactory::Instance()->Init before use!");
        }
        auto channel_ptr = dds_factory_model_->CreateTopicChannel<MSG>(name);
        dds_factory_model_->SetReader(channel_ptr, handler);
        return channel_ptr;
    }

    // dynamic
    ChannelPtr<jsr::common::dds::DdsDynamicData> CreateSendChannel(
        const std::string& name, const jsr::common::dds::DdsDynamicTypeBuilder::_ref_type& type_builder) {
        if (!initialized_) {
            throw std::runtime_error(
                "ChannelFactory not initialized! Must exec ChannelFactory::Instance()->Init before use!");
        }
        auto channel_ptr = dds_factory_model_->CreateTopicChannel<jsr::common::dds::DdsDynamicData>(name, type_builder);
        dds_factory_model_->SetWriter(channel_ptr);
        return channel_ptr;
    }

    ChannelPtr<jsr::common::dds::DdsDynamicData> CreateRecvChannel(
        const std::string& name, const jsr::common::dds::DdsDynamicTypeBuilder::_ref_type& type_builder,
        const std::function<void(const void*)>& handler) {
        if (!initialized_) {
            throw std::runtime_error(
                "ChannelFactory not initialized! Must exec ChannelFactory::Instance()->Init before use!");
        }
        auto channel_ptr = dds_factory_model_->CreateTopicChannel<jsr::common::dds::DdsDynamicData>(name, type_builder);
        dds_factory_model_->SetReader(channel_ptr, type_builder, handler);
        return channel_ptr;
    }

   private:
    bool initialized_ = false;
    std::mutex mutex_;
    jsr::common::dds::DdsFactoryModelPtr dds_factory_model_;
};

}  // namespace jsr::robot::channel