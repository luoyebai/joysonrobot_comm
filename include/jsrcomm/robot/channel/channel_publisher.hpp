#pragma once

// STD
#include <memory>
#include <string>
// ROBOT CHANNEL
#include "channel_factory.hpp"
// ROBOT RPC
#include "jsrcomm/robot/rpc/request.hpp"

namespace jsr::robot::channel {

/**
 * @brief Channel publisher class
 *
 * @tparam MSG Message type
 */
template <typename MSG>
class ChannelPublisher {
   public:
    template <typename T = MSG, typename = jsr::common::dds::not_dynamic_data_t<T>>
    explicit ChannelPublisher(std::string channel_name) : channel_name_(std::move(channel_name)) {}

    template <typename T = MSG, typename = jsr::common::dds::is_dynamic_data_t<T>>
    explicit ChannelPublisher(std::string channel_name, jsr::common::dds::DdsDynamicTypeBuilder::_ref_type type_builder)
        : channel_name_(std::move(channel_name)), type_builder_(std::move(type_builder)) {}

    void initChannel() {
        if constexpr (jsr::common::dds::IS_DYNAMIC_DATA_V<MSG>) {
            assert(type_builder_ != nullptr && "ChannelPublisher::initChannel(): type builder is null");
            channel_ptr_ = ChannelFactory::instance()->createSendChannel(channel_name_, type_builder_);
        } else {
            channel_ptr_ = ChannelFactory::instance()->createSendChannel<MSG>(channel_name_);
        }
        return;
    }

    bool write(void* msg) {
        if (channel_ptr_) {
            return channel_ptr_->write(msg);
        }
        return false;
    }

    void closeChannel() {
        if (channel_ptr_) {
            ChannelFactory::instance()->closeWriter(channel_name_);
            channel_ptr_.reset();
        }
    }

    const std::string& getChannelName() const { return channel_name_; }

   private:
    std::string channel_name_;
    ChannelPtr<MSG> channel_ptr_;
    // dynamic
    jsr::common::dds::DdsDynamicTypeBuilder::_ref_type type_builder_;
};

template <typename MSG>
using ChannelPublisherPtr = std::shared_ptr<ChannelPublisher<MSG>>;

}  // namespace jsr::robot::channel
