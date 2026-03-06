// STD
#include <deque>
#include <mutex>
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
using grpc::ServerReaderWriter;
// proto
using hello::Greeter1;
using hello::HelloReply;
using hello::HelloRequest;

constexpr size_t GREETER_CLIENT_STREAM_MSG_SIZE = 10;
class GreeterClient {
   public:
    using ServiceObject = Greeter1;
    using Wrapper = jsr::grpc::ClientWrapper<ServiceObject, HelloRequest, HelloReply>;
    explicit GreeterClient(std::shared_ptr<Channel> channel) : wrapper_(std::make_unique<Wrapper>(channel)) {}
    GreeterClient(const GreeterClient&) = delete;
    GreeterClient& operator=(const GreeterClient&) = delete;
    virtual ~GreeterClient() = default;

    std::string sayHello(const std::string& user) {
        auto request = HelloRequest{};
        request.set_name(user);
        return wrapper_->rpcCall(request, &ServiceObject::Stub::SayHello).message();
    }

    std::string sayHelloAgain(const std::string& user) {
        auto request = HelloRequest{};
        request.set_name(user);
        return wrapper_->rpcCall(request, &ServiceObject::Stub::SayHelloAgain).message();
    }

    void startChat(std::string& name) {
        chat_flag_.store(true);
        wrapper_->streamCall(
            &ServiceObject::Stub::Chat, chat_flag_,
            [&name] {
                HelloRequest req;
                req.set_name(name);
                return req;
            },
            [this](HelloReply* reply) {
                std::lock_guard lock(stream_mutex_);
                if (stream_messages_.size() >= GREETER_CLIENT_STREAM_MSG_SIZE) {
                    stream_messages_.pop_front();
                }
                stream_messages_.push_back(reply->message());
            });
        return;
    }

    void runChat(const std::vector<std::string>& names) {
        size_t i = 0;
        wrapper_->streamCall(
            &ServiceObject::Stub::Chat, names.size(),
            [&names, &i] {
                HelloRequest req;
                req.set_name(names[i]);
                ++i;
                return req;
            },
            [this](HelloReply* reply) {
                std::lock_guard lock(stream_mutex_);
                if (stream_messages_.size() >= GREETER_CLIENT_STREAM_MSG_SIZE) {
                    stream_messages_.pop_front();
                }
                stream_messages_.push_back(reply->message());
            });
        return;
    }

    std::deque<std::string> getStreamMessages() {
        std::lock_guard lock(stream_mutex_);
        return stream_messages_;
    }

    void clearStreamMessages() {
        std::lock_guard lock(stream_mutex_);
        stream_messages_.clear();
    }

    void stopChat() {
        chat_flag_.store(false);
        return;
    }

   private:
    std::unique_ptr<Wrapper> wrapper_{nullptr};
    std::atomic_bool chat_flag_{true};
    std::mutex stream_mutex_{};
    std::deque<std::string> stream_messages_{};
};

int main() {
    constexpr const char* ADDR = "localhost:50051";
    auto channel = grpc::CreateChannel(ADDR, grpc::InsecureChannelCredentials());
    if (!channel->WaitForConnected(std::chrono::system_clock::now() + std::chrono::seconds(1))) {
        fmt::print(stderr, "[Client] Failed to connect to {}\n", ADDR);
        return -1;
    }
    auto client = GreeterClient(channel);

    fmt::print("Commands:\n");
    fmt::print(" 1 <name>  -> SayHello\n");
    fmt::print(" 2 <name>  -> SayHelloAgain\n");
    fmt::print(" 3 <a b c> -> runChat\n");
    fmt::print(" 4 <name>  -> startChat (continuous)\n");
    fmt::print(" 5         -> stopChat\n");
    fmt::print(" 6 <name>  -> setName\n");
    fmt::print(" q         -> quit\n");

    std::string cmd;
    std::string name;

    while (true) {
        fmt::print("> ");
        if (!(std::cin >> cmd)) {
            break;
        }
        if (cmd == "1") {
            std::cin >> name;
            fmt::print("{}\n", client.sayHello(name));
        } else if (cmd == "2") {
            std::cin >> name;
            fmt::print("{}\n", client.sayHelloAgain(name));
        } else if (cmd == "3") {
            std::vector<std::string> names;
            while (std::cin.peek() != '\n' && std::cin >> name) {
                names.push_back(name);
            }
            client.runChat(names);
        } else if (cmd == "4") {
            std::cin >> name;
            std::thread([&client, &name] { client.startChat(name); }).detach();
            fmt::print("Chat started\n");
        } else if (cmd == "5") {
            client.stopChat();
        } else if (cmd == "6") {
            std::string new_name;
            std::cout << "Input new name: ";
            std::cin >> new_name;
            name = new_name;
        } else if (cmd == "q") {
            break;
        }
        auto msgs = client.getStreamMessages();
        if (!msgs.empty()) {
            fmt::print("---- stream messages ----\n");
            for (auto& m : msgs) {
                fmt::print("{}\n", m);
            }
            client.clearStreamMessages();
        }
    }
}