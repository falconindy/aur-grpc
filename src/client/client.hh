#pragma once

#include <grpcpp/grpcpp.h>

#include <memory>
#include <string>
#include <vector>

#include "aur_v1.grpc.pb.h"
#include "aur_v1.pb.h"

namespace aur::v1 {

class AurClient final {
 public:
  explicit AurClient(std::shared_ptr<grpc::Channel> channel)
      : stub_(aur::v1::Aur::NewStub(channel)) {}

  struct CallOptions {
    SearchRequest::SearchBy search_by = SearchRequest::SEARCHBY_UNKNOWN;
    SearchRequest::SearchLogic search_logic = SearchRequest::SEARCHLOGIC_UNKNOWN;

    LookupRequest::LookupBy lookup_by = LookupRequest::LOOKUPBY_UNKNOWN;

    google::protobuf::FieldMask field_mask;
  };

  void Lookup(const std::vector<std::string>& args, const CallOptions& call_options);

  void Search(const std::vector<std::string>& args, const CallOptions& call_options);

  void Resolve(const std::vector<std::string>& args, const CallOptions& call_options);

 private:
  std::unique_ptr<aur::v1::Aur::Stub> stub_;
};

}  // namespace aur
