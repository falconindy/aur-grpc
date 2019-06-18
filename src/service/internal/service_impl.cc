#include "service/internal/service_impl.hh"

#include <fnmatch.h>

#include "absl/algorithm/container.h"
#include "absl/strings/match.h"
#include "absl/time/time.h"
#include "google/protobuf/util/field_mask_util.h"
#include "service/internal/parsed_dependency.hh"

using google::protobuf::RepeatedPtrFieldBackInserter;
using google::protobuf::util::FieldMaskUtil;

namespace aur_internal {

namespace {

template <typename T>
class FieldMaskingBackInsertIterator
    : public std::iterator<std::output_iterator_tag, T> {
 public:
  FieldMaskingBackInsertIterator(
      const google::protobuf::FieldMask& mask,
      google::protobuf::RepeatedPtrField<T>* const mutable_field)
      : mask_(mask), field_(mutable_field) {}
  FieldMaskingBackInsertIterator<T>& operator=(const T& value) {
    FieldMaskUtil::MergeMessageTo(value, mask_, FieldMaskUtil::MergeOptions(),
                                  field_->Add());
    return *this;
  }
  FieldMaskingBackInsertIterator<T>& operator=(const T* const ptr_to_value) {
    FieldMaskUtil::MergeMessageTo(*ptr_to_value, mask_,
                                  FieldMaskUtil::MergeOptions(), field_->Add());
    return *this;
  }
  FieldMaskingBackInsertIterator<T>& operator*() { return *this; }
  FieldMaskingBackInsertIterator<T>& operator++() { return *this; }
  FieldMaskingBackInsertIterator<T>& operator++(int) { return *this; }

 private:
  const google::protobuf::FieldMask& mask_;
  google::protobuf::RepeatedPtrField<T>* field_;
};

template <typename T>
FieldMaskingBackInsertIterator<T> FieldMaskingBackInserter(
    const google::protobuf::FieldMask& mask,
    google::protobuf::RepeatedPtrField<T>* const mutable_field) {
  return FieldMaskingBackInsertIterator<T>(mask, mutable_field);
}

void LookupByIndex(const PackageIndex& index, const LookupRequest& request,
                   LookupResponse* response) {
  absl::flat_hash_set<const Package*> results;
  for (const auto& name : request.names()) {
    const auto pkgs = index.Get(name);
    if (pkgs.empty()) {
      response->add_not_found_names(name);
    } else {
      results.insert(pkgs.begin(), pkgs.end());
    }
  }

  response->mutable_packages()->Reserve(results.size());
  absl::c_copy(results,
               FieldMaskingBackInserter(request.options().package_field_mask(),
                                        response->mutable_packages()));
}

bool PatternMatch(const std::string& pattern, const std::string& subject) {
  return fnmatch(pattern.c_str(), subject.c_str(), FNM_CASEFOLD) == 0;
}

bool SearchOneName(const Package& package, const std::string& term) {
  return PatternMatch(term, package.name());
}

bool SearchOneDesc(const Package& package, const std::string& term) {
  return PatternMatch(term, package.description());
}

bool SearchOneNameDesc(const Package& package, const std::string& term) {
  return SearchOneName(package, term) || SearchOneDesc(package, term);
}

}  // namespace

ServiceImpl::ServiceImpl(const aur_storage::Storage* storage)
    : storage_(storage) {
  Reload();
}

static absl::Mutex reload_mu_{absl::kConstInit};

void ServiceImpl::Reload() {
  std::shared_ptr<const InMemoryDB> db;
  {
    absl::MutexLock l(&reload_mu_);
    db = std::make_shared<const InMemoryDB>(storage_);
  }

  absl::WriterMutexLock l(&mutex_);
  db_ = std::move(db);
}

grpc::Status ServiceImpl::Lookup(const LookupRequest& request,
                                 LookupResponse* response) const {
  const auto db = snapshot_db();

  switch (request.lookup_by()) {
    case LookupRequest::LOOKUPBY_UNKNOWN:
    case LookupRequest::LOOKUPBY_NAME:
      LookupByIndex(db->idx_pkgname(), request, response);
      break;
    case LookupRequest::LOOKUPBY_PKGBASE:
      LookupByIndex(db->idx_pkgbase(), request, response);
      break;
    case LookupRequest::LOOKUPBY_MAINTAINER:
      LookupByIndex(db->idx_maintainers(), request, response);
      break;
    case LookupRequest::LOOKUPBY_CHECKDEPENDS:
      LookupByIndex(db->idx_checkdepends(), request, response);
      break;
    case LookupRequest::LOOKUPBY_DEPENDS:
      LookupByIndex(db->idx_depends(), request, response);
      break;
    case LookupRequest::LOOKUPBY_MAKEDEPENDS:
      LookupByIndex(db->idx_makedepends(), request, response);
      break;
    case LookupRequest::LOOKUPBY_OPTDEPENDS:
      LookupByIndex(db->idx_optdepends(), request, response);
      break;
    default:
      return grpc::Status(
          grpc::StatusCode::UNIMPLEMENTED,
          absl::StrCat("Unimplemented lookup_by kind ",
                       LookupRequest::LookupBy_Name(request.lookup_by())));
  }

  return grpc::Status::OK;
}

// static
grpc::Status ServiceImpl::SearchByPredicate(
    const std::shared_ptr<const InMemoryDB>& db,
    const SearchPredicate& predicate, const SearchRequest& request,
    SearchResponse* response) {
  absl::flat_hash_set<const Package*> results;

  switch (request.search_logic()) {
    case SearchRequest::SEARCHLOGIC_UNKNOWN:
    case SearchRequest::SEARCHLOGIC_DISJUNCTIVE:
      for (const auto& package : db->packages()) {
        if (absl::c_any_of(request.terms(), [&](const std::string& term) {
              return predicate(package, term);
            })) {
          results.emplace(&package);
        }
      }
      break;
    case SearchRequest::SEARCHLOGIC_CONJUNCTIVE:
      for (const auto& package : db->packages()) {
        if (absl::c_all_of(request.terms(), [&](const std::string& term) {
              return predicate(package, term);
            })) {
          results.emplace(&package);
        }
      }
      break;
    default:
      return grpc::Status(grpc::StatusCode::UNIMPLEMENTED,
                          absl::StrCat("Unimplemented search_logic kind ",
                                       SearchRequest::SearchLogic_Name(
                                           request.search_logic())));
  }

  response->mutable_packages()->Reserve(results.size());
  absl::c_copy(results,
               FieldMaskingBackInserter(request.options().package_field_mask(),
                                        response->mutable_packages()));

  return grpc::Status::OK;
}

grpc::Status ServiceImpl::Search(const SearchRequest& request,
                                 SearchResponse* response) const {
  const auto db = snapshot_db();

  switch (request.search_by()) {
    case SearchRequest::SEARCHBY_UNKNOWN:
    case SearchRequest::SEARCHBY_NAME_DESC:
      return SearchByPredicate(db, &SearchOneNameDesc, request, response);
    case SearchRequest::SEARCHBY_NAME:
      return SearchByPredicate(db, &SearchOneName, request, response);
    default:
      return grpc::Status(
          grpc::StatusCode::UNIMPLEMENTED,
          absl::StrCat("Unimplemented search_by kind ",
                       SearchRequest::SearchBy_Name(request.search_by())));
  }

  return grpc::Status::OK;
}

// static
absl::flat_hash_set<const Package*> ServiceImpl::ResolveProviders(
    const std::shared_ptr<const InMemoryDB>& db, const std::string& depstring) {
  const ParsedDependency dep(depstring);
  auto dep_satisfied_by = [&](const Package* p) { return dep.SatisfiedBy(*p); };

  absl::flat_hash_set<const Package*> providers;
  auto resolve_by_idx = [&](const PackageIndex& idx) {
    auto candidates = idx.Get(dep.name());
    absl::c_copy_if(candidates, std::inserter(providers, providers.end()),
                    dep_satisfied_by);
  };

  resolve_by_idx(db->idx_pkgname());
  resolve_by_idx(db->idx_provides());

  return providers;
}

grpc::Status ServiceImpl::Resolve(const ResolveRequest& request,
                                  ResolveResponse* response) const {
  const auto db = snapshot_db();

  response->mutable_resolved_packages()->Reserve(request.depstrings_size());

  for (const auto& depstring : request.depstrings()) {
    auto resolved = response->add_resolved_packages();
    resolved->set_depstring(depstring);

    const auto providers = ResolveProviders(db, depstring);
    resolved->mutable_providers()->Reserve(providers.size());

    absl::c_copy(providers, FieldMaskingBackInserter(
                                request.options().package_field_mask(),
                                resolved->mutable_providers()));
  }

  return grpc::Status::OK;
}

std::shared_ptr<const ServiceImpl::InMemoryDB> ServiceImpl::snapshot_db()
    const {
  absl::ReaderMutexLock l(&mutex_);
  return db_;
}

ServiceImpl::InMemoryDB::InMemoryDB(const aur_storage::Storage* storage) {
  absl::Time start;

  start = absl::Now();
  const std::vector<std::string> names = storage->List("*");
  packages_.reserve(names.size());
  for (const auto& name : names) {
    std::string s;
    if (!storage->Get(name, &s)) {
      // Unlikely
      continue;
    }

    Package p;
    if (!p.ParseFromString(s)) {
      // Unlikely
      continue;
    }

    packages_.push_back(std::move(p));
  }

  auto load_time = absl::Now() - start;

  printf("caching complete in %s. %zd packages loaded.\n",
         absl::FormatDuration(load_time).c_str(), packages_.size());

  start = absl::Now();

  idx_pkgname_ = PackageIndex::Create(
      packages_, "pkgname",
      PackageIndex::ScalarFieldIndexingAdapter(&Package::name));
  idx_pkgbase_ = PackageIndex::Create(
      packages_, "pkgbase",
      PackageIndex::ScalarFieldIndexingAdapter(&Package::pkgbase));
  idx_maintainers_ = PackageIndex::Create(
      packages_, "maintainers",
      PackageIndex::RepeatedFieldIndexingAdapter(&Package::maintainers));
  idx_provides_ = PackageIndex::Create(
      packages_, "provides",
      PackageIndex::DepstringFieldIndexingAdapter(&Package::provides));
  idx_depends_ = PackageIndex::Create(
      packages_, "depends",
      PackageIndex::DepstringFieldIndexingAdapter(&Package::depends));
  idx_optdepends_ = PackageIndex::Create(
      packages_, "optdepends",
      PackageIndex::DepstringFieldIndexingAdapter(&Package::optdepends));
  idx_makedepends_ = PackageIndex::Create(
      packages_, "makedepends",
      PackageIndex::DepstringFieldIndexingAdapter(&Package::makedepends));
  idx_checkdepends_ = PackageIndex::Create(
      packages_, "checkdepends",
      PackageIndex::DepstringFieldIndexingAdapter(&Package::checkdepends));

  printf("index building complete in %s.\n",
         absl::FormatDuration(load_time).c_str());
}

}  // namespace aur_internal
