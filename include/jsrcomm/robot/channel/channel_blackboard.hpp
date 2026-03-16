#pragma once

// STD
#include <map>
#include <optional>
#include <shared_mutex>
#include <string>
// DDS
#include "channel_subscriber.hpp"

namespace jsr::robot::channel {

template <typename T>
class ChannelDataBucket {
   public:
    static ChannelDataBucket& instance() {
        static ChannelDataBucket instance;
        return instance;
    }

    void update(const std::string& topic, T&& data) {
        std::unique_lock lock(mutex_);
        storage_[topic] = std::move(data);
    }

    void update(const std::string& topic, const T& data) {
        std::unique_lock lock(mutex_);
        storage_[topic] = data;
        timestamps_[topic] = std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
    }

    std::optional<T> get(const std::string& topic) const {
        std::shared_lock lock(mutex_);
        auto it = storage_.find(topic);
        if (it != storage_.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    std::optional<double> get_timestamp(const std::string& topic) const {
        std::shared_lock lock(mutex_);
        auto it = timestamps_.find(topic);
        if (it != timestamps_.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    std::vector<std::string> getAllTopics() const {
        std::shared_lock lock(mutex_);
        std::vector<std::string> topics;
        for (const auto& [topic, _] : storage_) {
            topics.push_back(topic);
        }
        return topics;
    }

    void addSubscriber(const std::string& topic, std::shared_ptr<void> sub) {
        std::unique_lock lock(sub_mutex_);
        if (subscribers_.count(topic) != 0) {
            throw std::runtime_error("Topic: " + topic + " already exists");
        }
        subscribers_.insert({topic, sub});
    }

    void removeStorage(const std::string& topic) {
        std::unique_lock lock(mutex_);
        storage_.erase(topic);
    }

    void removeSubscriber(const std::string& topic) {
        std::unique_lock lock(sub_mutex_);
        subscribers_.erase(topic);
    }

   private:
    ChannelDataBucket() = default;
    mutable std::shared_mutex mutex_;
    mutable std::shared_mutex sub_mutex_;
    std::unordered_map<std::string, T> storage_;
    std::unordered_map<std::string, std::shared_ptr<void>> subscribers_;
    std::unordered_map<std::string, double> timestamps_;
};

class ChannelBlackboard {
   public:
    template <typename T>
    static std::optional<T> get(const std::string& topic) {
        return ChannelDataBucket<T>::instance().get(topic);
    }

    /**
     * @brief Get the timestamp of the steady_clock
     * @tparam data type of the topic
     * @param topic topic name
     * 
     * @return timestamp of the topic
     */
    template <typename T>
    static std::optional<double> get_timestamp(const std::string& topic) {
        return ChannelDataBucket<T>::instance().get_timestamp(topic);
    }

    template <typename T>
    void registerTopic(const std::string& topic) {
        const auto registered_topics = ChannelDataBucket<T>::instance().getAllTopics();
        if (std::find(registered_topics.begin(), registered_topics.end(), topic) != registered_topics.end()) {
            throw std::runtime_error("Topic: " + topic + " already exists");
        }
        auto sub = std::make_shared<ChannelSubscriber<T>>(topic, [topic](const void* msg) {
            const auto* data_ptr = static_cast<const T*>(msg);
            ChannelDataBucket<T>::instance().update(topic, *data_ptr);
        });
        sub->initChannel();
        // Add reference counting to prevent stack recycling
        ChannelDataBucket<T>::instance().addSubscriber(topic, sub);
    }

    template <typename T>
    void registerTopic(const std::string& topic, std::function<void(const T*)> callback) {
        auto sub = std::make_shared<ChannelSubscriber<T>>(topic, [topic, callback](const void* msg) {
            const auto* data_ptr = static_cast<const T*>(msg);
            ChannelDataBucket<T>::instance().update(topic, *data_ptr);
            callback(data_ptr);
        });
        sub->initChannel();
        // Add reference counting to prevent stack recycling
        ChannelDataBucket<T>::instance().addSubscriber(topic, sub);
    }

    template <typename T>
    void removeTopic(const std::string& topic) {
        ChannelDataBucket<T>::instance().removeSubscriber(topic);
        ChannelDataBucket<T>::instance().removeStorage(topic);
    }
};

}  // namespace jsr::robot::channel