#pragma once

#include <string>
#include <string_view>

#include "google/protobuf/message.h"

namespace aur_storage {

bool ReadFileToString(const std::string& path, std::string* out);
bool WriteStringToFile(const std::string& path, std::string_view contents);

bool GetBinaryProto(const std::string& path,
                    google::protobuf::Message* message);
bool SetBinaryProto(const std::string& path,
                    const google::protobuf::Message& message);

}  // namespace aur_storage
