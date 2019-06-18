#pragma once

#include <filesystem>
#include <string>

#include "storage/storage.hh"

namespace aur_storage {

class FilesystemStorage : public Storage {
 public:
  explicit FilesystemStorage(std::string root) : root_(std::move(root)) {}

  bool Get(const std::string& key, std::string* value) const override;

  std::vector<std::string> List(const std::string& pattern) const override;

 private:
  std::filesystem::path root_;
};

}  // namespace aur_storage
