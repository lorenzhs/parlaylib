#include "gtest/gtest.h"

#include <algorithm>
#include <deque>
#include <numeric>

#include <parlay/monoid.h>
#include <parlay/primitives.h>
#include <parlay/sequence.h>

#include "sorting_utils.h"

TEST(TestPrimitives, TestTabulate) {
  auto s = parlay::tabulate(100000, [](long long i) -> long long {
    return (50021 * i + 61) % (1 << 20);
  });
  ASSERT_EQ(s.size(), 100000);
  for (size_t i = 0; i < 100000; i++) {
    ASSERT_EQ(s[i], (50021 * i + 61) % (1 << 20));
  }
}

TEST(TestPrimitives, TestMap) {
  auto s = parlay::tabulate(100000, [](long long i) -> long long {
    return (50021 * i + 61) % (1 << 20);
  });
  auto m = parlay::map(s, [](int x) { return 3*x - 1; });
  ASSERT_EQ(m.size(), s.size());
  for (size_t i = 0; i < 10; i++) {
    ASSERT_EQ(m[i], 3*s[i] - 1);
  }
}

TEST(TestPrimitives, TestDMap) {
  auto s = parlay::tabulate(100000, [](long long i) -> long long {
    return (50021 * i + 61) % (1 << 20);
  });
  auto m = parlay::dmap(s, [](int x) { return 3*x - 1; });
  ASSERT_EQ(m.size(), s.size());
  for (size_t i = 0; i < 100000; i++) {
    ASSERT_EQ(m[i], 3*s[i] - 1);
  }
}

TEST(TestPrimitives, TestCopy) {
  auto s = parlay::tabulate(100000, [](long long i) -> long long {
    return (50021 * i + 61) % (1 << 20);
  });
  auto s2 = parlay::sequence<long long>(100000);
  parlay::copy(s, s2);
  ASSERT_EQ(s, s2);
}

TEST(TestPrimitives, TestReduce) {
  auto s = parlay::tabulate(100000, [](long long i) -> long long {
    return (50021 * i + 61) % (1 << 20);
  });
  auto sum = parlay::reduce(s);
  ASSERT_EQ(sum, std::accumulate(std::begin(s), std::end(s), 0LL));
}

TEST(TestPrimitives, TestReduceMax) {
  auto s = parlay::tabulate(100000, [](long long i) -> long long {
    return (50021 * i + 61) % (1 << 20);
  });
  auto maxval = parlay::reduce(s, parlay::maxm<long long>());
  ASSERT_EQ(maxval, *std::max_element(std::begin(s), std::end(s)));
}

TEST(TestPrimitives, TestScan) {
  auto s = parlay::tabulate(100000, [](long long i) -> long long {
    return (50021 * i + 61) % (1 << 20);
  });
  auto [scanz, total] = parlay::scan(s);
  auto psums = parlay::sequence<long long>(100000);
  std::partial_sum(std::begin(s), std::end(s)-1, std::begin(psums)+1);
  ASSERT_EQ(scanz, psums);
  ASSERT_EQ(total, std::accumulate(std::begin(s), std::end(s), 0LL));
}

TEST(TestPrimitives, TestScanInclusive) {
  auto s = parlay::tabulate(100000, [](long long i) -> long long {
    return (50021 * i + 61) % (1 << 20);
  });
  auto [scanz, total] = parlay::scan_inclusive(s);
  auto psums = parlay::sequence<long long>(100000);
  std::partial_sum(std::begin(s), std::end(s), std::begin(psums));
  ASSERT_EQ(scanz, psums);
  ASSERT_EQ(total, std::accumulate(std::begin(s), std::end(s), 0LL));
}

TEST(TestPrimitives, TestScanInplace) {
  auto s = parlay::tabulate(100000, [](long long i) -> long long {
    return (50021 * i + 61) % (1 << 20);
  });
  auto sum = std::accumulate(std::begin(s), std::end(s), 0LL);
  auto psums = parlay::sequence<long long>(100000);
  std::partial_sum(std::begin(s), std::end(s)-1, std::begin(psums)+1);
  auto total = parlay::scan_inplace(s);
  ASSERT_EQ(s, psums);
  ASSERT_EQ(total, sum);
}

