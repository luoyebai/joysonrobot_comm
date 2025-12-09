// STD
#include <map>
#include <memory>
// JSON
#include "serialization/json.hpp"
// DDS
#include "common/dds/dds_factory_model.hpp"
// FASTDDS
#include "fastdds/dds/domain/DomainParticipant.hpp"
#include "fastdds/dds/domain/DomainParticipantFactory.hpp"

namespace jsr::common::dds {

DdsFactoryModel::DdsFactoryModel()
    : participant_(nullptr),
      publisher_(nullptr),
      subscriber_(nullptr),
      participant_qos_(DDS_PARTICIPANT_QOS_DEFAULT),
      publisher_qos_(DDS_PUBLISHER_QOS_DEFAULT),
      subscriber_qos_(DDS_SUBSCRIBER_QOS_DEFAULT),
      writer_qos_(DDS_DATAWRITER_QOS_DEFAULT),
      reader_qos_(DDS_DATAREADER_QOS_DEFAULT) {
    writer_qos_.data_sharing().automatic();
    reader_qos_.data_sharing().automatic();
    return;
}

DdsFactoryModel::~DdsFactoryModel() = default;

void DdsFactoryModel::Init(int32_t domain_id, const std::string& network_interface) {
    if (!network_interface.empty()) {
        // Create a descriptor for the new transport.
        auto udp_transport = std::make_shared<DdsUDPv4TransportDescriptor>();
        udp_transport->interfaceWhiteList.emplace_back(network_interface);
        participant_qos_.transport().use_builtin_transports = false;
        participant_qos_.transport().user_transports.push_back(udp_transport);
    }

    DdsDomainParticipant* raw_participant =
        DdsDomainParticipantFactory::get_instance()->create_participant(domain_id, participant_qos_);
    // RAII: don't need deleter
    participant_ = DdsParticipantPtr(raw_participant, [](DdsDomainParticipant*) {});

    if (!participant_) {
        throw std::runtime_error("Failed to create DomainParticipant.");
        return;
    }
    // Init publisher
    DdsPublisher* raw_publisher = participant_->create_publisher(publisher_qos_, nullptr);
    publisher_ = DdsPublisherPtr(raw_publisher, [](DdsPublisher*) {});
    if (!publisher_) {
        throw std::runtime_error("Failed to create Publisher.");
        return;
    }

    // Init subscriber
    DdsSubscriber* raw_subscriber = participant_->create_subscriber(subscriber_qos_, nullptr);
    subscriber_ = DdsSubscriberPtr(raw_subscriber, [](DdsSubscriber*) {});

    if (!subscriber_) {
        throw std::runtime_error("Failed to create Subscriber.");
        return;
    }
}

void DdsFactoryModel::Init(const nlohmann::json& config) {
    if (config.empty()) {
        std::runtime_error("DdsFactoryModel config is empty");
        return;
    }
    int32_t domain_id = config.value("domain_id", 0);
    std::string network_interface = config.value("network_interface", "");
    this->Init(domain_id, network_interface);
}

}  // namespace jsr::common::dds
