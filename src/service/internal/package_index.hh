#pragma once

#include <string>
#include <utility>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "aur_internal.pb.h"
#include "google/protobuf/repeated_field.h"

namespace aur_internal {

// PackageIndex provides an API for accessing Package protos by an arbitrary
// key. Indexes are made space efficient by keeping only pointers to Packages.
// Thus, care must be taken to ensure that the lifetime of a PackageIndex does
// not exceed the lifetime of the backing store that a PackageIndex depends on.
class PackageIndex final {
 public:
  PackageIndex() {}

  using SecondaryValueFn =
      std::function<google::protobuf::RepeatedPtrField<std::string>(
          const Package&)>;

  static SecondaryValueFn DepstringFieldIndexingAdapter(
      const google::protobuf::RepeatedPtrField<std::string>& (
          Package::*depstring_field)() const);

  static SecondaryValueFn ScalarFieldIndexingAdapter(
      const std::string& (Package::*scalar_field)() const);

  static SecondaryValueFn RepeatedFieldIndexingAdapter(
      const google::protobuf::RepeatedPtrField<std::string>& (
          Package::*repeated_field)() const);

  static PackageIndex Create(const std::vector<Package>& packages,
                             const std::string& index_name,
                             SecondaryValueFn fn);

  PackageIndex(PackageIndex&&) = default;
  PackageIndex& operator=(PackageIndex&&) = default;

  PackageIndex(const PackageIndex&) = delete;
  PackageIndex& operator=(const PackageIndex&) = delete;

  // Lookup the packages associated with the given key. An empty vector is
  // returned when the key is not found in the index.
  const std::vector<const Package*>& Get(const std::string& key) const;

 private:
  using container_type =
      absl::flat_hash_map<std::string, std::vector<const Package*>>;

  class Builder {
   public:
    explicit Builder(SecondaryValueFn getter) : getter_(std::move(getter)) {}

    Builder(Builder&&) = delete;
    Builder& operator=(Builder&&) = delete;

    Builder(const Builder&) = delete;
    Builder& operator=(const Builder&) = delete;

    void IndexPackage(const Package& package);

    PackageIndex Build(const std::string& index_name) {
      index_.rehash(0);
      printf("%s index built with %zd terms\n", index_name.c_str(),
             index_.size());
      return PackageIndex(index_name, std::move(index_));
    }

   private:
    void AddEntry(const std::string& key, const Package* value);

    const SecondaryValueFn getter_;
    container_type index_;
  };

  // Private constructor. PackageIndex objects must be created through the
  // static Create method.
  explicit PackageIndex(const std::string& index_name, container_type index)
      : name_(index_name), index_(std::move(index)) {}

  inline static const container_type::value_type::second_type empty_value_;

  std::string name_;
  container_type index_;
};

// Helpers for creating SecondaryValueFn objects.
PackageIndex::SecondaryValueFn DepstringFieldIndexingAdapter(
    const google::protobuf::RepeatedPtrField<std::string>& (
        Package::*depstring_field)() const);

PackageIndex::SecondaryValueFn ScalarFieldIndexingAdapter(
    const std::string& (Package::*scalar_field)() const);

PackageIndex::SecondaryValueFn RepeatedFieldIndexingAdapter(
    const google::protobuf::RepeatedPtrField<std::string>& (
        Package::*repeated_field)() const);

}  // namespace aur_internal