TEST(TestPrimitives, TestScanInclusiveInplace) {
  auto s = parlay::tabulate(100000, [](long long i) -> long long {
    return (50021 * i + 61) % (1 << 20);
  });
  auto sum = std::accumulate(std::begin(s), std::end(s), 0LL);
  auto psums = parlay::sequence<long long>(100000);
  std::partial_sum(std::begin(s), std::end(s), std::begin(psums));
  auto total = parlay::scan_inclusive_inplace(s);
  ASSERT_EQ(s, psums);
  ASSERT_EQ(total, sum);
}

template<typename T>
struct TakeMax {
  T operator()(const T& a, const T& b) { return std::max(a, b); }
};

TEST(TestPrimitives, TestScanMax) {
  auto s = parlay::tabulate(100000, [](long long i) -> long long {
    return (50021 * i + 61) % (1 << 20);
  });
  auto [scanz, total] = parlay::scan(s, parlay::maxm<long long>());
  auto psums = parlay::sequence<long long>(100000);
  psums[0] = parlay::maxm<long long>().identity;
  std::partial_sum(std::begin(s), std::end(s)-1, std::begin(psums)+1, TakeMax<long long>());
  ASSERT_EQ(scanz, psums);
  ASSERT_EQ(total, std::accumulate(std::begin(s), std::end(s), 0LL, TakeMax<long long>()));
}

TEST(TestPrimitives, TestScanInclusiveMax) {
  auto s = parlay::tabulate(100000, [](long long i) -> long long {
    return (50021 * i + 61) % (1 << 20);
  });
  auto [scanz, total] = parlay::scan_inclusive(s, parlay::maxm<long long>());
  auto psums = parlay::sequence<long long>(100000);
  std::partial_sum(std::begin(s), std::end(s), std::begin(psums), TakeMax<long long>());
  ASSERT_EQ(scanz, psums);
  ASSERT_EQ(total, std::accumulate(std::begin(s), std::end(s), 0LL, TakeMax<long long>()));
}

TEST(TestPrimitives, TestScanInplaceMax) {
  auto s = parlay::tabulate(100000, [](long long i) -> long long {
    return (50021 * i + 61) % (1 << 20);
  });
  auto sum = std::accumulate(std::begin(s), std::end(s), 0LL, TakeMax<long long>());
  auto psums = parlay::sequence<long long>(100000);
  psums[0] = parlay::maxm<long long>().identity;
  std::partial_sum(std::begin(s), std::end(s)-1, std::begin(psums)+1, TakeMax<long long>());
  auto total = parlay::scan_inplace(s, parlay::maxm<long long>());
  ASSERT_EQ(s, psums);
  ASSERT_EQ(total, sum);
}

TEST(TestPrimitives, TestScanInclusiveInplaceMax) {
  auto s = parlay::tabulate(100000, [](long long i) -> long long {
    return (50021 * i + 61) % (1 << 20);
  });
  auto sum = std::accumulate(std::begin(s), std::end(s), 0LL, TakeMax<long long>());
  auto psums = parlay::sequence<long long>(100000);
  std::partial_sum(std::begin(s), std::end(s), std::begin(psums), TakeMax<long long>());
  auto total = parlay::scan_inclusive_inplace(s, parlay::maxm<long long>());
  ASSERT_EQ(s, psums);
  ASSERT_EQ(total, sum);
}

TEST(TestPrimitives, TestPack) {
  auto s = parlay::tabulate(100000, [](int i) { return i; });
  auto b = parlay::tabulate(100000, [](int i) -> bool { return i % 2 == 0; });
  auto packed = parlay::pack(s, b);
  ASSERT_EQ(packed.size(), 50000);
  for (size_t i = 0; i < 50000; i++) {
    ASSERT_EQ(packed[i], 2*i);
  }
}

TEST(TestPrimitives, TestPackConvertible) {
  auto s = parlay::tabulate(100000, [](int i) { return i; });
  auto b = parlay::tabulate(100000, [](int i) -> int { return i % 2 == 0; });
  auto packed = parlay::pack(s, b);
  ASSERT_EQ(packed.size(), 50000);
  for (size_t i = 0; i < 50000; i++) {
    ASSERT_EQ(packed[i], 2*i);
  }
}

TEST(TestPrimitives, TestPackInto) {
  auto s = parlay::tabulate(100000, [](int i) { return i; });
  auto d = parlay::sequence<int>(50000);
  auto b = parlay::tabulate(100000, [](int i) -> bool { return i % 2 == 0; });
  auto packed = parlay::pack_into(s, b, d);
  ASSERT_EQ(packed, 50000);
  ASSERT_EQ(d.size(), 50000);
  for (size_t i = 0; i < 50000; i++) {
    ASSERT_EQ(d[i], 2*i);
  }
}

