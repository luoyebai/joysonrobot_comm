#pragma once

// STD
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
// GRPC
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/support/async_stream.h>
#include <grpcpp/support/async_unary_call.h>
#include <grpcpp/support/server_callback.h>

// FMT
#include <fmt/format.h>

namespace jsr::grpc {

constexpr auto GRPC_BUILDER_USE_DEFAULT_CQS = false;
constexpr auto GRPC_BUILDER_DEFAULT_CQS = 4;
constexpr auto GRPC_BUILDER_DEFAULT_POLLERS_MIN = 1;
constexpr auto GRPC_BUILDER_DEFAULT_POLLERS_MAX = 2;

static_assert(GRPC_BUILDER_DEFAULT_POLLERS_MIN <= GRPC_BUILDER_DEFAULT_POLLERS_MAX,
              "GRPC_BUILDER_DEFAULT_POLLERS_MIN must be <= GRPC_BUILDER_DEFAULT_POLLERS_MAX");

template <bool Cond>
struct GrpcPollerEqualWarning;
template <>
struct [[deprecated("GRPC poller MIN == MAX, dynamic scaling disabled.")]] GrpcPollerEqualWarning<true>{};
template <>
struct GrpcPollerEqualWarning<false> {};

template <bool Cond>
struct GrpcUseCoresTooMuchWarning;
template <>
struct [[deprecated("GRPC poller MAX * CQS exceeds 2x CPU cores")]] GrpcUseCoresTooMuchWarning<true>{};
template <>
struct GrpcUseCoresTooMuchWarning<false> {};

enum class GrpcSecurityMode { INSECURE, ALTS, MTLS, TLS };

struct GrpcConfigOptions {
    int cqs = GRPC_BUILDER_DEFAULT_CQS;
    int pollers_min = GRPC_BUILDER_DEFAULT_POLLERS_MIN;
    int pollers_max = GRPC_BUILDER_DEFAULT_POLLERS_MAX;
    GrpcSecurityMode security = GrpcSecurityMode::INSECURE;
    // TLS
    std::string server_key;
    std::string server_cert;
    std::string ca_cert;
};

using ::grpc::Channel;
using ::grpc::ClientContext;
using ::grpc::Server;
using ::grpc::ServerBuilder;
using ::grpc::Status;

// --------------------
// Credentials config |
// --------------------
inline std::string LoadFile(const std::string& path) {
    std::ifstream file(path);
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

inline std::shared_ptr<::grpc::ServerCredentials> CreateServerCredentials(const GrpcConfigOptions& options) {
    switch (options.security) {
        case GrpcSecurityMode::TLS: {
            ::grpc::SslServerCredentialsOptions ssl_opts;
            ::grpc::SslServerCredentialsOptions::PemKeyCertPair keycert;
            keycert.private_key = LoadFile(options.server_key);
            keycert.cert_chain = LoadFile(options.server_cert);
            ssl_opts.pem_key_cert_pairs.push_back(keycert);
            return ::grpc::SslServerCredentials(ssl_opts);
        }
        case GrpcSecurityMode::MTLS: {
            ::grpc::SslServerCredentialsOptions ssl_opts;
            ::grpc::SslServerCredentialsOptions::PemKeyCertPair keycert;
            keycert.private_key = LoadFile(options.server_key);
            keycert.cert_chain = LoadFile(options.server_cert);
            ssl_opts.pem_key_cert_pairs.push_back(keycert);
            ssl_opts.pem_root_certs = LoadFile(options.ca_cert);
            ssl_opts.client_certificate_request = GRPC_SSL_REQUEST_AND_REQUIRE_CLIENT_CERTIFICATE_AND_VERIFY;
            return ::grpc::SslServerCredentials(ssl_opts);
        }
        case GrpcSecurityMode::ALTS: {
            ::grpc::experimental::AltsServerCredentialsOptions opts;
            return ::grpc::experimental::AltsServerCredentials(opts);
        }
        case GrpcSecurityMode::INSECURE:
        default:
            return ::grpc::InsecureServerCredentials();
    }
}

// -------------
// Sync client |
// -------------
template <class T, class Request, class Reply>
class ClientWrapper {
   public:
    explicit ClientWrapper(std::shared_ptr<Channel> channel) : stub_(T::NewStub(channel)) {}

    template <typename Rpc>
    Reply rpcCall(Request request, Rpc rpc, std::function<void(Reply*)> bad_rpc_set = nullptr) {
        Reply reply;
        ClientContext context;
        Status status = std::invoke(rpc, stub_.get(), &context, request, &reply);
        if (status.ok()) {
            return reply;
        }
        if (bad_rpc_set) {
            bad_rpc_set(&reply);
        }
        return reply;
    }

    template <typename Stream>
    void streamCall(Stream s, std::atomic_bool& run_flag, std::function<Request(void)> get_req,
                    std::function<void(Reply*)> take_rep) {
        context_ = std::make_unique<ClientContext>();
        auto stream = std::invoke(s, stub_.get(), context_.get());
        auto* stream_ptr = stream.get();

        std::thread writer([stream_ptr, &run_flag, &get_req]() {
            while (run_flag.load()) {
                auto req = get_req();
                stream_ptr->Write(req);
            }
            stream_ptr->WritesDone();
        });
        Reply reply;
        while (stream->Read(&reply)) {
            take_rep(&reply);
            if (!run_flag.load()) {
                break;
            }
        }
        writer.join();
        stream->Finish();
        context_.reset();
    }

    template <typename Stream>
    void streamCall(Stream s, size_t count, std::function<Request(void)> get_req,
                    std::function<void(Reply*)> take_rep) {
        ClientContext context;
        auto stream = std::invoke(s, stub_.get(), &context);
        auto* stream_ptr = stream.get();

        std::thread writer([stream_ptr, count, &get_req]() {
            size_t count_now = 0;
            while (count_now != count) {
                auto req = get_req();
                stream_ptr->Write(req);
                ++count_now;
            }
            stream_ptr->WritesDone();
        });
        Reply reply;
        while (stream->Read(&reply)) {
            take_rep(&reply);
        }
        writer.join();
        stream->Finish();
    }

    void tryCancel() {
        if (context_) {
            context_->TryCancel();
        }
    }

   private:
    std::unique_ptr<typename T::Stub> stub_;
    std::unique_ptr<ClientContext> context_;
};

// ------------------------
// Server builder options |
// ------------------------
auto GetServerBuilderDefaultOptions() {
    GrpcPollerEqualWarning<GRPC_BUILDER_DEFAULT_POLLERS_MIN == GRPC_BUILDER_DEFAULT_POLLERS_MAX> warn_equal;
    auto options = GrpcConfigOptions();
    if constexpr (!GRPC_BUILDER_USE_DEFAULT_CQS) {
        options.cqs = static_cast<int>(std::thread::hardware_concurrency() / 2);
        GrpcUseCoresTooMuchWarning<(GRPC_BUILDER_DEFAULT_POLLERS_MAX > 2)> warn_cores_too_much;
    }
    return options;
}

// -------------
// Sync Server |
// -------------
template <class T>
void RunServer(uint16_t port, GrpcConfigOptions options = GetServerBuilderDefaultOptions()) {
    std::string server_address = fmt::format("0.0.0.0:{}", port);
    ::grpc::EnableDefaultHealthCheckService(true);
    ::grpc::reflection::InitProtoReflectionServerBuilderPlugin();
    ServerBuilder builder;
    T service;
    // Listen on the given address without any authentication mechanism.
    builder.AddListeningPort(server_address, CreateServerCredentials(options));
    // Register "service" as the instance through which we'll communicate with
    // clients. In this case it corresponds to an *synchronous* service.
    builder.RegisterService(&service);
    builder.SetSyncServerOption(::grpc::ServerBuilder::SyncServerOption::NUM_CQS, options.cqs);
    builder.SetSyncServerOption(::grpc::ServerBuilder::SyncServerOption::MIN_POLLERS, options.pollers_min);
    builder.SetSyncServerOption(::grpc::ServerBuilder::SyncServerOption::MAX_POLLERS, options.pollers_max);
    // Finally assemble the server.
    std::unique_ptr<Server> server(builder.BuildAndStart());

    // Wait for the server to shutdown. Note that some other thread must be
    // responsible for shutting down the server for this call to ever return.
    server->Wait();
}

template <class T>
std::unique_ptr<Server> CreateServer(uint16_t port, T& service,
                                     GrpcConfigOptions options = GetServerBuilderDefaultOptions()) {
    std::string server_address = fmt::format("0.0.0.0:{}", port);
    ::grpc::EnableDefaultHealthCheckService(true);
    ::grpc::reflection::InitProtoReflectionServerBuilderPlugin();
    ServerBuilder builder;
    builder.AddListeningPort(server_address, CreateServerCredentials(options));
    builder.RegisterService(&service);
    builder.SetSyncServerOption(::grpc::ServerBuilder::SyncServerOption::NUM_CQS, options.cqs);
    builder.SetSyncServerOption(::grpc::ServerBuilder::SyncServerOption::MIN_POLLERS, options.pollers_min);
    builder.SetSyncServerOption(::grpc::ServerBuilder::SyncServerOption::MAX_POLLERS, options.pollers_max);
    return builder.BuildAndStart();
}

template <class... Services>
std::unique_ptr<Server> CreateServers(uint16_t port, GrpcConfigOptions options, Services&... services) {
    std::string server_address = fmt::format("0.0.0.0:{}", port);
    ::grpc::EnableDefaultHealthCheckService(true);
    ::grpc::reflection::InitProtoReflectionServerBuilderPlugin();
    ::grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, CreateServerCredentials(options));
    (builder.RegisterService(&services), ...);
    builder.SetSyncServerOption(::grpc::ServerBuilder::SyncServerOption::NUM_CQS, options.cqs);
    builder.SetSyncServerOption(::grpc::ServerBuilder::SyncServerOption::MIN_POLLERS, options.pollers_min);
    builder.SetSyncServerOption(::grpc::ServerBuilder::SyncServerOption::MAX_POLLERS, options.pollers_max);
    return builder.BuildAndStart();
}

template <class... Services>
std::unique_ptr<Server> CreateServers(uint16_t port, Services&... services) {
    return std::move(CreateServers(port, GetServerBuilderDefaultOptions(), services...));
}

// ---------------------------------------
// TODO(luoyebai): Async Server & Client |
// ---------------------------------------

// struct AsyncServer {
//     std::unique_ptr<Server> server;
//     std::vector<std::unique_ptr<::grpc::ServerCompletionQueue>> cqs;
// };

}  // namespace jsr::grpc
