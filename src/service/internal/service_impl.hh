#pragma once

#include <string>
#include <vector>

#include "absl/base/thread_annotations.h"
#include "absl/container/flat_hash_set.h"
#include "absl/synchronization/mutex.h"
#include "aur_internal.pb.h"
#include "grpcpp/grpcpp.h"
#include "service/internal/package_index.hh"
#include "storage/storage.hh"

namespace aur_internal {

// ServiceImpl is the version-agnotsic implementation of the AUR. At
// construction, storage contents are read into memory and adds indexing to
// optimize lookups. Objects are thread-safe and the underlying cached data can
// be reloaded during runtime without interruptions to serving.
class ServiceImpl final {
 public:
  explicit ServiceImpl(const aur_storage::Storage* storage);

  ServiceImpl(ServiceImpl&&) = delete;
  ServiceImpl& operator=(ServiceImpl&&) = delete;

  ServiceImpl(const ServiceImpl&) = delete;
  ServiceImpl& operator=(const ServiceImpl&) = delete;

  grpc::Status Lookup(const LookupRequest& request,
                      LookupResponse* response) const;
  grpc::Status Search(const SearchRequest& request,
                      SearchResponse* response) const;
  grpc::Status Resolve(const ResolveRequest& request,
                       ResolveResponse* response) const;

  void Reload();

 private:
  class InMemoryDB final {
   public:
    explicit InMemoryDB(const aur_storage::Storage* storage) {
      LoadPackages(storage);
      BuildIndexes();
    }

    InMemoryDB(InMemoryDB&&) = default;
    InMemoryDB& operator=(InMemoryDB&&) = default;

    InMemoryDB(const InMemoryDB&) = delete;
    InMemoryDB& operator=(const InMemoryDB&) = delete;

    const std::vector<Package>& packages() const { return packages_; }
    const PackageIndex& idx_pkgname() const { return idx_pkgname_; }
    const PackageIndex& idx_pkgbase() const { return idx_pkgbase_; }
    const PackageIndex& idx_maintainers() const { return idx_maintainers_; }
    const PackageIndex& idx_groups() const { return idx_groups_; }
    const PackageIndex& idx_keywords() const { return idx_keywords_; }
    const PackageIndex& idx_provides() const { return idx_provides_; }
    const PackageIndex& idx_depends() const { return idx_depends_; }
    const PackageIndex& idx_optdepends() const { return idx_optdepends_; }
    const PackageIndex& idx_makedepends() const { return idx_makedepends_; }
    const PackageIndex& idx_checkdepends() const { return idx_checkdepends_; }

   private:
    void LoadPackages(const aur_storage::Storage* storage);
    void BuildIndexes();

    std::vector<Package> packages_;

    PackageIndex idx_pkgname_;
    PackageIndex idx_pkgbase_;
    PackageIndex idx_maintainers_;
    PackageIndex idx_groups_;
    PackageIndex idx_keywords_;
    PackageIndex idx_provides_;
    PackageIndex idx_depends_;
    PackageIndex idx_optdepends_;
    PackageIndex idx_makedepends_;
    PackageIndex idx_checkdepends_;
  };

  const std::shared_ptr<const InMemoryDB> snapshot_db() const;

  using SearchPredicate =
      std::function<bool(const Package&, const std::string&)>;
  static grpc::Status SearchByPredicate(
      const std::shared_ptr<const InMemoryDB>& db,
      const SearchPredicate& predicate, const SearchRequest& request,
      SearchResponse* response);

  static absl::flat_hash_set<const Package*> ResolveProviders(
      const std::shared_ptr<const InMemoryDB>& db,
      const std::string& depstring);

  const aur_storage::Storage* storage_;

  mutable absl::Mutex mutex_;
  std::shared_ptr<const InMemoryDB> db_ ABSL_GUARDED_BY(mutex_);
};

}  // namespace aur_internal
