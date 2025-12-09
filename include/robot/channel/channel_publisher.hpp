#pragma once

// STD
#include <memory>
#include <string>
// ROBOT CHANNEL
#include "robot/channel/channel_factory.hpp"
// ROBOT RPC
#include "robot/rpc/request.hpp"

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
    explicit ChannelPublisher(const std::string& channel_name) : channel_name_(channel_name) {}

    template <typename T = MSG, typename = jsr::common::dds::is_dynamic_data_t<T>>
    explicit ChannelPublisher(const std::string& channel_name,
                              jsr::common::dds::DdsDynamicTypeBuilder::_ref_type type_builder)
        : channel_name_(channel_name), type_builder_(type_builder) {}

    void InitChannel() {
        if constexpr (jsr::common::dds::is_dynamic_data_v<MSG>) {
            assert(type_builder_ != nullptr && "ChannelPublisher::InitChannel(): type builder is null");
            channel_ptr_ = ChannelFactory::Instance()->CreateSendChannel(channel_name_, type_builder_);
        } else {
            channel_ptr_ = ChannelFactory::Instance()->CreateSendChannel<MSG>(channel_name_);
        }
        return;
    }

    bool Write(void* msg) {
        if (channel_ptr_) {
            return channel_ptr_->Write(msg);
        }
        return false;
    }

    void CloseChannel() {
        if (channel_ptr_) {
            ChannelFactory::Instance()->CloseWriter(channel_name_);
            channel_ptr_.reset();
        }
    }

    const std::string& GetChannelName() const { return channel_name_; }

   private:
    std::string channel_name_;
    ChannelPtr<MSG> channel_ptr_;
    // dynamic
    jsr::common::dds::DdsDynamicTypeBuilder::_ref_type type_builder_;
};

template <typename MSG>
using ChannelPublisherPtr = std::shared_ptr<ChannelPublisher<MSG>>;

}  // namespace jsr::robot::channel
