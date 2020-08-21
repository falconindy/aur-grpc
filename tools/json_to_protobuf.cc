#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <filesystem>
#include <string_view>

#include "absl/algorithm/container.h"
#include "aur_internal.pb.h"
#include "google/protobuf/repeated_field.h"
#include "google/protobuf/util/json_util.h"
#include "legacy.pb.h"
#include "storage/file_io.hh"

namespace proto = google::protobuf;

namespace {

aur_internal::Package LegacyToInternalPackage(
    const aur_legacy::Package& legacy) {
  aur_internal::Package package;

  package.set_name(legacy.name());
  package.set_pkgbase(legacy.packagebase());
  package.set_pkgver(legacy.version());
  package.set_description(legacy.description());
  package.set_url(legacy.url());
  package.set_votes(legacy.numvotes());
  package.set_popularity(legacy.popularity());
  package.set_out_of_date(legacy.outofdate());
  package.set_submitted(legacy.firstsubmitted());
  package.set_modified(legacy.lastmodified());

  if (!legacy.maintainer().empty()) {
    package.add_maintainers(legacy.maintainer());
  }

  auto* inserter = proto::RepeatedPtrFieldBackInserter<std::string>;
  absl::c_copy(legacy.depends(), inserter(package.mutable_depends()));
  absl::c_copy(legacy.makedepends(), inserter(package.mutable_makedepends()));
  absl::c_copy(legacy.checkdepends(), inserter(package.mutable_checkdepends()));
  absl::c_copy(legacy.optdepends(), inserter(package.mutable_optdepends()));
  absl::c_copy(legacy.provides(), inserter(package.mutable_provides()));
  absl::c_copy(legacy.replaces(), inserter(package.mutable_replaces()));
  absl::c_copy(legacy.license(), inserter(package.mutable_licenses()));
  absl::c_copy(legacy.conflicts(), inserter(package.mutable_conflicts()));
  absl::c_copy(legacy.groups(), inserter(package.mutable_groups()));
  absl::c_copy(legacy.keywords(), inserter(package.mutable_keywords()));

  return package;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 3) {
    std::cerr << "usage: " << program_invocation_short_name
              << " inputfile dbdir\n";
    return 42;
  }

  std::string json_string;
  if (!aur_storage::ReadFileToString(argv[1], &json_string)) {
    std::cerr << "error: failed to read file to string: " << argv[1] << '\n';
    return 1;
  }

  aur_legacy::Response response;
  const auto status = proto::util::JsonStringToMessage(json_string, &response);
  if (!status.ok()) {
    std::cerr << "JsonStringToMessage failed: " << status.message() << '\n';
    return 1;
  }

  const std::filesystem::path dbroot = argv[2];
  for (const auto& result : response.results()) {
    if (!aur_storage::SetBinaryProto(dbroot / result.name(),
                                     LegacyToInternalPackage(result))) {
      std::cerr << "failed to write result to db\n";
    }
  }

  return 0;
}
