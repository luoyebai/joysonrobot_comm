/**
 * @file dds_callback.hpp
 * @author TanJiachun (jiachun.tan@joysonrobot.com)
 * @brief DDS reader callback declare for Joyson Robot Robotics COMM.
 * @version 0.1
 * @date 2025-07-07
 *
 * @copyright Copyright 2025 Joyson Robot, Inc. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * See the LICENSE file or http://www.apache.org/licenses/LICENSE-2.0 for details.
 *
 * Modifications:
 * - Changed namespace to `jsr::common::dds`
 */

#pragma once

// STD
#include <functional>
#include <memory>

namespace jsr::common::dds {

using DdsMessageHandler = std::function<void(const void*)>;

/**
 * @brief DDS reader callback
 *
 */
class DdsReaderCallback {
   public:
    DdsReaderCallback() = default;
    explicit DdsReaderCallback(DdsMessageHandler handler) : handler_(std::move(handler)){};
    DdsReaderCallback(const DdsReaderCallback& other) = default;
    DdsReaderCallback& operator=(const DdsReaderCallback& other) = default;
    ~DdsReaderCallback() = default;

    /**
     * @brief Check if the callback has a message handler
     *
     * @return true
     * @return false
     */
    bool HasMessageHandler() const;

    /**
     * @brief When data is available, call the message handler
     *
     * @param data pointer to the data
     */
    void OnDataAvailable(const void* data);

   private:
    DdsMessageHandler handler_;
};

}  // namespace jsr::common::dds
