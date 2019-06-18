#include "service/internal/package_index.hh"

#include "absl/strings/ascii.h"
#include "parsed_dependency.hh"

namespace aur_internal {

const std::vector<const Package*>& PackageIndex::Get(
    const std::string& key) const {
  auto iter = index_.find(absl::AsciiStrToLower(key));
  if (iter != index_.end()) {
    return iter->second;
  }

  return empty_value_;
}

void PackageIndex::Builder::AddEntry(const std::string& key,
                                     const Package* value) {
  auto iter = index_.find(key);
  if (iter == index_.end()) {
    index_.emplace_hint(iter, key, std::vector<const Package*>{value});
  } else {
    iter->second.push_back(value);
  }
}

void PackageIndex::Builder::IndexPackage(const Package& package) {
  for (const std::string& item : getter_(package)) {
    AddEntry(absl::AsciiStrToLower(item), &package);
  }
}

// static
PackageIndex PackageIndex::Create(const std::vector<Package>& packages,
                                  const std::string& index_name,
                                  SecondaryValueFn fn) {
  Builder builder(std::move(fn));
  for (const auto& p : packages) {
    builder.IndexPackage(p);
  }
  return builder.Build(index_name);
}

// static
PackageIndex::SecondaryValueFn PackageIndex::DepstringFieldIndexingAdapter(
    const google::protobuf::RepeatedPtrField<std::string>& (
        Package::*depstring_field)() const) {
  return [=](const Package& p) {
    google::protobuf::RepeatedPtrField<std::string> field;

    for (const auto& depstring : (p.*depstring_field)()) {
      *field.Add() = ParsedDependency(depstring).name();
    }

    return field;
  };
}

// static
PackageIndex::SecondaryValueFn PackageIndex::ScalarFieldIndexingAdapter(
    const std::string& (Package::*scalar_field)() const) {
  return [=](const Package& p) {
    google::protobuf::RepeatedPtrField<std::string> field;
    *field.Add() = (p.*scalar_field)();
    return field;
  };
}

// static
PackageIndex::SecondaryValueFn PackageIndex::RepeatedFieldIndexingAdapter(
    const google::protobuf::RepeatedPtrField<std::string>& (
        Package::*repeated_field)() const) {
  return [=](const Package& p) { return (p.*repeated_field)(); };
}

}  // namespace aur_internal
