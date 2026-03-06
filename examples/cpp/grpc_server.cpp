// FMT
#define FMT_HEADER_ONLY
#include <fmt/core.h>
// GRPC
#include "jsrcomm/common/grpc/grpc_wrapper.hpp"
// PROTO
#include "jsrcomm/proto/hello_world.grpc.pb.h"
#include "jsrcomm/proto/hello_world.pb.h"

// grpc
using grpc::Channel;
using grpc::ClientContext;
using grpc::ServerContext;
using grpc::ServerReaderWriter;
using grpc::Status;
// proto
using hello::Greeter1;
using hello::HelloReply;
using hello::HelloRequest;

class GreeterServiceImpl final : public Greeter1::Service {
    Status SayHello(ServerContext* context, const HelloRequest* request, HelloReply* reply) override {
        auto user_name = request->name();
        reply->set_message("[Greeter1] Hello " + user_name);
        return Status::OK;
    }

    Status SayHelloAgain(ServerContext* context, const HelloRequest* request, HelloReply* reply) override {
        auto user_name = request->name();
        reply->set_message("[Greeter1] Hello again " + user_name);
        return Status::OK;
    }

    Status Chat(ServerContext* context, ServerReaderWriter<HelloReply, HelloRequest>* stream) override {
        auto req = HelloRequest();
        while (stream->Read(&req)) {
            auto reply = HelloReply();
            reply.set_message("[Greeter1] Stream Hello " + req.name());
            stream->Write(reply);
        }
        return Status::OK;
    }
};

int main() {
    constexpr uint16_t PORT = 50051;
    // constexpr size_t WAIT_TIME = 60;  // seconds
    constexpr size_t WAIT_TIME = 0;  // seconds
    if constexpr (WAIT_TIME == 0) {
        fmt::print("[Server] Press any key to exit...\n");
    } else {
        fmt::print("[Server] runtime: {} seconds\n", WAIT_TIME);
    }
    auto service = GreeterServiceImpl();
    auto server = jsr::grpc::CreateServer(PORT, service);
    std::thread([&server, WAIT_TIME] {
        if (WAIT_TIME == 0) {
            getchar();
        } else {
            std::this_thread::sleep_for(std::chrono::seconds(WAIT_TIME));
        }
        server->Shutdown();
    }).detach();
    server->Wait();
    return 0;
}