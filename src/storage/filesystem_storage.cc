#include "storage/filesystem_storage.hh"

#include <filesystem>

#include "storage/file_io.hh"

namespace fs = std::filesystem;

namespace aur_storage {

bool FilesystemStorage::Get(const std::string& key, std::string* value) const {
  // Actively reject any requests that look like filesystem traversal.
  if (key.find('/') != key.npos) {
    return false;
  }

  return ReadFileToString(root_ / key, value);
}

std::vector<std::string> FilesystemStorage::List() const {
  std::vector<std::string> results;

  for (const auto& p : fs::directory_iterator(root_)) {
    if (!p.is_regular_file()) {
      continue;
    }

    results.push_back(p.path().filename());
  }

  return results;
}

}  // namespace aur_storage
