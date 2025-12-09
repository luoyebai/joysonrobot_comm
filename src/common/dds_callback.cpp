#include "common/dds/dds_callback.hpp"

namespace jsr::common::dds {
bool DdsReaderCallback::HasMessageHandler() const {
    return handler_ == nullptr;
}
void DdsReaderCallback::OnDataAvailable(const void* data) {
    if (data == nullptr) {
        return;
    }
    handler_(data);
}
}  // namespace jsr::common::dds
