#pragma once

#include "absl/container/flat_hash_map.h"
#include "storage/storage.hh"

namespace aur_storage {

class InMemoryStorage : public Storage {
 public:
  InMemoryStorage() {}

  bool Get(const std::string& key, std::string* value) const override;
  std::vector<std::string> List(const std::string& pattern) const override;

  void Add(const std::string& key, const std::string& value);

 private:
  absl::flat_hash_map<std::string, std::string> storage_;
};

}  // namespace aur_storage
