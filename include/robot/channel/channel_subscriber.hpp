#pragma once

#include "robot/channel/channel_factory.hpp"
#include "robot/rpc/response.hpp"

namespace jsr::robot::channel {

/**
 * @brief ChannelSubscriber is a class that subscribes to a channel and receives messages.
 *
 * @tparam MSG Message type
 */
template <typename MSG>
class ChannelSubscriber {
   public:
    template <typename T = MSG, typename = jsr::common::dds::not_dynamic_data_t<T>>
    explicit ChannelSubscriber(const std::string& channel_name) : channel_name_(channel_name) {}
    template <typename T = MSG, typename = jsr::common::dds::not_dynamic_data_t<T>>
    explicit ChannelSubscriber(const std::string& channel_name, const std::function<void(const void*)>& handler)
        : channel_name_(channel_name), handler_(handler) {}

    template <typename T = MSG, typename = jsr::common::dds::is_dynamic_data_t<T>>
    explicit ChannelSubscriber(const std::string& channel_name,
                               jsr::common::dds::DdsDynamicTypeBuilder::_ref_type type_builder)
        : channel_name_(channel_name), type_builder_(type_builder) {}
    template <typename T = MSG, typename = jsr::common::dds::is_dynamic_data_t<T>>
    explicit ChannelSubscriber(const std::string& channel_name,
                               jsr::common::dds::DdsDynamicTypeBuilder::_ref_type type_builder,
                               const std::function<void(const void*)>& handler)
        : channel_name_(channel_name), type_builder_(type_builder), handler_(handler) {}

    void InitChannel(const std::function<void(const void*)>& handler) {
        handler_ = handler;
        InitChannel();
    }

    void InitChannel() {
        if (!handler_) {
            fmt::print(stderr, "ChannelSubscriber::InitChannel: handler is not set\n");
            return;
        }
        if constexpr (jsr::common::dds::is_dynamic_data_v<MSG>) {
            assert(type_builder_ != nullptr && "ChannelSubscriber::InitChannel(): type builder is null");
            channel_ptr_ = ChannelFactory::Instance()->CreateRecvChannel(channel_name_, type_builder_, handler_);
        } else {
            channel_ptr_ = ChannelFactory::Instance()->CreateRecvChannel<MSG>(channel_name_, handler_);
        }
    }

    void CloseChannel() {
        if (channel_ptr_) {
            ChannelFactory::Instance()->CloseReader(channel_name_);
            channel_ptr_.reset();
        }
    }

    const std::string& GetChannelName() const { return channel_name_; }

   private:
    std::string channel_name_;
    ChannelPtr<MSG> channel_ptr_;
    std::function<void(const void*)> handler_;
    // dynamic
    jsr::common::dds::DdsDynamicTypeBuilder::_ref_type type_builder_;
};

template <typename MSG>
using ChannelSubscriberPtr = std::shared_ptr<ChannelSubscriber<MSG>>;

}  // namespace jsr::robot::channel
