#pragma once

#include <systemd/sd-event.h>

#include <string>

#include "grpcpp/grpcpp.h"
#include "service/internal/service_impl.hh"
#include "service/v1/service.hh"
#include "storage/filesystem_storage.hh"

namespace aur {

class Server {
 public:
  Server(const std::string& listen_address);
  ~Server();

  void Run();

 private:
  static int HandleSignal(sd_event_source* s, const struct signalfd_siginfo* si,
                          void* userdata);

  std::string listen_address_;
  aur_storage::FilesystemStorage storage_{"db"};
  aur_internal::ServiceImpl service_impl_{&storage_};
  aur::v1::AurService aur_service_v1_{&service_impl_};

  grpc::ServerBuilder builder_;
  std::unique_ptr<grpc::Server> server_;

  sd_event* event_;
  std::vector<sd_event_source*> signal_events_;
};

};  // namespace aur
