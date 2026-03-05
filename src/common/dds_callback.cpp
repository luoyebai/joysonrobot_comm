#include "jsrcomm/common/dds/dds_callback.hpp"

namespace jsr::common::dds {
bool DdsReaderCallback::hasMessageHandler() const {
    return handler_ == nullptr;
}
void DdsReaderCallback::onDataAvailable(const void* data) {
    if (data == nullptr) {
        return;
    }
    handler_(data);
}
}  // namespace jsr::common::dds
