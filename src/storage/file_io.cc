#include "storage/file_io.hh"

#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

namespace aur_storage {

bool ReadFileToString(const std::string& path, std::string* out) {
  FILE* f;

  f = fopen(path.c_str(), "r");
  if (f == nullptr) {
    return false;
  }

  struct stat st;
  fstat(fileno(f), &st);

  out->resize(st.st_size);

  off_t bytes_read = 0;
  while (bytes_read < st.st_size) {
    size_t n = fread(&out->at(bytes_read), 1, out->size(), f);
    if (n <= 0) {
      break;
    }

    bytes_read += n;
  }

  fclose(f);

  return bytes_read == st.st_size;
}

bool WriteStringToFile(const std::string& path, std::string_view contents) {
  FILE* f;

  f = fopen(path.c_str(), "w");
  if (f == nullptr) {
    return false;
  }

  while (!contents.empty()) {
    auto n = fwrite(contents.data(), 1, contents.size(), f);
    if (n <= 0) {
      break;
    }

    contents.remove_prefix(n);
  }

  const bool success = fclose(f) == 0 && contents.empty();
  if (!success) {
    unlink(path.c_str());
  }

  return success;
}

bool GetBinaryProto(const std::string& path,
                    google::protobuf::Message* message) {
  std::string contents;
  if (!ReadFileToString(path, &contents)) {
    return false;
  }
  return message->ParsePartialFromString(contents);
}

bool SetBinaryProto(const std::string& path,
                    const google::protobuf::Message& message) {
  return WriteStringToFile(path, message.SerializeAsString());
}

}  // namespace aur_storage
