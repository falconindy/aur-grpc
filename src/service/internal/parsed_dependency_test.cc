#include "service/internal/parsed_dependency.hh"

#include "aur_internal.pb.h"
#include "gtest/gtest.h"

using aur_internal::Package;
using aur_internal::ParsedDependency;

namespace {

TEST(ParsedDependencyTest, UnversionedRequirement) {
  Package foo;
  foo.set_name("foo");
  foo.set_pkgver("1.0.0");

  Package bar;
  bar.set_name("bar");
  bar.set_pkgver("1.0.0");

  ParsedDependency dep("foo");
  EXPECT_TRUE(dep.SatisfiedBy(foo));
  EXPECT_FALSE(dep.SatisfiedBy(bar));
}

TEST(ParsedDependencyTest, VersionedRequirement) {
  Package foo_0_9_9;
  foo_0_9_9.set_name("foo");
  foo_0_9_9.set_pkgver("0.9.9");

  Package foo_1_0_0;
  foo_1_0_0.set_name("foo");
  foo_1_0_0.set_pkgver("1.0.0");

  Package foo_1_1_0;
  foo_1_1_0.set_name("foo");
  foo_1_1_0.set_pkgver("1.1.0");

  Package bar_1_0_0;
  bar_1_0_0.set_name("bar");
  bar_1_0_0.set_pkgver("1.0.0");

  {
    ParsedDependency dep("foo=1.0.0");
    EXPECT_TRUE(dep.SatisfiedBy(foo_1_0_0));
    EXPECT_FALSE(dep.SatisfiedBy(foo_1_1_0));
  }

  {
    ParsedDependency dep("foo>=1.0.0");
    EXPECT_FALSE(dep.SatisfiedBy(foo_0_9_9));
    EXPECT_TRUE(dep.SatisfiedBy(foo_1_0_0));
    EXPECT_TRUE(dep.SatisfiedBy(foo_1_1_0));
  }

  {
    ParsedDependency dep("foo>1.0.0");
    EXPECT_FALSE(dep.SatisfiedBy(foo_0_9_9));
    EXPECT_FALSE(dep.SatisfiedBy(foo_1_0_0));
    EXPECT_TRUE(dep.SatisfiedBy(foo_1_1_0));
  }

  {
    ParsedDependency dep("foo<=1.0.0");
    EXPECT_TRUE(dep.SatisfiedBy(foo_0_9_9));
    EXPECT_TRUE(dep.SatisfiedBy(foo_1_0_0));
    EXPECT_FALSE(dep.SatisfiedBy(foo_1_1_0));
  }

  {
    ParsedDependency dep("foo<1.0.0");
    EXPECT_TRUE(dep.SatisfiedBy(foo_0_9_9));
    EXPECT_FALSE(dep.SatisfiedBy(foo_1_0_0));
    EXPECT_FALSE(dep.SatisfiedBy(foo_1_1_0));
  }

  {
    ParsedDependency dep("foo=1.0.0");
    EXPECT_FALSE(dep.SatisfiedBy(bar_1_0_0));
  }
}

TEST(ParsedDependencyTest, UnversionedRequirementByProvision) {
  Package bar;
  bar.set_name("bar");
  bar.set_pkgver("9.9.9");
  bar.add_provides("quux");
  bar.add_provides("foo");

  Package bar_2;
  bar_2.set_name("bar");
  bar_2.set_pkgver("9.9.9");
  bar_2.add_provides("quux");
  bar_2.add_provides("foo=42");

  ParsedDependency dep("foo");
  EXPECT_TRUE(dep.SatisfiedBy(bar));
  EXPECT_TRUE(dep.SatisfiedBy(bar_2));
}

TEST(ParsedDependencyTest, VersionedRequirementByProvision) {
  Package bar_0_9_9;
  bar_0_9_9.set_name("bar");
  bar_0_9_9.set_pkgver("9.9.9");
  bar_0_9_9.add_provides("quux");
  bar_0_9_9.add_provides("foo=0.9.9");

  Package bar_1_0_0;
  bar_1_0_0.set_name("bar");
  bar_1_0_0.set_pkgver("9.9.9");
  bar_0_9_9.add_provides("quux");
  bar_1_0_0.add_provides("foo=1.0.0");

  Package bar_1_1_0;
  bar_1_1_0.set_name("bar");
  bar_1_1_0.set_pkgver("9.9.9");
  bar_0_9_9.add_provides("quux");
  bar_1_1_0.add_provides("foo=1.1.0");

  {
    ParsedDependency dep("foo=1.0.0");
    EXPECT_TRUE(dep.SatisfiedBy(bar_1_0_0));
    EXPECT_FALSE(dep.SatisfiedBy(bar_1_1_0));
  }

  {
    ParsedDependency dep("foo>=1.0.0");
    EXPECT_FALSE(dep.SatisfiedBy(bar_0_9_9));
    EXPECT_TRUE(dep.SatisfiedBy(bar_1_0_0));
    EXPECT_TRUE(dep.SatisfiedBy(bar_1_1_0));
  }

  {
    ParsedDependency dep("foo>1.0.0");
    EXPECT_FALSE(dep.SatisfiedBy(bar_0_9_9));
    EXPECT_FALSE(dep.SatisfiedBy(bar_1_0_0));
    EXPECT_TRUE(dep.SatisfiedBy(bar_1_1_0));
  }

  {
    ParsedDependency dep("foo<=1.0.0");
    EXPECT_TRUE(dep.SatisfiedBy(bar_0_9_9));
    EXPECT_TRUE(dep.SatisfiedBy(bar_1_0_0));
    EXPECT_FALSE(dep.SatisfiedBy(bar_1_1_0));
  }

  {
    ParsedDependency dep("foo<1.0.0");
    EXPECT_TRUE(dep.SatisfiedBy(bar_0_9_9));
    EXPECT_FALSE(dep.SatisfiedBy(bar_1_0_0));
    EXPECT_FALSE(dep.SatisfiedBy(bar_1_1_0));
  }
}

TEST(ParsedDependencyTest, MalformedProvider) {
  Package foo;
  foo.add_provides("bar>=9");

  ParsedDependency dep("bar=9");
  EXPECT_FALSE(dep.SatisfiedBy(foo));
}

}  // namespace
