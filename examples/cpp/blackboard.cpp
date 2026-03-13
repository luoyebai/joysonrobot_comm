// STD
#include <map>
#include <optional>
#include <shared_mutex>
#include <string>
// JSRCOMM
#include "jsrcomm/common/dds/dds_dynamic_factory.hpp"
#include "jsrcomm/robot/channel/channel_subscriber.hpp"
// IDL
#include "jsrcomm/idl/LowState.hpp"

namespace jrc = jsr::robot::channel;
namespace jcd = jsr::common::dds;

/**
 * @brief 强类型数据桶：管理同一类型下的所有话题
 */
template <typename T>
class DdsDataBucket {
   public:
    static DdsDataBucket& instance() {
        static DdsDataBucket instance;
        return instance;
    }

    // 更新特定话题的数据
    void update(const std::string& topic, const T& data) {
        std::unique_lock lock(mutex_);
        storage_[topic] = data;
    }

    // 获取特定话题的最新数据
    std::optional<T> get(const std::string& topic) const {
        std::shared_lock lock(mutex_);
        auto it = storage_.find(topic);
        if (it != storage_.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    // 获取该类型下所有已注册的话题名
    std::vector<std::string> getAllTopics() const {
        std::shared_lock lock(mutex_);
        std::vector<std::string> topics;
        for (const auto& [topic, _] : storage_) {
            topics.push_back(topic);
        }
        return topics;
    }

   private:
    DdsDataBucket() = default;
    mutable std::shared_mutex mutex_;
    std::unordered_map<std::string, T> storage_;
};

/**
 * @brief 通用黑板门面（Facade）
 */
class Blackboard {
   public:
    template <typename T>
    static void set(const std::string& topic, const T& data) {
        DdsDataBucket<T>::instance().update(topic, data);
    }

    template <typename T>
    static auto get(const std::string& topic) {
        return DdsDataBucket<T>::instance().get(topic);
    }

    template <typename T>
    void registerTopic(const std::string& topic) {
        static auto sub = std::make_shared<jrc::ChannelSubscriber<T>>(topic, [this, topic](const void* msg) {
            std::cout << "I get it \n";
            const auto* data = static_cast<const T*>(msg);
            set<T>(topic, *data);
        });
        sub->initChannel();
    }

   private:
};

int main() {
    jrc::ChannelFactory::instance()->init(0);
    Blackboard bb;
    bb.registerTopic<jsr::msg::LowState>("rt/low_state");
    while (true) {
        auto low_state = Blackboard::get<jsr::msg::LowState>("rt/low_state");
        if (low_state.has_value()) {
            std::cout << "Acc_2: " << low_state->imu_state().acc()[2] << std::endl;
        }
    }
    return 0;
}