TEST(TestPrimitives, TestPackIntoConvertible) {
  auto s = parlay::tabulate(100000, [](int i) { return i; });
  auto d = parlay::sequence<int>(50000);
  auto b = parlay::tabulate(100000, [](int i) -> int { return i % 2 == 0; });
  auto packed = parlay::pack_into(s, b, d);
  ASSERT_EQ(packed, 50000);
  ASSERT_EQ(d.size(), 50000);
  for (size_t i = 0; i < 50000; i++) {
    ASSERT_EQ(d[i], 2*i);
  }
}

TEST(TestPrimitives, TestPackIndex) {
  auto s = parlay::tabulate(100000, [](int i) -> int { return i % 2 == 0; });
  auto packed = parlay::pack_index(s);
  ASSERT_EQ(packed.size(), 50000);
  for (size_t i = 0; i < 50000; i++) {
    ASSERT_EQ(packed[i], 2*i);
  }
}

TEST(TestPrimitives, TestPackIndexType) {
  auto s = parlay::tabulate(100000, [](int i) -> int { return i % 2 == 0; });
  auto packed = parlay::pack_index<int>(s);
  ASSERT_EQ(packed.size(), 50000);
  for (size_t i = 0; i < 50000; i++) {
    ASSERT_EQ(packed[i], 2*i);
  }
}

TEST(TestPrimitives, TestFilter) {
  auto s = parlay::tabulate(100000, [](int i) { return i; });
  auto f = parlay::filter(s, [](auto x) { return x % 3 == 0; });
  ASSERT_EQ(f.size(), 33334);
  for (size_t i = 0; i < 33334; i++) {
    ASSERT_EQ(f[i], 3*i);
  }
}

TEST(TestPrimitives, TestFilterInto) {
  auto s = parlay::tabulate(100000, [](int i) { return i; });
  auto d = parlay::sequence<int>(33334);
  auto f = parlay::filter_into(s, d, [](auto x) { return x % 3 == 0; });
  ASSERT_EQ(d.size(), 33334);
  ASSERT_EQ(f, 33334);
  for (size_t i = 0; i < 33334; i++) {
    ASSERT_EQ(d[i], 3*i);
  }
}

TEST(TestPrimitives, TestHistogram) {
  auto s = parlay::tabulate(100000, [](long long i) -> long long {
    return (50021 * i + 61) % (1 << 20);
  });
  auto hist = parlay::histogram(s, 1 << 20);
  ASSERT_EQ(hist.size(), 1 << 20);
  auto cnts = parlay::sequence<size_t>(1 << 20, 0);
  for (auto x : s) {
    cnts[x]++;
  }
  ASSERT_TRUE(std::equal(hist.begin(), hist.end(), cnts.begin()));
}

TEST(TestPrimitives, TestSort) {
  auto s = parlay::tabulate(100000, [](long long i) -> long long {
    return (50021 * i + 61) % (1 << 20);
  });
  auto sorted = parlay::sort(s);
  ASSERT_EQ(s.size(), sorted.size());
  std::sort(std::begin(s), std::end(s));
  ASSERT_EQ(s, sorted);
  ASSERT_TRUE(std::is_sorted(std::begin(sorted), std::end(sorted)));
}

TEST(TestPrimitives, TestSortCustomCompare) {
  auto s = parlay::tabulate(100000, [](long long i) -> long long {
    return (50021 * i + 61) % (1 << 20);
  });
  auto sorted = parlay::sort(s, std::greater<long long>());
  ASSERT_EQ(s.size(), sorted.size());
  std::sort(std::rbegin(s), std::rend(s));
  ASSERT_EQ(s, sorted);
  ASSERT_TRUE(std::is_sorted(std::rbegin(sorted), std::rend(sorted)));
}

TEST(TestPrimitives, TestStableSort) {
  auto s = parlay::tabulate(100000, [](long long i) -> UnstablePair {
    UnstablePair x;
    x.x = (53 * i + 61) % (1 << 10);
    x.y = i;
    return x;
  });
  auto sorted = parlay::stable_sort(s);
  ASSERT_EQ(s.size(), sorted.size());
  std::stable_sort(std::begin(s), std::end(s));
  ASSERT_EQ(s, sorted);
  ASSERT_TRUE(std::is_sorted(std::begin(sorted), std::end(sorted)));
}

