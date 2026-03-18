// STD
#include <chrono>
#include <exception>
#include <fstream>
#include <shared_mutex>
#include <thread>
// JSRCOMM
#include "jsrcomm/robot/channel/channel_blackboard.hpp"
#include "jsrcomm/robot/channel/channel_publisher.hpp"
#include "jsrcomm/robot/channel/channel_subscriber.hpp"
// IDL
#include "jsrcomm/idl/LowState.hpp"

namespace jrc = jsr::robot::channel;
namespace jcd = jsr::common::dds;

enum class DdsBagMode { RECORD, PLAY };

class DdsBag {
   public:
    explicit DdsBag(DdsBagMode mode, const std::string& bag_name) : mode_(mode), bag_file_name_(bag_name) {}
    ~DdsBag() {
        std::unique_lock lock(mutex_);
        if (out_.is_open()) {
            out_.close();
        }
        if (in_.is_open()) {
            in_.close();
        }
        player_.clear();
        play_callbacks_.clear();
    }

    template <typename T>
    void registerTopic(const std::string& topic) {
        if (mode_ == DdsBagMode::RECORD) {
            // subscriber
            bb_.registerTopic<T>(topic, [this, topic](const T* msg) {
                auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                              std::chrono::system_clock::now().time_since_epoch())
                              .count();
                saveDdsData(topic, ns, *msg);
            });
        } else if (mode_ == DdsBagMode::PLAY) {
            // publisher
            if (player_.count(topic) != 0) {
                throw std::runtime_error("Topic " + topic + " has been registered.");
            }
            auto pub = std::make_shared<jrc::ChannelPublisher<T>>(topic);
            pub->initChannel();
            player_[topic] = pub;
            play_callbacks_[topic] = [this, topic, pub](const char* data, uint32_t size) {
                eprosima::fastcdr::FastBuffer buffer(const_cast<char*>(data), size);
                eprosima::fastcdr::Cdr deser(buffer);
                T msg{};
                deser.read_encapsulation();
                deser.deserialize(msg);
                pub->write(&msg);
            };
        } else {
            throw std::runtime_error("Invalid mode.");
        }
    }

    void play() {
        if (mode_ != DdsBagMode::PLAY) {
            throw std::runtime_error("Invalid mode.");
        }
        if (!in_.is_open()) {
            in_.open(bag_file_name_, std::ios::in | std::ios::binary);
        }
        if (!in_.is_open()) {
            throw std::runtime_error("Can't open file " + bag_file_name_ + "for reading.");
        }
        while (true) {
            if (in_.eof()) {
                break;
            }
            bool read_done = false;
            uint32_t topic_len;
            std::vector<char> topic_raw;
            if (!in_.read(reinterpret_cast<char*>(&topic_len), sizeof(topic_len))) {
                break;
            }
            topic_raw.resize(topic_len);
            if (!in_.read(topic_raw.data(), topic_len)) {
                break;
            }
            auto topic = std::string(topic_raw.data(), topic_len);

            int64_t ns;
            if (!in_.read(reinterpret_cast<char*>(&ns), sizeof(ns))) {
                break;
            }
            static auto last_ns = ns;

            uint32_t length;
            std::vector<char> data;
            if (!in_.read(reinterpret_cast<char*>(&length), sizeof(length))) {
                break;
            }
            data.resize(length);
            if (!in_.read(data.data(), length)) {
                break;
            }
            if (play_callbacks_.count(topic) != 0) {
                play_callbacks_[topic](data.data(), length);
            }
            std::this_thread::sleep_for(std::chrono::nanoseconds(ns - last_ns));
            last_ns = ns;
        }
    }

   private:
    DdsBagMode mode_;
    std::string bag_file_name_;
    std::ofstream out_;
    std::ifstream in_;

    jrc::ChannelBlackboard bb_;
    mutable std::shared_mutex mutex_;

    std::unordered_map<std::string, std::shared_ptr<void>> player_;
    std::unordered_map<std::string, std::function<void(const char*, uint32_t)>> play_callbacks_;

    template <typename T>
    void saveDdsData(const std::string& topic, int64_t ns, T data) {
        eprosima::fastcdr::FastBuffer fastbuffer;
        eprosima::fastcdr::Cdr ser(fastbuffer, eprosima::fastcdr::Cdr::DEFAULT_ENDIAN);
        if (!out_.is_open()) {
            out_.open(bag_file_name_, std::ios::out | std::ios::binary | std::ios::app);
        }
        if (!out_.is_open()) {
            throw std::runtime_error("Can't open file " + bag_file_name_ + "for writing.");
        }

        // 1. topic write
        uint32_t topic_len = static_cast<uint32_t>(topic.size());
        out_.write(reinterpret_cast<const char*>(&topic_len), sizeof(topic_len));
        out_.write(topic.c_str(), topic_len);

        // 2. time write
        out_.write(reinterpret_cast<const char*>(&ns), sizeof(ns));

        // 3. data write
        ser.serialize_encapsulation();
        ser.serialize(data);
        const char* data_ptr = fastbuffer.getBuffer();
        size_t data_size = ser.get_serialized_data_length();
        uint32_t length = static_cast<uint32_t>(data_size);
        out_.write(reinterpret_cast<const char*>(&length), sizeof(length));
        out_.write(data_ptr, data_size);
    }
};

int main(const int argc, const char* argv[]) {
    if (argc < 3) {
        fmt::print(
            "Usage: {} <mode> <file path>\n"
            "<mode>: play,record.\n"
            "<file path>: input play bag name, save record bag name.\n",
            argv[0]);
        return 0;
    }

    constexpr auto TOPIC = "rt/low_state";
    constexpr auto SLEEP_TIME = 10;

    jrc::ChannelFactory::instance()->init(0);

    DdsBagMode mode;

    if (std::string(argv[1]) == "play") {
        mode = DdsBagMode::PLAY;
    } else if (std::string(argv[1]) == "record") {
        mode = DdsBagMode::RECORD;
    } else {
        fmt::print("<mode> parma must be play or record");
        return -1;
    }

    DdsBag dds_bag(mode, argv[2]);
    dds_bag.registerTopic<jsr::msg::LowState>(TOPIC);
    if (mode == DdsBagMode::PLAY) {
        dds_bag.play();
    } else {
        std::this_thread::sleep_for(std::chrono::seconds(SLEEP_TIME));
    }

    return 0;
}
