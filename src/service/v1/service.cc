#include "service/v1/service.hh"

#include "service/v1/conversions.hh"

namespace aur::v1 {

grpc::Status AurService::Lookup(grpc::ServerContext*,
                                const LookupRequest* request,
                                LookupResponse* response) {
  aur_internal::LookupResponse impl_response;
  auto status = impl_->Lookup(ToInternal(*request), &impl_response);
  if (status.ok()) {
    *response = ToV1Response(std::move(impl_response));
  }

  return status;
}

grpc::Status AurService::Search(grpc::ServerContext*,
                                const SearchRequest* request,
                                SearchResponse* response) {
  aur_internal::SearchResponse impl_response;
  auto status = impl_->Search(ToInternal(*request), &impl_response);
  if (status.ok()) {
    *response = ToV1Response(std::move(impl_response));
  }

  return status;
}

grpc::Status AurService::Resolve(grpc::ServerContext*,
                                 const ResolveRequest* request,
                                 ResolveResponse* response) {
  aur_internal::ResolveResponse impl_response;
  auto status = impl_->Resolve(ToInternal(*request), &impl_response);
  if (status.ok()) {
    *response = ToV1Response(std::move(impl_response));
  }

  return status;
}

}  // namespace aur::v1
