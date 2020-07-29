#include "service/v1/conversions.hh"

#include "google/protobuf/repeated_field.h"
#include "google/protobuf/util/field_mask_util.h"

namespace proto = google::protobuf;

namespace aur::v1 {

namespace {

Package ToV1Package(aur_internal::Package package) {
  Package v1;

  v1.set_allocated_name(package.release_name());
  v1.set_allocated_pkgbase(package.release_pkgbase());
  v1.set_allocated_pkgver(package.release_pkgver());
  v1.set_allocated_description(package.release_description());
  v1.set_allocated_url(package.release_url());
  v1.set_votes(package.votes());
  v1.set_popularity(package.popularity());
  v1.mutable_architectures()->Swap(package.mutable_architectures());
  v1.mutable_maintainers()->Swap(package.mutable_maintainers());
  v1.mutable_conflicts()->Swap(package.mutable_conflicts());
  v1.mutable_groups()->Swap(package.mutable_groups());
  v1.mutable_keywords()->Swap(package.mutable_keywords());
  v1.mutable_licenses()->Swap(package.mutable_licenses());
  v1.mutable_optdepends()->Swap(package.mutable_optdepends());
  v1.mutable_provides()->Swap(package.mutable_provides());
  v1.mutable_replaces()->Swap(package.mutable_replaces());
  v1.mutable_depends()->Swap(package.mutable_depends());
  v1.mutable_makedepends()->Swap(package.mutable_makedepends());
  v1.mutable_checkdepends()->Swap(package.mutable_checkdepends());
  v1.set_out_of_date(package.out_of_date());
  v1.set_submitted(package.submitted());
  v1.set_modified(package.modified());

  return v1;
}

proto::RepeatedPtrField<Package> ToV1Packages(
    proto::RepeatedPtrField<aur_internal::Package>* packages) {
  proto::RepeatedPtrField<Package> v1;

  v1.Reserve(packages->size());
  for (int i = 0; i < packages->size(); ++i) {
    *v1.Add() = ToV1Package(std::move(packages->at(i)));
  }

  return v1;
}

}  // namespace

SearchResponse ToV1Response(aur_internal::SearchResponse response) {
  SearchResponse v1;

  *v1.mutable_packages() = ToV1Packages(response.mutable_packages());

  return v1;
}

LookupResponse ToV1Response(aur_internal::LookupResponse response) {
  LookupResponse v1;

  *v1.mutable_packages() = ToV1Packages(response.mutable_packages());
  v1.mutable_not_found_names()->Swap(response.mutable_not_found_names());

  return v1;
}

ResolveResponse ToV1Response(aur_internal::ResolveResponse response) {
  ResolveResponse v1;

  v1.mutable_resolved_packages()->Reserve(response.resolved_packages_size());
  for (int i = 0; i < response.resolved_packages_size(); ++i) {
    auto* internal_resolved = response.mutable_resolved_packages(i);
    auto* v1_resolved = v1.mutable_resolved_packages()->Add();

    v1_resolved->set_allocated_depstring(
        internal_resolved->release_depstring());
    *v1_resolved->mutable_providers() =
        ToV1Packages(internal_resolved->mutable_providers());
  }

  return v1;
}

aur_internal::SearchRequest ToInternalRequest(const SearchRequest& request) {
  aur_internal::SearchRequest internal;

  if (request.options().has_package_field_mask()) {
    *internal.mutable_options()->mutable_package_field_mask() =
        request.options().package_field_mask();
  } else {
    auto default_mask =
        internal.mutable_options()->mutable_package_field_mask();
    default_mask->add_paths("name");
  }

  internal.mutable_terms()->Reserve(request.terms_size());
  for (const auto& term : request.terms()) {
    internal.add_terms(term);
  }

  switch (request.search_by()) {
    case SearchRequest::SEARCHBY_UNKNOWN:
    case SearchRequest::SEARCHBY_NAME_DESC:
      internal.set_search_by(aur_internal::SearchRequest::SEARCHBY_NAME_DESC);
      break;
    case SearchRequest::SEARCHBY_NAME:
      internal.set_search_by(aur_internal::SearchRequest::SEARCHBY_NAME);
      break;
    default:
      // idk, return an error?
      internal.set_search_by(aur_internal::SearchRequest::SEARCHBY_UNKNOWN);
      break;
  }

  switch (request.search_logic()) {
    case SearchRequest::SEARCHLOGIC_UNKNOWN:
    case SearchRequest::SEARCHLOGIC_DISJUNCTIVE:
      internal.set_search_logic(
          aur_internal::SearchRequest::SEARCHLOGIC_DISJUNCTIVE);
      break;
    case SearchRequest::SEARCHLOGIC_CONJUNCTIVE:
      internal.set_search_logic(
          aur_internal::SearchRequest::SEARCHLOGIC_CONJUNCTIVE);
      break;
    default:
      // idk, return an error?
      internal.set_search_logic(
          aur_internal::SearchRequest::SEARCHLOGIC_UNKNOWN);
      break;
  }

  return internal;
}

aur_internal::LookupRequest ToInternalRequest(const LookupRequest& request) {
  aur_internal::LookupRequest internal;

  if (request.options().has_package_field_mask()) {
    *internal.mutable_options()->mutable_package_field_mask() =
        request.options().package_field_mask();
  } else {
    *internal.mutable_options()->mutable_package_field_mask() =
        proto::util::FieldMaskUtil::GetFieldMaskForAllFields<Package>();
  }

  internal.mutable_names()->Reserve(request.names_size());
  for (const auto& name : request.names()) {
    internal.add_names(name);
  }

  switch (request.lookup_by()) {
    case LookupRequest::LOOKUPBY_UNKNOWN:
    case LookupRequest::LOOKUPBY_NAME:
      internal.set_lookup_by(aur_internal::LookupRequest::LOOKUPBY_NAME);
      break;
    case LookupRequest::LOOKUPBY_PKGBASE:
      internal.set_lookup_by(aur_internal::LookupRequest::LOOKUPBY_PKGBASE);
      break;
    case LookupRequest::LOOKUPBY_MAINTAINER:
      internal.set_lookup_by(aur_internal::LookupRequest::LOOKUPBY_MAINTAINER);
      break;
    case LookupRequest::LOOKUPBY_DEPENDS:
      internal.set_lookup_by(aur_internal::LookupRequest::LOOKUPBY_DEPENDS);
      break;
    case LookupRequest::LOOKUPBY_MAKEDEPENDS:
      internal.set_lookup_by(aur_internal::LookupRequest::LOOKUPBY_MAKEDEPENDS);
      break;
    case LookupRequest::LOOKUPBY_CHECKDEPENDS:
      internal.set_lookup_by(
          aur_internal::LookupRequest::LOOKUPBY_CHECKDEPENDS);
      break;
    case LookupRequest::LOOKUPBY_OPTDEPENDS:
      internal.set_lookup_by(aur_internal::LookupRequest::LOOKUPBY_OPTDEPENDS);
      break;
    default:
      // idk, return an error?
      internal.set_lookup_by(aur_internal::LookupRequest::LOOKUPBY_UNKNOWN);
      break;
  }

  return internal;
}

aur_internal::ResolveRequest ToInternalRequest(const ResolveRequest& request) {
  aur_internal::ResolveRequest internal;

  if (request.options().has_package_field_mask()) {
    *internal.mutable_options()->mutable_package_field_mask() =
        request.options().package_field_mask();
  } else {
    *internal.mutable_options()->mutable_package_field_mask() =
        proto::util::FieldMaskUtil::GetFieldMaskForAllFields<Package>();
  }

  internal.mutable_depstrings()->Reserve(request.depstrings_size());
  for (const auto& depstring : request.depstrings()) {
    internal.add_depstrings(depstring);
  }

  return internal;
}

}  // namespace aur::v1
