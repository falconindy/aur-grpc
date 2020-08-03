#pragma once

#include <string>
#include <vector>

namespace aur_storage {

class Storage {
 public:
  virtual ~Storage() {}

  virtual bool Get(const std::string& key, std::string* value) const = 0;

  virtual std::vector<std::string> List() const = 0;
};

}  // namespace aur_storage