TEST(TestPrimitives, TestStableSortCustomCompare) {
  auto s = parlay::tabulate(100000, [](long long i) -> UnstablePair {
    UnstablePair x;
    x.x = (53 * i + 61) % (1 << 10);
    x.y = i;
    return x;
  });
  auto sorted = parlay::stable_sort(s, std::greater<UnstablePair>());
  ASSERT_EQ(s.size(), sorted.size());
  std::stable_sort(std::rbegin(s), std::rend(s));
  ASSERT_EQ(s, sorted);
  ASSERT_TRUE(std::is_sorted(std::rbegin(sorted), std::rend(sorted)));
}

TEST(TestPrimitives, TestSortInplace) {
  auto s = parlay::tabulate(100000, [](long long i) -> long long {
    return (50021 * i + 61) % (1 << 20);
  });
  auto s2 = s;
  ASSERT_EQ(s, s2);
  parlay::sort_inplace(s);
  std::sort(std::begin(s2), std::end(s2));
  ASSERT_EQ(s, s2); 
  ASSERT_TRUE(std::is_sorted(std::begin(s), std::end(s)));
}

TEST(TestPrimitives, TestSortInplaceCustomCompare) {
  auto s = parlay::tabulate(100000, [](long long i) -> long long {
    return (50021 * i + 61) % (1 << 20);
  });
  auto s2 = s;
  ASSERT_EQ(s, s2);
  parlay::sort_inplace(s, std::greater<long long>());
  std::sort(std::rbegin(s2), std::rend(s2));
  ASSERT_EQ(s, s2); 
  ASSERT_TRUE(std::is_sorted(std::rbegin(s), std::rend(s)));
}

TEST(TestPrimitives, TestStableSortInplace) {
  auto s = parlay::tabulate(100000, [](long long i) -> UnstablePair {
    UnstablePair x;
    x.x = (53 * i + 61) % (1 << 10);
    x.y = i;
    return x;
  });
  auto s2 = s;
  ASSERT_EQ(s, s2);
  parlay::stable_sort_inplace(s);
  std::stable_sort(std::begin(s2), std::end(s2));
  ASSERT_EQ(s, s2);
  ASSERT_TRUE(std::is_sorted(std::begin(s), std::end(s)));
}

TEST(TestPrimitives, TestStableSortInplaceCustomCompare) {
  auto s = parlay::tabulate(100000, [](long long i) -> UnstablePair {
    UnstablePair x;
    x.x = (53 * i + 61) % (1 << 10);
    x.y = i;
    return x;
  });
  auto s2 = s;
  ASSERT_EQ(s, s2);
  parlay::stable_sort_inplace(s, std::greater<UnstablePair>());
  std::stable_sort(std::rbegin(s2), std::rend(s2));
  ASSERT_EQ(s, s2);
  ASSERT_TRUE(std::is_sorted(std::rbegin(s), std::rend(s)));
}

TEST(TestPrimitives, TestSortInplaceUncopyable) {
  auto s = parlay::tabulate(100000, [](int i) -> UncopyableThing {
    return UncopyableThing(i);
  });
  auto s2 = parlay::tabulate(100000, [](int i) -> UncopyableThing {
    return UncopyableThing(i);
  });
  ASSERT_EQ(s, s2);
  parlay::sort_inplace(s, std::less<UncopyableThing>());
  std::stable_sort(std::begin(s2), std::end(s2));
  ASSERT_EQ(s, s2);
  ASSERT_TRUE(std::is_sorted(std::begin(s), std::end(s))); 
}

TEST(TestPrimitives, TestStableSortInplaceUncopyable) {
  auto s = parlay::tabulate(100000, [](int i) -> UncopyableThing {
    return UncopyableThing(i);
  });
  auto s2 = parlay::tabulate(100000, [](int i) -> UncopyableThing {
    return UncopyableThing(i);
  });
  ASSERT_EQ(s, s2);
  parlay::stable_sort_inplace(s, std::less<UncopyableThing>());
  std::stable_sort(std::begin(s2), std::end(s2));
  ASSERT_EQ(s, s2);
  ASSERT_TRUE(std::is_sorted(std::begin(s), std::end(s))); 
}

TEST(TestPrimitives, TestSortInplaceNonContiguous) {
  auto ss = parlay::tabulate(100000, [](long long i) -> long long {
    return (50021 * i + 61) % (1 << 20);
  });
  auto s = std::deque<long long>(ss.begin(), ss.end());
  auto s2 = s;
  ASSERT_EQ(s, s2);
  parlay::sort_inplace(s, std::less<long long>());
  std::sort(std::begin(s2), std::end(s2));
  ASSERT_EQ(s, s2); 
  ASSERT_TRUE(std::is_sorted(std::begin(s), std::end(s)));
}

