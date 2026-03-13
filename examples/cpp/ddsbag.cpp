// STD
#include <chrono>
#include <exception>
#include <fstream>
#include <thread>
// JSRCOMM
#include "jsrcomm/common/dds/dds_dynamic_factory.hpp"
#include "jsrcomm/robot/channel/channel_publisher.hpp"
#include "jsrcomm/robot/channel/channel_subscriber.hpp"

namespace jrc = jsr::robot::channel;
namespace jcd = jsr::common::dds;

class DdsBag {
   public:
    DdsBag() = default;

    void setInput(const std::string& input_path) { input_path_ = input_path; }
    void setOutput(const std::string& output_path) { output_path_ = output_path; }

    void init() {
        if (!output_path_.empty()) {
            record_file_.open(output_path_, std::ios::out | std::ios::binary);
        }
        if (!input_path_.empty()) {
            play_file_.open(input_path_, std::ios::in | std::ios::binary);
        }
    }

    void subscribeTopic(const std::string& topic_name, const std::string& type_name, const std::string& idl_path,
                        const std::string& idl_dir) {
        current_topic_ = topic_name;
        auto type_builder = jcd::DdsDynamicFactory::parseTypeFromIdlWithRos2(idl_path, type_name, idl_dir);
        auto type = type_builder->build();
        auto sub = std::make_shared<jrc::ChannelSubscriber<jcd::DdsDynamicData>>(
            topic_name, type_builder, [this, type](const void* msg) { this->recordCallback(msg, type); });
        sub->initChannel();
        subscribers_.push_back(sub);
    }

    void playBag(const std::string& type_name, const std::string& idl_path, const std::string& idl_dir) {
        auto type_builder = jcd::DdsDynamicFactory::parseTypeFromIdlWithRos2(idl_path, type_name, idl_dir);
        auto type = type_builder->build();
        if (!play_file_.is_open()) {
            throw std::runtime_error("play file not open");
            return;
        }
        std::unordered_map<std::string, std::shared_ptr<jrc::ChannelPublisher<jcd::DdsDynamicData>>> publishers;
        int64_t last_msg_timestamp = 0;
        while (play_file_.peek() != EOF) {
            uint32_t size = 0;
            play_file_.read(static_cast<char*>(static_cast<void*>((&size))), sizeof(size));
            if (play_file_.gcount() != sizeof(size)) {
                break;
            }
            std::string serialized(size, ' ');
            play_file_.read(serialized.data(), size);
            auto json = nlohmann::json::parse(serialized);
            std::string topic = json["topic"];
            int64_t timestamp = 0;
            timestamp = json["timestamp"];
            json.erase("topic");
            json.erase("timestamp");
            if (last_msg_timestamp != 0) {
                auto diff = timestamp - last_msg_timestamp;
                if (diff > 0) {
                    std::this_thread::sleep_for(std::chrono::nanoseconds(diff));
                }
            }
            last_msg_timestamp = timestamp;

            if (publishers.find(topic) == publishers.end()) {
                auto pub = std::make_shared<jrc::ChannelPublisher<jcd::DdsDynamicData>>(topic, type_builder);
                pub->initChannel();
                publishers[topic] = pub;
            }

            auto data = jcd::DdsDynamicFactory::parseFromJson(json, type);
            publishers[topic]->write(&data);
        }
    }

    void recordCallback(const void* msg, const jcd::TypeRef& type) {
        if (!is_recording_) {
            return;
        }
        if (!record_file_.is_open()) {
            throw std::runtime_error("record file not open");
            return;
        }
        auto data = *static_cast<const jcd::DdsDynamicData::_ref_type*>(msg);
        auto json = jcd::DdsDynamicFactory::toJson(data);
        json["timestamp"] = std::chrono::system_clock::now().time_since_epoch().count();
        json["topic"] = current_topic_;
        std::string serialized = json.dump();
        uint32_t size = serialized.size();
        record_file_.write(static_cast<char*>(static_cast<void*>(&size)), sizeof(size));
        record_file_.write(serialized.data(), size);
    }

    void startRecording() { is_recording_ = true; }

    void stopRecording() { is_recording_ = false; }

    ~DdsBag() {
        if (record_file_.is_open()) {
            record_file_.close();
        }
        if (play_file_.is_open()) {
            play_file_.close();
        }
    }

   private:
    std::string output_path_;
    std::string input_path_;
    std::ofstream record_file_;
    std::ifstream play_file_;
    std::vector<std::shared_ptr<jrc::ChannelSubscriber<jcd::DdsDynamicData>>> subscribers_;
    bool is_recording_ = false;
    std::string current_topic_;
};

int main() {
    jrc::ChannelFactory::instance()->init(0);

    DdsBag dds_bag;
    // dds_bag.setInput("xxx.bag");
    dds_bag.setOutput("xxx.bag");
    dds_bag.init();
    // dds_bag.playBag("jsr::msg::LowState", "../../idl/LowState.idl", "../../idl");

    dds_bag.subscribeTopic("rt/low_state", "jsr::msg::LowState", "../../idl/LowState.idl", "../../idl");
    dds_bag.startRecording();
    std::this_thread::sleep_for(std::chrono::seconds(10));
    dds_bag.stopRecording();

    return 0;
}
