#include "client/client.hh"

namespace aur::v1 {

namespace {

template <typename RequestT, typename ResponseT>
void Invoke(Aur::Stub* service,
            grpc::Status (Aur::Stub::*method)(grpc::ClientContext*,
                                              const RequestT&, ResponseT*),
            const RequestT& request) {
  grpc::ClientContext ctx;
  ResponseT response;
  auto status = (service->*method)(&ctx, request, &response);
  if (status.ok()) {
    printf("%s\n", response.DebugString().c_str());
  } else {
    printf("rpc failed: %s\n", status.error_message().c_str());
  }
}

}  // namespace

void AurClient::Lookup(const std::vector<std::string>& names,
                       const CallOptions& call_options) {
  LookupRequest request;
  request.set_lookup_by(call_options.lookup_by);
  if (call_options.field_mask.paths_size() > 0) {
    *request.mutable_options()->mutable_package_field_mask() =
        call_options.field_mask;
  }

  for (const auto& n : names) {
    request.add_names(n);
  }

  Invoke(stub_.get(), &Aur::Stub::Lookup, request);
}

void AurClient::Search(const std::vector<std::string>& names,
                       const CallOptions& call_options) {
  SearchRequest request;
  request.set_search_by(call_options.search_by);
  request.set_search_logic(call_options.search_logic);
  if (call_options.field_mask.paths_size() > 0) {
    *request.mutable_options()->mutable_package_field_mask() =
        call_options.field_mask;
  }

  for (const auto& n : names) {
    request.add_terms(n);
  }

  Invoke(stub_.get(), &Aur::Stub::Search, request);
}

void AurClient::Resolve(const std::vector<std::string>& names,
                        const AurClient::CallOptions& call_options) {
  ResolveRequest request;
  if (call_options.field_mask.paths_size() > 0) {
    *request.mutable_options()->mutable_package_field_mask() =
        call_options.field_mask;
  }

  for (const auto& n : names) {
    request.add_depstrings(n);
  }

  Invoke(stub_.get(), &Aur::Stub::Resolve, request);
}

}  // namespace aur::v1
