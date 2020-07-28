#include "service/internal/service_impl.hh"

#include <filesystem>

#include "aur_internal.pb.h"
#include "gmock/gmock.h"
#include "google/protobuf/util/field_mask_util.h"
#include "gtest/gtest.h"
#include "storage/file_io.hh"
#include "storage/inmemory_storage.hh"

namespace fs = std::filesystem;

using aur_internal::LookupRequest;
using aur_internal::LookupResponse;
using aur_internal::Package;
using aur_internal::ResolveRequest;
using aur_internal::ResolveResponse;
using aur_internal::SearchRequest;
using aur_internal::SearchResponse;
using aur_internal::ServiceImpl;
using aur_storage::InMemoryStorage;
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

void FillAllFieldsMask(aur_internal::RequestOptions* options) {
  *options->mutable_package_field_mask() =
      google::protobuf::util::FieldMaskUtil::GetFieldMaskForAllFields<
          aur_internal::Package>();
}

class ServiceImplTest : public testing::Test {
 public:
  ServiceImplTest() {
    PopulateDB();
    service_ = std::make_unique<ServiceImpl>(&storage_);
  }

 protected:
  grpc::Status Lookup(const LookupRequest& request, LookupResponse* response) {
    return service_->Lookup(request, response);
  }

  grpc::Status Search(const SearchRequest& request, SearchResponse* response) {
    return service_->Search(request, response);
  }

  grpc::Status Resolve(const ResolveRequest& request,
                       ResolveResponse* response) {
    return service_->Resolve(request, response);
  }

 private:
  void PopulateDB() {
    {
      Package p;
      p.set_name("expac-git");
      p.set_description("pacman database extraction utility");
      p.set_pkgbase("expac");
      p.set_pkgver("10.1");
      p.add_provides("expac=10");
      p.add_architectures("x86_64");
      AddPackage(p);
    }
    {
      Package p;
      p.set_name("auracle-git");
      p.set_description("A flexible client for the AUR");
      p.set_pkgbase("auracle");
      p.set_pkgver("0");
      AddPackage(p);
    }
    {
      Package p;
      p.set_name("pkgfile-git");
      p.set_description("a pacman .files metadata explorer");
      p.set_pkgbase("pkgfile");
      p.set_pkgver("32");
      AddPackage(p);
    }
    {
      Package p;
      p.set_name("pacman-git");
      p.set_description("A library-based package manager");
      p.set_pkgbase("pacman");
      p.set_pkgver("6.0.0");
      p.add_provides("pacman=6.0.0");
      AddPackage(p);
    }
    {
      Package p;
      p.set_name("pacman-extraponies-git");
      p.set_description("A library-based package manager with more ponies");
      p.set_pkgbase("pacman");
      p.set_pkgver("6.0.0");
      p.add_provides("pacman=6.0.0");
      AddPackage(p);
    }
  }

  void AddPackage(const Package& p) {
    storage_.Add(p.name(), p.SerializeAsString());
  }

  InMemoryStorage storage_;
  std::unique_ptr<ServiceImpl> service_;
};

TEST_F(ServiceImplTest, Lookup) {
  LookupRequest request;
  LookupResponse response;

  request.add_names("expac-git");
  request.add_names("auracle-git");
  request.add_names("notfound");

  FillAllFieldsMask(request.mutable_options());

  auto status = Lookup(request, &response);
  ASSERT_TRUE(status.ok()) << status.error_message();

  EXPECT_THAT(response.not_found_names(), UnorderedElementsAre("notfound"));
  EXPECT_THAT(response.packages(),
              UnorderedElementsAre(Property(&Package::name, "expac-git"),
                                   Property(&Package::name, "auracle-git")));
}

TEST_F(ServiceImplTest, LookupIsCaseInsensitive) {
  LookupRequest request;
  LookupResponse response;

  request.add_names("EXPAC-git");
  request.add_names("auracle-GIT");

  FillAllFieldsMask(request.mutable_options());

  auto status = Lookup(request, &response);
  ASSERT_TRUE(status.ok()) << status.error_message();

  EXPECT_THAT(response.packages(),
              UnorderedElementsAre(Property(&Package::name, "expac-git"),
                                   Property(&Package::name, "auracle-git")));
}

