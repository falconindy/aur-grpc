#include "storage/filesystem_storage.hh"

#include <glob.h>
#include <stdio.h>
#include <sys/stat.h>

#include <filesystem>
#include <string_view>

#include "absl/strings/str_cat.h"
#include "absl/types/span.h"
#include "storage/file_io.hh"

namespace aur_storage {

class Glob {
 public:
  explicit Glob(std::string_view pattern) {
    int r =
        glob(std::string(pattern).c_str(), 0, nullptr, &glob_);
    if (r == 0) {
      results_ = absl::MakeSpan(glob_.gl_pathv, glob_.gl_pathc);
    }

    glob_ok_ = r == 0 || r == GLOB_NOMATCH;
  }

  ~Glob() {
    if (glob_ok_) {
      globfree(&glob_);
    }
  }

  using iterator = absl::Span<char*>::iterator;
  iterator begin() { return results_.begin(); }
  iterator end() { return results_.end(); }

 private:
  glob_t glob_;
  bool glob_ok_;
  absl::Span<char*> results_;
};

bool FilesystemStorage::Get(const std::string& key, std::string* value) const {
  // Actively reject any requests that look like filesystem traversal.
  if (key.find('/') != key.npos) {
    return false;
  }

  return ReadFileToString(root_ / key, value);
}

std::vector<std::string> FilesystemStorage::List(
    const std::string& pattern) const {
  std::vector<std::string> results;
  for (const auto& result : Glob(absl::StrCat(std::string(root_ / pattern)))) {
    results.push_back(std::filesystem::path(result).filename());
  }
  return results;
}

}  // namespace aur_storage
