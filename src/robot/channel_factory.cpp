// DDS
#include "robot/channel/channel_factory.hpp"

namespace jsr::robot::channel {

void ChannelFactory::Init(int32_t domain_id, const std::string& network_interface) {
    if (!initialized_) {
        dds_factory_model_ = std::make_shared<jsr::common::dds::DdsFactoryModel>();
        dds_factory_model_->Init(domain_id, network_interface);
        initialized_ = true;
        return;
    }
    fmt::print(stderr, "ChannelFactory has already been initialized\n");
    return;
}

void ChannelFactory::Init(const nlohmann::json& config) {
    if (!initialized_) {
        dds_factory_model_ = std::make_shared<jsr::common::dds::DdsFactoryModel>();
        dds_factory_model_->Init(config);
        return;
    }
    fmt::print(stderr, "ChannelFactory has already been initialized\n");
    return;
}

void ChannelFactory::CloseWriter(const std::string& channel_name) {
    std::lock_guard<std::mutex> lock(mutex_);
    dds_factory_model_->CloseWriter(channel_name);
}
void ChannelFactory::CloseReader(const std::string& channel_name) {
    std::lock_guard<std::mutex> lock(mutex_);
    dds_factory_model_->CloseReader(channel_name);
}
void ChannelFactory::CloseTopic(const TopicPtr& topic) {
    std::lock_guard<std::mutex> lock(mutex_);
    dds_factory_model_->CloseTopic(topic);
}

}  // namespace jsr::robot::channel