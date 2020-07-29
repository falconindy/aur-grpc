#pragma once

#include "aur_internal.pb.h"
#include "aur_v1.pb.h"

namespace aur::v1 {

SearchResponse ToV1Response(aur_internal::SearchResponse response);
LookupResponse ToV1Response(aur_internal::LookupResponse response);
ResolveResponse ToV1Response(aur_internal::ResolveResponse response);

aur_internal::SearchRequest ToInternalRequest(const SearchRequest& request);
aur_internal::LookupRequest ToInternalRequest(const LookupRequest& request);
aur_internal::ResolveRequest ToInternalRequest(const ResolveRequest& request);

}  // namespace aur::v1
