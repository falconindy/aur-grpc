#include "service/internal/package_index.hh"

#include "absl/strings/str_join.h"
#include "aur_internal.pb.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

using aur_internal::Package;
using aur_internal::PackageIndex;
using testing::IsEmpty;
using testing::Property;
using testing::UnorderedElementsAre;

namespace {

TEST(PackageIndexTest, IndexByRepeatedFieldAdapter) {
  std::vector<Package> packages;
  {
    auto& p = packages.emplace_back();
    p.set_name("auracle");
    p.add_maintainers("falconindy");
  }
  {
    auto& p = packages.emplace_back();
    p.set_name("pkgfile");
    p.add_maintainers("falconindy");
    p.add_maintainers("dreisner");
  }
  {
    auto& p = packages.emplace_back();
    p.set_name("expac");
    p.add_maintainers("dreisner");
  }
  {
    auto& p = packages.emplace_back();
    p.set_name("systemd");
    p.add_maintainers("eworm");
  }

  auto index = PackageIndex::Create(
      packages, "maintainers",
      PackageIndex::RepeatedFieldIndexingAdapter(&Package::maintainers));

  EXPECT_THAT(index.Get("falconindy"),
              UnorderedElementsAre(Property(&Package::name, "pkgfile"),
                                   Property(&Package::name, "auracle")));
  EXPECT_THAT(index.Get("dreisner"),
              UnorderedElementsAre(Property(&Package::name, "pkgfile"),
                                   Property(&Package::name, "expac")));

  EXPECT_THAT(index.Get("eworm"),
              UnorderedElementsAre(Property(&Package::name, "systemd")));
  EXPECT_THAT(index.Get("notfound"), IsEmpty());
}

TEST(PackageIndexTest, IndexByRepeatedFieldAdapterWithSyntheticEmpty) {
  std::vector<Package> packages;
  {
    auto& p = packages.emplace_back();
    p.set_name("auracle");
  }
  {
    auto& p = packages.emplace_back();
    p.set_name("systemd");
    p.add_maintainers("eworm");
  }

  auto index = PackageIndex::Create(
      packages, "maintainers",
      PackageIndex::RepeatedFieldIndexingAdapter(&Package::maintainers, true));

  EXPECT_THAT(index.Get(""),
              UnorderedElementsAre(Property(&Package::name, "auracle")));
}

TEST(PackageIndexTest, IndexByDepstringFieldAdapter) {
  std::vector<Package> packages;
  {
    auto& p = packages.emplace_back();
    p.set_name("auracle-git");
    p.add_provides("auracle=999");
  }
  {
    auto& p = packages.emplace_back();
    p.set_name("expac-git");
    p.add_provides("expac");
  }

  auto index = PackageIndex::Create(
      packages, "provides",
      PackageIndex::DepstringFieldIndexingAdapter(&Package::provides));

  EXPECT_THAT(index.Get("auracle"),
              UnorderedElementsAre(Property(&Package::name, "auracle-git")));
  EXPECT_THAT(index.Get("expac"),
              UnorderedElementsAre(Property(&Package::name, "expac-git")));
  EXPECT_THAT(index.Get("notfound"), IsEmpty());
}

TEST(PackageIndexTest, IndexByScalarFieldAdapter) {
  std::vector<Package> packages;
  {
    auto& p = packages.emplace_back();
    p.set_name("auracle-git");
    p.set_pkgbase("auracle");
  }

  auto index = PackageIndex::Create(
      packages, "pkgbase",
      PackageIndex::ScalarFieldIndexingAdapter(&Package::pkgbase));

  EXPECT_THAT(index.Get("auracle"),
              UnorderedElementsAre(Property(&Package::name, "auracle-git")));
  EXPECT_THAT(index.Get("notfound"), IsEmpty());
}

}  // namespace