TEST(TestPrimitives, TestStableSortInplaceNonContiguous) {
  auto ss = parlay::tabulate(100000, [](long long i) -> long long {
    return (50021 * i + 61) % (1 << 20);
  });
  auto s = std::deque<long long>(ss.begin(), ss.end());
  auto s2 = s;
  ASSERT_EQ(s, s2);
  parlay::stable_sort_inplace(s, std::less<long long>());
  std::sort(std::begin(s2), std::end(s2));
  ASSERT_EQ(s, s2); 
  ASSERT_TRUE(std::is_sorted(std::begin(s), std::end(s)));
}

TEST(TestPrimitives, TestIntegerSort) {
  auto s = parlay::tabulate(100000, [](unsigned long long i) -> unsigned long long {
    return (50021 * i + 61) % (1 << 20);
  });
  auto sorted = parlay::integer_sort(s);
  ASSERT_EQ(s.size(), sorted.size());
  std::sort(std::begin(s), std::end(s));
  ASSERT_EQ(s, sorted);
  ASSERT_TRUE(std::is_sorted(std::begin(sorted), std::end(sorted)));
}

TEST(TestPrimitives, TestIntegerSortInplace) {
  auto s = parlay::tabulate(100000, [](unsigned long long i) -> unsigned long long {
    return (50021 * i + 61) % (1 << 20);
  });
  auto s2 = s;
  ASSERT_EQ(s, s2);
  parlay::integer_sort_inplace(s);
  std::sort(std::begin(s2), std::end(s2));
  ASSERT_EQ(s, s2); 
  ASSERT_TRUE(std::is_sorted(std::begin(s), std::end(s)));
}

TEST(TestPrimitives, TestIntegerSortCustomKey) {
  auto s = parlay::tabulate(100000, [](long long i) -> UnstablePair {
    UnstablePair x;
    x.x = (53 * i + 61) % (1 << 10);
    x.y = 0;
    return x;
  });
  auto sorted = parlay::integer_sort(s, [](const auto& x) -> unsigned long long {
    return x.x;
  });
  ASSERT_EQ(s.size(), sorted.size());
  std::stable_sort(std::begin(s), std::end(s));
  ASSERT_EQ(s, sorted);
  ASSERT_TRUE(std::is_sorted(std::begin(sorted), std::end(sorted)));
}

TEST(TestPrimitives, TestIntegerSortInplaceCustomKey) {
  auto s = parlay::tabulate(100000, [](long long i) -> UnstablePair {
    UnstablePair x;
    x.x = (53 * i + 61) % (1 << 10);
    x.y = 0;
    return x;
  });
  auto s2 = s;
  ASSERT_EQ(s, s2);
  parlay::integer_sort_inplace(s, [](const auto& x) -> unsigned long long {
    return x.x;
  });
  std::stable_sort(std::begin(s2), std::end(s2));
  ASSERT_EQ(s, s2);
  ASSERT_TRUE(std::is_sorted(std::begin(s), std::end(s)));
}

TEST(TestPrimitives, TestIntegerSortInplaceUncopyable) {
  auto s = parlay::tabulate(100000, [](int i) -> UncopyableThing {
    return UncopyableThing(100000-i);
  });
  auto s2 = parlay::tabulate(100000, [](int i) -> UncopyableThing {
    return UncopyableThing(100000-i);
  });
  ASSERT_EQ(s, s2);
  parlay::integer_sort_inplace(s, [](const auto& a) { return a.x; });
  std::sort(std::begin(s2), std::end(s2));
  ASSERT_EQ(s, s2);
  ASSERT_TRUE(std::is_sorted(std::begin(s), std::end(s))); 
}

TEST(TestPrimitives, TestIntegerSortInplaceNonContiguous) {
  auto ss = parlay::tabulate(100000, [](long long i) -> long long {
    return (50021 * i + 61) % (1 << 20);
  });
  auto s = std::deque<long long>(ss.begin(), ss.end());
  auto s2 = s;
  ASSERT_EQ(s, s2);
  parlay::integer_sort_inplace(s);
  std::sort(std::begin(s2), std::end(s2));
  ASSERT_EQ(s, s2); 
  ASSERT_TRUE(std::is_sorted(std::begin(s), std::end(s)));
}
