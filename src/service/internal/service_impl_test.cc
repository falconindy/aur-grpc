#include "service/internal/service_impl.hh"

#include <filesystem>

#include "aur_internal.pb.h"
#include "gmock/gmock.h"
#include "google/protobuf/util/field_mask_util.h"
#include "gtest/gtest.h"
#include "storage/file_io.hh"
#include "storage/filesystem_storage.hh"

namespace fs = std::filesystem;

using aur_internal::LookupRequest;
using aur_internal::LookupResponse;
using aur_internal::Package;
using aur_internal::ResolveRequest;
using aur_internal::ResolveResponse;
using aur_internal::SearchRequest;
using aur_internal::SearchResponse;
using aur_internal::ServiceImpl;
using aur_storage::FilesystemStorage;
using testing::AllOf;
using testing::Property;
using testing::UnorderedElementsAre;
using testing::UnorderedElementsAreArray;

namespace aur_internal {
void PrintTo(const ResolveResponse::ResolvedPackage& p, std::ostream* os) {
  *os << p.ShortDebugString();
}
void PrintTo(const Package& p, std::ostream* os) {
  *os << p.ShortDebugString();
}
}  // namespace aur_internal

namespace {

std::vector<std::string> GetAllSetFieldNames(
    const google::protobuf::Message& message) {
  std::vector<const google::protobuf::FieldDescriptor*> fields;
  message.GetReflection()->ListFields(message, &fields);

  std::vector<std::string> field_names;
  for (const auto* field : fields) {
    field_names.push_back(field->name());
  }

  return field_names;
}

void FillFieldMask(aur_internal::RequestOptions* options,
                   std::vector<std::string> paths) {
  auto mask = options->mutable_package_field_mask();
  for (const auto& p : paths) {
    mask->add_paths(p);
  }
}

class TemporaryDirectory {
 public:
  TemporaryDirectory() {
    Create(testing::UnitTest::GetInstance()->current_test_info()->name());
  }
  ~TemporaryDirectory() { std::filesystem::remove_all(tempdir_); }

  const fs::path& dirpath() const { return tempdir_; }

 private:
  void Create(std::string_view designator) {
    const char* tmpdir = getenv("TMPDIR");
    if (tmpdir == nullptr) {
      tmpdir = "/tmp";
    }

    std::string tmpdir_template =
        absl::StrCat(tmpdir, "/", designator, ".XXXXXX");

    tempdir_ = mkdtemp(tmpdir_template.data());
  }

  std::filesystem::path tempdir_;
};

class ServiceImplTest : public testing::Test {
 protected:
  std::unique_ptr<ServiceImpl> BuildService(
      const std::vector<Package>& packages) {
    for (const auto& p : packages) {
      AddPackage(p);
    }

    return std::make_unique<ServiceImpl>(&storage_);
  }

 private:
  void AddPackage(const Package& p) {
    aur_storage::SetBinaryProto(std::string(tempdir_.dirpath() / p.name()), p);
  }

