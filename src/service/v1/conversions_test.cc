#include "service/v1/conversions.hh"

#include "aur_internal.pb.h"
#include "aur_v1.pb.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace v1 = aur::v1;

namespace {

std::vector<std::string> AllPackageFieldNames() {
  std::vector<std::string> field_names;

  auto* descriptor = v1::Package::GetDescriptor();
  for (int i = 0; i < descriptor->field_count(); ++i) {
    field_names.push_back(descriptor->field(i)->name());
  }
  return field_names;
}

TEST(ConversionsTest, AddsDefaultFieldMaskForSearch) {
  v1::SearchRequest request;
  request.add_terms("blah");

  auto internal_request = ToInternal(request);

  EXPECT_THAT(internal_request.options().package_field_mask().paths(),
              testing::UnorderedElementsAre("name"));
}

TEST(ConversionsTest, SetsDefaultSearchBy) {
  v1::SearchRequest request;
  request.add_terms("blah");

  auto internal_request = ToInternal(request);

  EXPECT_EQ(internal_request.search_by(),
            aur_internal::SearchRequest::SEARCHBY_NAME_DESC);
}

TEST(ConversionsTest, SetsDefaultSearchLogic) {
  v1::SearchRequest request;
  request.add_terms("blah");

  auto internal_request = ToInternal(request);

  EXPECT_EQ(internal_request.search_logic(),
            aur_internal::SearchRequest::SEARCHLOGIC_DISJUNCTIVE);
}

TEST(ConversionsTest, AddsDefaultFieldMaskForLookup) {
  v1::LookupRequest request;
  request.add_names("blah");

  auto internal_request = ToInternal(request);

  EXPECT_THAT(internal_request.options().package_field_mask().paths(),
              testing::UnorderedElementsAreArray(AllPackageFieldNames()));
}

TEST(ConversionsTest, SetsDefaultLookupBy) {
  v1::LookupRequest request;
  request.add_names("blah");

  auto internal_request = ToInternal(request);

  EXPECT_EQ(internal_request.lookup_by(),
            aur_internal::LookupRequest::LOOKUPBY_NAME);
}

TEST(ConversionsTest, AddsDefaultFieldMaskForResolve) {
  v1::ResolveRequest request;
  request.add_depstrings("blah");

  auto internal_request = ToInternal(request);

  EXPECT_THAT(internal_request.options().package_field_mask().paths(),
              testing::UnorderedElementsAreArray(AllPackageFieldNames()));
}

}  // namespace
