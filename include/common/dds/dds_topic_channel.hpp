/**
 * @file dds topic channel.hpp
 * @author TanJiachun (jiachun.tan@joysonrobot.com)
 * @brief DDS topic channel declare for Joyson Robot Robotics SDK.
 * @version 0.1
 * @date 2025-07-07
 *
 * @copyright Copyright 2025 Joyson Robotics, Inc. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * See the LICENSE file or http://www.apache.org/licenses/LICENSE-2.0 for details.
 *
 * Modifications:
 * - Changed namespace to `jsr::common::dds`
 */

#pragma once

// DDS
#include "common/dds/dds_entity.hpp"

namespace jsr::common::dds {

/**
 * @brief Use it in dds factory model
 *
 * @tparam MSG Message type
 */
template <typename MSG>
class DdsTopicChannel {
   public:
    DdsTopicChannel() = default;
    ~DdsTopicChannel() = default;

    void SetTopic(DdsTopicPtr topic) { topic_ = std::move(topic); }

    void SetWriter(const DdsPublisherPtr& publisher, const DataWriterQos& qos) {
        auto* raw_writer = publisher->create_datawriter(topic_.get(), qos);
        if (raw_writer == nullptr) {
            fmt::print(stderr, "Failed to create writer.\n");
            return;
        }
        writer_ = DdsWriterPtr(raw_writer, [](DdsWriter*) {});
    }

    template <typename T = MSG, typename = not_dynamic_data_t<T>>
    void SetReader(const DdsSubscriberPtr& subscriber, const DataReaderQos& qos, const DdsReaderCallback& cb) {
        listener_ = std::make_shared<DdsReaderListener<MSG>>();
        listener_->SetCallback(cb);
        reader_ = DdsReaderPtr(subscriber->create_datareader(topic_.get(), qos, listener_.get()), [](DdsReader*) {});
        if (reader_ == nullptr) {
            fmt::print(stderr, "Failed to create reader.\n");
            return;
        }
    }

    template <typename T = MSG, typename = is_dynamic_data_t<T>>
    void SetReader(const DdsSubscriberPtr& subscriber, const DataReaderQos& qos,
                   DdsDynamicTypeBuilder::_ref_type type_builder, const DdsReaderCallback& cb) {
        listener_ = std::make_shared<DdsReaderListener<MSG>>();
        listener_->SetCallback(cb);
        listener_->SetDynamicTypeBuilder(type_builder);
        reader_ = DdsReaderPtr(subscriber->create_datareader(topic_.get(), qos, listener_.get()), [](DdsReader*) {});
        if (reader_ == nullptr) {
            fmt::print(stderr, "Failed to create reader.\n");
            return;
        }
    }

    DdsWriterPtr GetWriter() const { return writer_; }

    DdsReaderPtr GetReader() const { return reader_; }

    bool Write(void* msg) {
        // const MSG *const_msg_ptr = &msg;
        // MSG *non_const_msg_ptr = const_cast<MSG *>(const_msg_ptr);
        // return writer_->write(static_cast<void *>(&non_const_msg_ptr));
        return writer_->write(msg) == fdds::dds::RETCODE_OK;
    }

   private:
    DdsWriterPtr writer_;
    DdsReaderPtr reader_;
    DdsTopicPtr topic_;
    DdsReaderListenerPtr<MSG> listener_;
};

template <typename MSG>
using DdsTopicChannelPtr = std::shared_ptr<DdsTopicChannel<MSG>>;

}  // namespace jsr::common::dds