TEST_F(ServiceImplTest, SearchByName) {
  SearchRequest request;
  SearchResponse response;

  request.set_search_by(SearchRequest::SEARCHBY_NAME);
  request.add_terms("exp*");

  FillAllFieldsMask(request.mutable_options());

  auto status = Search(request, &response);
  ASSERT_TRUE(status.ok()) << status.error_message();

  EXPECT_THAT(response.packages(),
              UnorderedElementsAre(Property(&Package::name, "expac-git")));
}

TEST_F(ServiceImplTest, SearchByNameIsCaseInsensitive) {
  SearchRequest request;
  SearchResponse response;

  request.set_search_by(SearchRequest::SEARCHBY_NAME);
  request.add_terms("eXP*");

  FillAllFieldsMask(request.mutable_options());

  auto status = Search(request, &response);
  ASSERT_TRUE(status.ok()) << status.error_message();

  EXPECT_THAT(response.packages(),
              UnorderedElementsAre(Property(&Package::name, "expac-git")));
}

TEST_F(ServiceImplTest, SearchByNameDesc) {
  SearchRequest request;
  SearchResponse response;

  request.set_search_by(SearchRequest::SEARCHBY_NAME_DESC);
  request.add_terms("*PACMAN*");

  FillAllFieldsMask(request.mutable_options());

  auto status = Search(request, &response);
  ASSERT_TRUE(status.ok()) << status.error_message();

  EXPECT_THAT(
      response.packages(),
      UnorderedElementsAre(Property(&Package::name, "expac-git"),
                           Property(&Package::name, "pkgfile-git"),
                           Property(&Package::name, "pacman-git"),
                           Property(&Package::name, "pacman-extraponies-git")));
}

TEST_F(ServiceImplTest, SearchByNameDescConjunctive) {
  SearchRequest request;
  SearchResponse response;

  request.set_search_by(SearchRequest::SEARCHBY_NAME_DESC);
  request.set_search_logic(SearchRequest::SEARCHLOGIC_CONJUNCTIVE);
  request.add_terms("*pacman*");
  request.add_terms("*metadata*");

  FillAllFieldsMask(request.mutable_options());

  auto status = Search(request, &response);
  ASSERT_TRUE(status.ok()) << status.error_message();

  EXPECT_THAT(response.packages(),
              UnorderedElementsAre(Property(&Package::name, "pkgfile-git")));
}

TEST_F(ServiceImplTest, SearchWithFieldMask) {
  SearchRequest request;
  SearchResponse response;

  request.set_search_by(SearchRequest::SEARCHBY_NAME);
  request.add_terms("expac-*");

  std::vector<std::string> expected_fields{"name", "description", "pkgbase",
                                           "pkgver"};

  auto mask = request.mutable_options()->mutable_package_field_mask();
  for (const auto& field : expected_fields) {
    mask->add_paths(field);
  }

  auto status = Search(request, &response);
  ASSERT_TRUE(status.ok()) << status.error_message();

  ASSERT_EQ(1, response.packages_size());
  EXPECT_THAT(GetAllSetFieldNames(response.packages(0)),
              UnorderedElementsAreArray(expected_fields));
}

TEST_F(ServiceImplTest, ResolveUnversioned) {
  ResolveRequest request;
  ResolveResponse response;

  request.add_depstrings("pacman");
  request.add_depstrings("pkgfile-git");

  FillAllFieldsMask(request.mutable_options());

  auto status = Resolve(request, &response);
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
  ResolveRequest request;
  ResolveResponse response;

  request.add_depstrings("pacman>5");
  request.add_depstrings("expac<11");

  FillAllFieldsMask(request.mutable_options());

  auto status = Resolve(request, &response);
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