  TemporaryDirectory tempdir_;
  FilesystemStorage storage_{tempdir_.dirpath()};
};

TEST_F(ServiceImplTest, LookupByName) {
  std::vector<Package> packages;
  {
    auto& p = packages.emplace_back();
    p.set_name("expac-git");
    p.set_description("pacman database extraction utility");
    p.set_pkgbase("expac");
    p.set_pkgver("10.1");
    p.add_provides("expac=10");
    p.add_architectures("x86_64");
  }
  {
    auto& p = packages.emplace_back();
    p.set_name("auracle-git");
    p.set_description("A flexible client for the AUR");
    p.set_pkgbase("auracle");
    p.set_pkgver("0");
  }
  {
    auto& p = packages.emplace_back();
    p.set_name("pkgfile-git");
    p.set_description("a pacman .files metadata explorer");
    p.set_pkgbase("pkgfile");
    p.set_pkgver("32");
  }
  {
    auto& p = packages.emplace_back();
    p.set_name("pacman-git");
    p.set_description("A library-based package manager");
    p.set_pkgbase("pacman");
    p.set_pkgver("6.0.0");
    p.add_provides("pacman=6.0.0");
  }
  {
    auto& p = packages.emplace_back();
    p.set_name("pacman-extraponies-git");
    p.set_description("A library-based package manager with more ponies");
    p.set_pkgbase("pacman");
    p.set_pkgver("6.0.0");
    p.add_provides("pacman=6.0.0");
  }
  auto service = BuildService(packages);

  LookupRequest request;
  LookupResponse response;

  request.set_lookup_by(LookupRequest::LOOKUPBY_NAME);
  request.add_names("expac-git");
  request.add_names("auracle-git");
  request.add_names("notfound");
  FillFieldMask(request.mutable_options(), {"name"});

  auto status = service->Lookup(request, &response);
  ASSERT_TRUE(status.ok()) << status.error_message();

  EXPECT_THAT(response.not_found_names(), UnorderedElementsAre("notfound"));
  EXPECT_THAT(response.packages(),
              UnorderedElementsAre(Property(&Package::name, "expac-git"),
                                   Property(&Package::name, "auracle-git")));
}

TEST_F(ServiceImplTest, LookupByPkgbase) {
  std::vector<Package> packages;
  {
    auto& p = packages.emplace_back();
    p.set_name("pkgfile-git");
    p.set_pkgbase("pkgfile");
  }
  {
    auto& p = packages.emplace_back();
    p.set_name("pkgfile");
    p.set_pkgbase("pacman");
  }
  {
    auto& p = packages.emplace_back();
    p.set_name("pacman-extraponies-git");
    p.set_pkgbase("pacman");
  }
  auto service = BuildService(packages);

  LookupRequest request;
  LookupResponse response;

  request.set_lookup_by(LookupRequest::LOOKUPBY_PKGBASE);
  request.add_names("pkgfile");
  FillFieldMask(request.mutable_options(), {"name"});

  auto status = service->Lookup(request, &response);
  ASSERT_TRUE(status.ok()) << status.error_message();

  EXPECT_THAT(response.packages(),
              UnorderedElementsAre(Property(&Package::name, "pkgfile-git")));
}

TEST_F(ServiceImplTest, LookupByMaintainer) {
  std::vector<Package> packages;
  {
    auto& p = packages.emplace_back();
    p.set_name("pkgfile-git");
    p.add_maintainers("falconindy");
  }
  {
    auto& p = packages.emplace_back();
    p.set_name("pacman-git");
    p.add_maintainers("falconindy");
    p.add_maintainers("dreisner");
  }
  {
    auto& p = packages.emplace_back();
    p.set_name("pacman-extraponies-git");
    p.add_maintainers("dreisner");
  }
  auto service = BuildService(packages);

  LookupRequest request;
  LookupResponse response;

  request.set_lookup_by(LookupRequest::LOOKUPBY_MAINTAINER);
  request.add_names("falconindy");
  FillFieldMask(request.mutable_options(), {"name", "maintainers"});

  auto status = service->Lookup(request, &response);
  ASSERT_TRUE(status.ok()) << status.error_message();

  EXPECT_THAT(
      response.packages(),
      UnorderedElementsAre(
          AllOf(Property(&Package::name, "pkgfile-git"),
                Property(&Package::maintainers,
                         UnorderedElementsAre("falconindy"))),
          AllOf(Property(&Package::name, "pacman-git"),
                Property(&Package::maintainers,
                         UnorderedElementsAre("falconindy", "dreisner")))));
}

TEST_F(ServiceImplTest, LookupByGroup) {
  std::vector<Package> packages;
  {
    auto& p = packages.emplace_back();
    p.set_name("pkgfile-git");
    p.add_maintainers("falconindy");
    p.add_groups("a");
  }
  {
    auto& p = packages.emplace_back();
    p.set_name("pacman-git");
    p.set_pkgbase("pacman");
    p.add_groups("a");
    p.add_groups("b");
  }
  {
    auto& p = packages.emplace_back();
    p.set_name("pacman-extraponies-git");
    p.set_pkgbase("pacman");
    p.add_groups("b");
  }
  auto service = BuildService(packages);

  LookupRequest request;
  LookupResponse response;

  request.set_lookup_by(LookupRequest::LOOKUPBY_GROUP);
  request.add_names("a");
  FillFieldMask(request.mutable_options(), {"name", "groups"});

  auto status = service->Lookup(request, &response);
  ASSERT_TRUE(status.ok()) << status.error_message();

  EXPECT_THAT(
      response.packages(),
      UnorderedElementsAre(
          AllOf(Property(&Package::name, "pkgfile-git"),
                Property(&Package::groups, UnorderedElementsAre("a"))),
          AllOf(Property(&Package::name, "pacman-git"),
                Property(&Package::groups, UnorderedElementsAre("a", "b")))));
}

TEST_F(ServiceImplTest, LookupByKeyword) {
  std::vector<Package> packages;
  {
    auto& p = packages.emplace_back();
    p.set_name("pkgfile-git");
    p.add_keywords("alpm");
  }
  {
    auto& p = packages.emplace_back();
    p.set_name("pacman-git");
    p.add_keywords("alpm");
    p.add_keywords("ponies");
  }
  {
    auto& p = packages.emplace_back();
    p.set_name("pacman-extraponies-git");
    p.add_keywords("ponies");
  }
  auto service = BuildService(packages);

  LookupRequest request;
  LookupResponse response;

  request.set_lookup_by(LookupRequest::LOOKUPBY_KEYWORD);
  request.add_names("alpm");
  FillFieldMask(request.mutable_options(), {"name", "keywords"});

  auto status = service->Lookup(request, &response);
  ASSERT_TRUE(status.ok()) << status.error_message();

  EXPECT_THAT(
      response.packages(),
      UnorderedElementsAre(
          AllOf(Property(&Package::name, "pkgfile-git"),
                Property(&Package::keywords, UnorderedElementsAre("alpm"))),
          AllOf(Property(&Package::name, "pacman-git"),
                Property(&Package::keywords,
                         UnorderedElementsAre("alpm", "ponies")))));
}

TEST_F(ServiceImplTest, LookupByDepends) {
  std::vector<Package> packages;
  {
    auto& p = packages.emplace_back();
    p.set_name("pkgfile-git");
    p.add_depends("alpm");
  }
  {
    auto& p = packages.emplace_back();
    p.set_name("pacman-git");
    p.add_depends("alpm");
    p.add_depends("ponies");
  }
  {
    auto& p = packages.emplace_back();
    p.set_name("pacman-extraponies-git");
    p.add_depends("ponies");
  }
  auto service = BuildService(packages);

  LookupRequest request;
  LookupResponse response;

  request.set_lookup_by(LookupRequest::LOOKUPBY_DEPENDS);
  request.add_names("alpm");
  FillFieldMask(request.mutable_options(), {"name", "depends"});

  auto status = service->Lookup(request, &response);
  ASSERT_TRUE(status.ok()) << status.error_message();

  EXPECT_THAT(
      response.packages(),
      UnorderedElementsAre(
          AllOf(Property(&Package::name, "pkgfile-git"),
                Property(&Package::depends, UnorderedElementsAre("alpm"))),
          AllOf(Property(&Package::name, "pacman-git"),
                Property(&Package::depends,
                         UnorderedElementsAre("alpm", "ponies")))));
}

TEST_F(ServiceImplTest, LookupByMakedepends) {
  std::vector<Package> packages;
  {
    auto& p = packages.emplace_back();
    p.set_name("pkgfile-git");
    p.add_makedepends("alpm");
  }
  {
    auto& p = packages.emplace_back();
    p.set_name("pacman-git");
    p.add_makedepends("alpm");
    p.add_makedepends("ponies");
  }
  {
    auto& p = packages.emplace_back();
    p.set_name("pacman-extraponies-git");
    p.add_makedepends("ponies");
  }
  auto service = BuildService(packages);

  LookupRequest request;
  LookupResponse response;

  request.set_lookup_by(LookupRequest::LOOKUPBY_MAKEDEPENDS);
  request.add_names("alpm");
  FillFieldMask(request.mutable_options(), {"name", "makedepends"});

  auto status = service->Lookup(request, &response);
  ASSERT_TRUE(status.ok()) << status.error_message();

  EXPECT_THAT(
      response.packages(),
      UnorderedElementsAre(
          AllOf(Property(&Package::name, "pkgfile-git"),
                Property(&Package::makedepends, UnorderedElementsAre("alpm"))),
          AllOf(Property(&Package::name, "pacman-git"),
                Property(&Package::makedepends,
                         UnorderedElementsAre("alpm", "ponies")))));
}

TEST_F(ServiceImplTest, LookupByCheckdepends) {
  std::vector<Package> packages;
  {
    auto& p = packages.emplace_back();
    p.set_name("pkgfile-git");
    p.add_checkdepends("alpm");
  }
  {
    auto& p = packages.emplace_back();
    p.set_name("pacman-git");
    p.add_checkdepends("alpm");
    p.add_checkdepends("ponies");
  }
  {
    auto& p = packages.emplace_back();
    p.set_name("pacman-extraponies-git");
    p.add_checkdepends("ponies");
  }
  auto service = BuildService(packages);

  LookupRequest request;
  LookupResponse response;

  request.set_lookup_by(LookupRequest::LOOKUPBY_CHECKDEPENDS);
  request.add_names("alpm");
  FillFieldMask(request.mutable_options(), {"name", "checkdepends"});

  auto status = service->Lookup(request, &response);
  ASSERT_TRUE(status.ok()) << status.error_message();

  EXPECT_THAT(
      response.packages(),
      UnorderedElementsAre(
          AllOf(Property(&Package::name, "pkgfile-git"),
                Property(&Package::checkdepends, UnorderedElementsAre("alpm"))),
          AllOf(Property(&Package::name, "pacman-git"),
                Property(&Package::checkdepends,
                         UnorderedElementsAre("alpm", "ponies")))));
}

TEST_F(ServiceImplTest, LookupByOptdepends) {
  std::vector<Package> packages;
  {
    auto& p = packages.emplace_back();
    p.set_name("pkgfile-git");
    p.add_optdepends("alpm");
  }
  {
    auto& p = packages.emplace_back();
    p.set_name("pacman-git");
    p.add_optdepends("alpm");
    p.add_optdepends("ponies");
  }
  {
    auto& p = packages.emplace_back();
    p.set_name("pacman-extraponies-git");
    p.add_optdepends("ponies");
  }
  auto service = BuildService(packages);

  LookupRequest request;
  LookupResponse response;

  request.set_lookup_by(LookupRequest::LOOKUPBY_OPTDEPENDS);
  request.add_names("alpm");
  FillFieldMask(request.mutable_options(), {"name", "optdepends"});

  auto status = service->Lookup(request, &response);
  ASSERT_TRUE(status.ok()) << status.error_message();

  EXPECT_THAT(
      response.packages(),
      UnorderedElementsAre(
          AllOf(Property(&Package::name, "pkgfile-git"),
                Property(&Package::optdepends, UnorderedElementsAre("alpm"))),
          AllOf(Property(&Package::name, "pacman-git"),
                Property(&Package::optdepends,
                         UnorderedElementsAre("alpm", "ponies")))));
}

TEST_F(ServiceImplTest, LookupIsCaseInsensitive) {
  std::vector<Package> packages;
  {
    auto& p = packages.emplace_back();
    p.set_name("expac-git");
    p.set_description("pacman database extraction utility");
    p.set_pkgbase("expac");
    p.set_pkgver("10.1");
    p.add_provides("expac=10");
    p.add_architectures("x86_64");
  }
  {
    auto& p = packages.emplace_back();
    p.set_name("auracle-git");
    p.set_description("A flexible client for the AUR");
    p.set_pkgbase("auracle");
    p.set_pkgver("0");
  }
  {
    auto& p = packages.emplace_back();
    p.set_name("pkgfile-git");
    p.set_description("a pacman .files metadata explorer");
    p.set_pkgbase("pkgfile");
    p.set_pkgver("32");
  }
  {
    auto& p = packages.emplace_back();
    p.set_name("pacman-git");
    p.set_description("A library-based package manager");
    p.set_pkgbase("pacman");
    p.set_pkgver("6.0.0");
    p.add_provides("pacman=6.0.0");
  }
  {
    auto& p = packages.emplace_back();
    p.set_name("pacman-extraponies-git");
    p.set_description("A library-based package manager with more ponies");
    p.set_pkgbase("pacman");
    p.set_pkgver("6.0.0");
    p.add_provides("pacman=6.0.0");
  }
  auto service = BuildService(packages);

  LookupRequest request;
  LookupResponse response;

  request.set_lookup_by(LookupRequest::LOOKUPBY_NAME);
  request.add_names("EXPAC-git");
  request.add_names("auracle-GIT");
  FillFieldMask(request.mutable_options(), {"name"});

  auto status = service->Lookup(request, &response);
  ASSERT_TRUE(status.ok()) << status.error_message();

  EXPECT_THAT(response.packages(),
              UnorderedElementsAre(Property(&Package::name, "expac-git"),
                                   Property(&Package::name, "auracle-git")));
}

TEST_F(ServiceImplTest, LookupByUnknownBy) {
  auto service = BuildService({});

  LookupRequest request;
  LookupResponse response;

  request.set_lookup_by(LookupRequest::LOOKUPBY_UNKNOWN);
  auto status = service->Lookup(request, &response);
  EXPECT_EQ(status.error_code(), grpc::StatusCode::UNIMPLEMENTED);
}

TEST_F(ServiceImplTest, SearchByName) {
  std::vector<Package> packages;
  {
    auto& p = packages.emplace_back();
    p.set_name("expac-git");
    p.set_description("pacman database extraction utility");
    p.set_pkgbase("expac");
    p.set_pkgver("10.1");
    p.add_provides("expac=10");
    p.add_architectures("x86_64");
  }
  {
    auto& p = packages.emplace_back();
    p.set_name("auracle-git");
    p.set_description("A flexible client for the AUR");
    p.set_pkgbase("auracle");
    p.set_pkgver("0");
  }
  {
    auto& p = packages.emplace_back();
    p.set_name("pkgfile-git");
    p.set_description("a pacman .files metadata explorer");
    p.set_pkgbase("pkgfile");
    p.set_pkgver("32");
  }
  {
    auto& p = packages.emplace_back();
    p.set_name("pacman-git");
    p.set_description("A library-based package manager");
    p.set_pkgbase("pacman");
    p.set_pkgver("6.0.0");
    p.add_provides("pacman=6.0.0");
  }
  {
    auto& p = packages.emplace_back();
    p.set_name("pacman-extraponies-git");
    p.set_description("A library-based package manager with more ponies");
    p.set_pkgbase("pacman");
    p.set_pkgver("6.0.0");
    p.add_provides("pacman=6.0.0");
  }
  auto service = BuildService(packages);

  SearchRequest request;
  SearchResponse response;

  request.set_search_by(SearchRequest::SEARCHBY_NAME);
  request.set_search_logic(SearchRequest::SEARCHLOGIC_DISJUNCTIVE);
  request.add_terms("exp*");
  FillFieldMask(request.mutable_options(), {"name"});

  auto status = service->Search(request, &response);
  ASSERT_TRUE(status.ok()) << status.error_message();

  EXPECT_THAT(response.packages(),
              UnorderedElementsAre(Property(&Package::name, "expac-git")));
}

TEST_F(ServiceImplTest, SearchByNameIsCaseInsensitive) {
  std::vector<Package> packages;
  {
    auto& p = packages.emplace_back();
    p.set_name("expac-git");
    p.set_description("pacman database extraction utility");
    p.set_pkgbase("expac");
    p.set_pkgver("10.1");
    p.add_provides("expac=10");
    p.add_architectures("x86_64");
  }
  {
    auto& p = packages.emplace_back();
    p.set_name("auracle-git");
    p.set_description("A flexible client for the AUR");
    p.set_pkgbase("auracle");
    p.set_pkgver("0");
  }
  {
    auto& p = packages.emplace_back();
    p.set_name("pkgfile-git");
    p.set_description("a pacman .files metadata explorer");
    p.set_pkgbase("pkgfile");
    p.set_pkgver("32");
  }
  {
    auto& p = packages.emplace_back();
    p.set_name("pacman-git");
    p.set_description("A library-based package manager");
    p.set_pkgbase("pacman");
    p.set_pkgver("6.0.0");
    p.add_provides("pacman=6.0.0");
  }
  {
    auto& p = packages.emplace_back();
    p.set_name("pacman-extraponies-git");
    p.set_description("A library-based package manager with more ponies");
    p.set_pkgbase("pacman");
    p.set_pkgver("6.0.0");
    p.add_provides("pacman=6.0.0");
  }
  auto service = BuildService(packages);

  SearchRequest request;
  SearchResponse response;

  request.set_search_by(SearchRequest::SEARCHBY_NAME);
  request.set_search_logic(SearchRequest::SEARCHLOGIC_DISJUNCTIVE);
  request.add_terms("eXP*");
  FillFieldMask(request.mutable_options(), {"name"});

  auto status = service->Search(request, &response);
  ASSERT_TRUE(status.ok()) << status.error_message();

  EXPECT_THAT(response.packages(),
              UnorderedElementsAre(Property(&Package::name, "expac-git")));
}

TEST_F(ServiceImplTest, SearchByNameDesc) {
  std::vector<Package> packages;
  {
    auto& p = packages.emplace_back();
    p.set_name("expac-git");
    p.set_description("pacman database extraction utility");
    p.set_pkgbase("expac");
    p.set_pkgver("10.1");
    p.add_provides("expac=10");
    p.add_architectures("x86_64");
  }
  {
    auto& p = packages.emplace_back();
    p.set_name("auracle-git");
    p.set_description("A flexible client for the AUR");
    p.set_pkgbase("auracle");
    p.set_pkgver("0");
  }
  {
    auto& p = packages.emplace_back();
    p.set_name("pkgfile-git");
    p.set_description("a pacman .files metadata explorer");
    p.set_pkgbase("pkgfile");
    p.set_pkgver("32");
  }
  {
    auto& p = packages.emplace_back();
    p.set_name("pacman-git");
    p.set_description("A library-based package manager");
    p.set_pkgbase("pacman");
    p.set_pkgver("6.0.0");
    p.add_provides("pacman=6.0.0");
  }
  {
    auto& p = packages.emplace_back();
    p.set_name("pacman-extraponies-git");
    p.set_description("A library-based package manager with more ponies");
    p.set_pkgbase("pacman");
    p.set_pkgver("6.0.0");
    p.add_provides("pacman=6.0.0");
  }
  auto service = BuildService(packages);

  SearchRequest request;
  SearchResponse response;

  request.set_search_by(SearchRequest::SEARCHBY_NAME_DESC);
  request.set_search_logic(SearchRequest::SEARCHLOGIC_DISJUNCTIVE);
  request.add_terms("*PACMAN*");
  FillFieldMask(request.mutable_options(), {"name"});

  auto status = service->Search(request, &response);
  ASSERT_TRUE(status.ok()) << status.error_message();

  EXPECT_THAT(
      response.packages(),
      UnorderedElementsAre(Property(&Package::name, "expac-git"),
                           Property(&Package::name, "pkgfile-git"),
                           Property(&Package::name, "pacman-git"),
                           Property(&Package::name, "pacman-extraponies-git")));
}

TEST_F(ServiceImplTest, SearchByUnknownBy) {
  auto service = BuildService({});

  SearchRequest request;
  SearchResponse response;

  request.set_search_by(SearchRequest::SEARCHBY_UNKNOWN);
  auto status = service->Search(request, &response);
  EXPECT_EQ(status.error_code(), grpc::StatusCode::UNIMPLEMENTED);
}

TEST_F(ServiceImplTest, SearchByUnknownLogic) {
  auto service = BuildService({});

  SearchRequest request;
  SearchResponse response;

  request.set_search_logic(SearchRequest::SEARCHLOGIC_UNKNOWN);
  auto status = service->Search(request, &response);
  EXPECT_EQ(status.error_code(), grpc::StatusCode::UNIMPLEMENTED);
}

TEST_F(ServiceImplTest, SearchByNameDescConjunctive) {
  std::vector<Package> packages;
  {
    auto& p = packages.emplace_back();
    p.set_name("expac-git");
    p.set_description("pacman database extraction utility");
    p.set_pkgbase("expac");
    p.set_pkgver("10.1");
    p.add_provides("expac=10");
    p.add_architectures("x86_64");
  }
  {
    auto& p = packages.emplace_back();
    p.set_name("auracle-git");
    p.set_description("A flexible client for the AUR");
    p.set_pkgbase("auracle");
    p.set_pkgver("0");
  }
  {
    auto& p = packages.emplace_back();
    p.set_name("pkgfile-git");
    p.set_description("a pacman .files metadata explorer");
    p.set_pkgbase("pkgfile");
    p.set_pkgver("32");
  }
  {
    auto& p = packages.emplace_back();
    p.set_name("pacman-git");
    p.set_description("A library-based package manager");
    p.set_pkgbase("pacman");
    p.set_pkgver("6.0.0");
    p.add_provides("pacman=6.0.0");
  }
  {
    auto& p = packages.emplace_back();
    p.set_name("pacman-extraponies-git");
    p.set_description("A library-based package manager with more ponies");
    p.set_pkgbase("pacman");
    p.set_pkgver("6.0.0");
    p.add_provides("pacman=6.0.0");
  }
  auto service = BuildService(packages);

  SearchRequest request;
  SearchResponse response;

  request.set_search_by(SearchRequest::SEARCHBY_NAME_DESC);
  request.set_search_logic(SearchRequest::SEARCHLOGIC_CONJUNCTIVE);
  request.add_terms("*pacman*");
  request.add_terms("*metadata*");
  FillFieldMask(request.mutable_options(), {"name"});

  auto status = service->Search(request, &response);
  ASSERT_TRUE(status.ok()) << status.error_message();

  EXPECT_THAT(response.packages(),
              UnorderedElementsAre(Property(&Package::name, "pkgfile-git")));
}

TEST_F(ServiceImplTest, SearchWithFieldMask) {
  std::vector<Package> packages;
  {
    auto& p = packages.emplace_back();
    p.set_name("expac-git");
    p.set_description("pacman database extraction utility");
    p.set_pkgbase("expac");
    p.set_pkgver("10.1");
    p.add_provides("expac=10");
    p.add_architectures("x86_64");
  }
  {
    auto& p = packages.emplace_back();
    p.set_name("auracle-git");
    p.set_description("A flexible client for the AUR");
    p.set_pkgbase("auracle");
    p.set_pkgver("0");
  }
  {
    auto& p = packages.emplace_back();
    p.set_name("pkgfile-git");
    p.set_description("a pacman .files metadata explorer");
    p.set_pkgbase("pkgfile");
    p.set_pkgver("32");
  }
  {
    auto& p = packages.emplace_back();
    p.set_name("pacman-git");
    p.set_description("A library-based package manager");
    p.set_pkgbase("pacman");
    p.set_pkgver("6.0.0");
    p.add_provides("pacman=6.0.0");
  }
  {
    auto& p = packages.emplace_back();
    p.set_name("pacman-extraponies-git");
    p.set_description("A library-based package manager with more ponies");
    p.set_pkgbase("pacman");
    p.set_pkgver("6.0.0");
    p.add_provides("pacman=6.0.0");
  }
  auto service = BuildService(packages);

  SearchRequest request;
  SearchResponse response;

  request.set_search_by(SearchRequest::SEARCHBY_NAME);
  request.set_search_logic(SearchRequest::SEARCHLOGIC_DISJUNCTIVE);
  request.add_terms("expac-*");

  const std::vector<std::string> expected_fields{"name", "description",
                                                 "pkgbase", "pkgver"};
  FillFieldMask(request.mutable_options(), expected_fields);

  auto status = service->Search(request, &response);
  ASSERT_TRUE(status.ok()) << status.error_message();

  ASSERT_EQ(1, response.packages_size());
  EXPECT_THAT(GetAllSetFieldNames(response.packages(0)),
              UnorderedElementsAreArray(expected_fields));
}

TEST_F(ServiceImplTest, ResolveUnversioned) {
  std::vector<Package> packages;
  {
    auto& p = packages.emplace_back();
    p.set_name("expac-git");
    p.set_description("pacman database extraction utility");
    p.set_pkgbase("expac");
    p.set_pkgver("10.1");
    p.add_provides("expac=10");
    p.add_architectures("x86_64");
  }
  {
    auto& p = packages.emplace_back();
    p.set_name("auracle-git");
    p.set_description("A flexible client for the AUR");
    p.set_pkgbase("auracle");
    p.set_pkgver("0");
  }
  {
    auto& p = packages.emplace_back();
    p.set_name("pkgfile-git");
    p.set_description("a pacman .files metadata explorer");
    p.set_pkgbase("pkgfile");
    p.set_pkgver("32");
  }
  {
    auto& p = packages.emplace_back();
    p.set_name("pacman-git");
    p.set_description("A library-based package manager");
    p.set_pkgbase("pacman");
    p.set_pkgver("6.0.0");
    p.add_provides("pacman=6.0.0");
  }
  {
    auto& p = packages.emplace_back();
    p.set_name("pacman-extraponies-git");
    p.set_description("A library-based package manager with more ponies");
    p.set_pkgbase("pacman");
    p.set_pkgver("6.0.0");
    p.add_provides("pacman=6.0.0");
  }
  auto service = BuildService(packages);

  ResolveRequest request;
  ResolveResponse response;

  request.add_depstrings("pacman");
  request.add_depstrings("pkgfile-git");
  FillFieldMask(request.mutable_options(), {"name"});

  auto status = service->Resolve(request, &response);
  ASSERT_TRUE(status.ok()) << status.error_message();

  EXPECT_THAT(
      response.resolved_packages(),
      UnorderedElementsAre(
          AllOf(
              Property(&ResolveResponse::ResolvedPackage::depstring, "pacman"),
              Property(
                  &ResolveResponse::ResolvedPackage::providers,
                  UnorderedElementsAre(
                      Property(&Package::name, "pacman-git"),
                      Property(&Package::name, "pacman-extraponies-git")))),
          AllOf(Property(&ResolveResponse::ResolvedPackage::depstring,
                         "pkgfile-git"),
                Property(&ResolveResponse::ResolvedPackage::providers,
                         UnorderedElementsAre(
                             Property(&Package::name, "pkgfile-git"))))));
}

TEST_F(ServiceImplTest, ResolveVersioned) {
  std::vector<Package> packages;
  {
    auto& p = packages.emplace_back();
    p.set_name("expac-git");
    p.set_description("pacman database extraction utility");
    p.set_pkgbase("expac");
    p.set_pkgver("10.1");
    p.add_provides("expac=10");
    p.add_architectures("x86_64");
  }
  {
    auto& p = packages.emplace_back();
    p.set_name("auracle-git");
    p.set_description("A flexible client for the AUR");
    p.set_pkgbase("auracle");
    p.set_pkgver("0");
  }
  {
    auto& p = packages.emplace_back();
    p.set_name("pkgfile-git");
    p.set_description("a pacman .files metadata explorer");
    p.set_pkgbase("pkgfile");
    p.set_pkgver("32");
  }
  {
    auto& p = packages.emplace_back();
    p.set_name("pacman-git");
    p.set_description("A library-based package manager");
    p.set_pkgbase("pacman");
    p.set_pkgver("6.0.0");
    p.add_provides("pacman=6.0.0");
  }
  {
    auto& p = packages.emplace_back();
    p.set_name("pacman-extraponies-git");
    p.set_description("A library-based package manager with more ponies");
    p.set_pkgbase("pacman");
    p.set_pkgver("6.0.0");
    p.add_provides("pacman=6.0.0");
  }
  auto service = BuildService(packages);

  ResolveRequest request;
  ResolveResponse response;

  request.add_depstrings("pacman>5");
  request.add_depstrings("expac<11");
  FillFieldMask(request.mutable_options(), {"name"});

  auto status = service->Resolve(request, &response);
  ASSERT_TRUE(status.ok()) << status.error_message();

  EXPECT_THAT(response.resolved_packages(),
              UnorderedElementsAre(
                  AllOf(Property(&ResolveResponse::ResolvedPackage::depstring,
                                 "pacman>5"),
                        Property(&ResolveResponse::ResolvedPackage::providers,
                                 UnorderedElementsAre(
                                     Property(&Package::name, "pacman-git"),
                                     Property(&Package::name,
                                              "pacman-extraponies-git")))),
                  AllOf(Property(&ResolveResponse::ResolvedPackage::depstring,
                                 "expac<11"),
                        Property(&ResolveResponse::ResolvedPackage::providers,
                                 UnorderedElementsAre(
                                     Property(&Package::name, "expac-git"))))));
}

}  // namespace
