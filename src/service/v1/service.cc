#include "service/v1/service.hh"

#include <iostream>

#include "absl/time/time.h"
#include "service/v1/conversions.hh"

namespace aur::v1 {

namespace {

template <typename RequestT>
void LogRequest(std::string_view method, const RequestT* request) {
  std::cout << absl::FormatTime("%Y-%m-%d %H:%M:%S", absl::Now(),
                                absl::LocalTimeZone())
            << " [v1." << method << "] " << request->ShortDebugString() << '\n';
}

}  // namespace

grpc::Status AurService::Lookup(grpc::ServerContext*,
                                const LookupRequest* request,
                                LookupResponse* response) {
  LogRequest(__func__, request);
  aur_internal::LookupResponse impl_response;
  auto status = impl_->Lookup(ToInternalRequest(*request), &impl_response);
  if (status.ok()) {
    *response = ToV1Response(std::move(impl_response));
  }

  return status;
}

grpc::Status AurService::Search(grpc::ServerContext*,
                                const SearchRequest* request,
                                SearchResponse* response) {
  LogRequest(__func__, request);
  aur_internal::SearchResponse impl_response;
  auto status = impl_->Search(ToInternalRequest(*request), &impl_response);
  if (status.ok()) {
    *response = ToV1Response(std::move(impl_response));
  }

  return status;
}

grpc::Status AurService::Resolve(grpc::ServerContext*,
                                 const ResolveRequest* request,
                                 ResolveResponse* response) {
  LogRequest(__func__, request);
  aur_internal::ResolveResponse impl_response;
  auto status = impl_->Resolve(ToInternalRequest(*request), &impl_response);
  if (status.ok()) {
    *response = ToV1Response(std::move(impl_response));
  }

  return status;
}

}  // namespace aur::v1
