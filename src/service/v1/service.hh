#pragma once

#include "aur_v1.grpc.pb.h"
#include "aur_v1.pb.h"
#include "service/internal/service_impl.hh"

namespace aur::v1 {

class AurService final : public Aur::Service {
 public:
  explicit AurService(const aur_internal::ServiceImpl* impl) : impl_(impl) {}

  AurService(const AurService&) = delete;
  AurService& operator=(const AurService&) = delete;

  AurService(AurService&&) = delete;
  AurService& operator=(AurService&&) = delete;

 private:
  grpc::Status Lookup(grpc::ServerContext* ctx, const LookupRequest* request,
                      LookupResponse* response) override;

  grpc::Status Search(grpc::ServerContext* ctx, const SearchRequest* request,
                      SearchResponse* response) override;

  grpc::Status Resolve(grpc::ServerContext* ctx, const ResolveRequest* request,
                       ResolveResponse* response) override;

  const aur_internal::ServiceImpl* impl_;
};

}  // namespace aur::v1
