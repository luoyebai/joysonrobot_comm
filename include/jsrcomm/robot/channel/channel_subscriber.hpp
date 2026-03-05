#pragma once

#include "channel_factory.hpp"
#include "jsrcomm/robot/rpc/response.hpp"

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
    explicit ChannelSubscriber(std::string channel_name) : channel_name_(std::move(channel_name)) {}
    template <typename T = MSG, typename = jsr::common::dds::not_dynamic_data_t<T>>
    explicit ChannelSubscriber(std::string channel_name, std::function<void(const void*)> handler)
        : channel_name_(std::move(channel_name)), handler_(std::move(handler)) {}

    template <typename T = MSG, typename = jsr::common::dds::is_dynamic_data_t<T>>
    explicit ChannelSubscriber(std::string channel_name,
                               jsr::common::dds::DdsDynamicTypeBuilder::_ref_type type_builder)
        : channel_name_(std::move(channel_name)), type_builder_(std::move(type_builder)) {}
    template <typename T = MSG, typename = jsr::common::dds::is_dynamic_data_t<T>>
    explicit ChannelSubscriber(std::string channel_name,
                               jsr::common::dds::DdsDynamicTypeBuilder::_ref_type type_builder,
                               std::function<void(const void*)> handler)
        : channel_name_(std::move(channel_name)),
          type_builder_(std::move(type_builder)),
          handler_(std::move(handler)) {}

    void initChannel(std::function<void(const void*)> handler) {
        handler_ = std::move(handler);
        initChannel();
    }

    void initChannel() {
        if (!handler_) {
            fmt::print(stderr, "ChannelSubscriber::initChannel: handler is not set\n");
            return;
        }
        if constexpr (jsr::common::dds::IS_DYNAMIC_DATA_V<MSG>) {
            assert(type_builder_ != nullptr && "ChannelSubscriber::initChannel(): type builder is null");
            channel_ptr_ = ChannelFactory::instance()->createRecvChannel(channel_name_, type_builder_, handler_);
        } else {
            channel_ptr_ = ChannelFactory::instance()->createRecvChannel<MSG>(channel_name_, handler_);
        }
    }

    void closeChannel() {
        if (channel_ptr_) {
            channel_ptr_.reset();
            ChannelFactory::instance()->closeReader(channel_name_);
        }
    }

    const std::string& getChannelName() const { return channel_name_; }

   private:
    std::string channel_name_;
    std::function<void(const void*)> handler_{nullptr};
    ChannelPtr<MSG> channel_ptr_{nullptr};
    // dynamic
    jsr::common::dds::DdsDynamicTypeBuilder::_ref_type type_builder_{nullptr};
};

template <typename MSG>
using ChannelSubscriberPtr = std::shared_ptr<ChannelSubscriber<MSG>>;

}  // namespace jsr::robot::channel
