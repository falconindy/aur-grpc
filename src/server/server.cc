#include "server/server.hh"

#include "grpcpp/ext/proto_server_reflection_plugin.h"

namespace aur {

Server::Server(const std::string& listen_address)
    : listen_address_(listen_address) {
  grpc::reflection::InitProtoReflectionServerBuilderPlugin();
  builder_.AddListeningPort(listen_address_, grpc::InsecureServerCredentials());
  builder_.RegisterService(&aur_service_v1_);
}

Server::~Server() {
  sd_event_source_unref(sigterm_event_);
  sd_event_source_unref(sighup_event_);
  sd_event_unref(event_);
}

void Server::Run() {
  sigset_t ss{};
  sigaddset(&ss, SIGHUP);
  sigaddset(&ss, SIGTERM);
  sigprocmask(SIG_BLOCK, &ss, nullptr);

  sd_event_default(&event_);

  sd_event_add_signal(event_, &sigterm_event_, SIGTERM, &Server::HandleSignal,
                      this);
  sd_event_add_signal(event_, &sighup_event_, SIGHUP, &Server::HandleSignal,
                      this);

  server_ = builder_.BuildAndStart();
  printf("ready to serve on %s\n", listen_address_.c_str());

  sd_event_loop(event_);
}

// static
int Server::HandleSignal(sd_event_source*, const struct signalfd_siginfo* si,
                         void* userdata) {
  auto server = static_cast<Server*>(userdata);

  switch (si->ssi_signo) {
    case SIGHUP:
      server->service_impl_.Reload();
      break;
    case SIGTERM:
      server->server_->Shutdown();
      sd_event_exit(server->event_, 0);
      break;
  }

  return 0;
}

}  // namespace aur
