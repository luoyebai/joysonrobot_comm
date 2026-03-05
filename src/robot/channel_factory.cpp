// DDS
#include "jsrcomm/robot/channel/channel_factory.hpp"

namespace jsr::robot::channel {

void ChannelFactory::init(int32_t domain_id, const std::string& network_interface) {
    if (!initialized_) {
        dds_factory_model_ = std::make_shared<jsr::common::dds::DdsFactoryModel>();
        dds_factory_model_->init(domain_id, network_interface);
        initialized_ = true;
        return;
    }
    fmt::print(stderr, "ChannelFactory has already been initialized\n");
    return;
}

void ChannelFactory::init(const nlohmann::json& config) {
    if (!initialized_) {
        dds_factory_model_ = std::make_shared<jsr::common::dds::DdsFactoryModel>();
        dds_factory_model_->init(config);
        return;
    }
    fmt::print(stderr, "ChannelFactory has already been initialized\n");
    return;
}

void ChannelFactory::closeWriter(const std::string& channel_name) {
    std::lock_guard<std::mutex> lock(mutex_);
    dds_factory_model_->closeWriter(channel_name);
}
void ChannelFactory::closeReader(const std::string& channel_name) {
    std::lock_guard<std::mutex> lock(mutex_);
    dds_factory_model_->closeReader(channel_name);
}
void ChannelFactory::closeTopic(const TopicPtr& topic) {
    std::lock_guard<std::mutex> lock(mutex_);
    dds_factory_model_->closeTopic(topic);
}

}  // namespace jsr::robot::channel