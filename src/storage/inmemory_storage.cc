#include "storage/inmemory_storage.hh"

#include <fnmatch.h>

namespace aur_storage {

bool InMemoryStorage::Get(const std::string& key, std::string* value) const {
  const auto iter = storage_.find(key);
  if (iter == storage_.end()) {
    return false;
  }

  *value = iter->second;
  return true;
}

std::vector<std::string> InMemoryStorage::List(
    const std::string& pattern) const {
  std::vector<std::string> results;

  for (const auto& [key, _] : storage_) {
    if (fnmatch(pattern.c_str(), key.c_str(), 0) == 0) {
      results.push_back(key);
    }
  }

  return results;
}

void InMemoryStorage::Add(const std::string& key, const std::string& value) {
  storage_.emplace(key, value);
}

}  // namespace aur_storage
