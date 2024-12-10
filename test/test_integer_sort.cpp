#include "gtest/gtest.h"

#include <algorithm>
#include <deque>
#include <numeric>

#include <parlay/primitives.h>
#include <parlay/sequence.h>

#include "sorting_utils.h"

#include <chrono>
#include <iostream>
#include <cstdlib> // for std::atoi

static_assert(parlay::is_trivially_relocatable_v<std::unique_ptr<int>>);

int g_n = 100000; // 默认值

TEST(TestIntegerSort, TestIntegerSortEmptyInput) {
  auto s = parlay::sequence<int>(0);
  const auto [sorted, counts] = parlay::internal::integer_sort_with_counts(
    make_slice(s),
    [](const int x) { return x; },
    1);
  EXPECT_EQ(sorted.size(), 0);
  EXPECT_EQ(counts.size(), 1);
}

TEST(TestIntegerSort, TestIntegerSortInplaceUniquePtr) {
  auto start_all = std::chrono::high_resolution_clock::now();

  // 初始化数据
  auto start_init = std::chrono::high_resolution_clock::now();
  auto s = parlay::tabulate(g_n, [](long long int i) {
    return std::make_unique<long long int>((50021 * i + 61) % (1 << 20));
  });
  auto sorted = parlay::tabulate(g_n, [](long long int i) {
    return std::make_unique<long long int>((50021 * i + 61) % (1 << 20));
  });
  auto end_init = std::chrono::high_resolution_clock::now();

  // std::sort
  auto start_sort = std::chrono::high_resolution_clock::now();
  std::sort(std::begin(sorted), std::end(sorted), [](const auto& p1, const auto& p2) {
    return *p1 < *p2;
  });
  auto end_sort = std::chrono::high_resolution_clock::now();

  // Parlay 的 inplace 排序
  auto start_parlay = std::chrono::high_resolution_clock::now();
  parlay::integer_sort_inplace(s, [](const auto& p) { return static_cast<unsigned int>(*p); });
  auto end_parlay = std::chrono::high_resolution_clock::now();

  // 总时间
  auto end_all = std::chrono::high_resolution_clock::now();

  std::cout << "Data generation time: "
            << std::chrono::duration_cast<std::chrono::milliseconds>(end_init - start_init).count() << " ms\n";
  std::cout << "Standard sort time: "
            << std::chrono::duration_cast<std::chrono::milliseconds>(end_sort - start_sort).count() << " ms\n";
  std::cout << "Parlay integer_sort time: "
            << std::chrono::duration_cast<std::chrono::milliseconds>(end_parlay - start_parlay).count() << " ms\n";
  std::cout << "Total test time: "
            << std::chrono::duration_cast<std::chrono::milliseconds>(end_all - start_all).count() << " ms\n";

  ASSERT_EQ(s.size(), sorted.size());
  for (size_t i = 0; i < g_n; i++) {
    ASSERT_EQ(*s[i], *sorted[i]);
  }
}

// HeapInt is both copyable and trivially relocatable
struct PARLAY_TRIVIALLY_RELOCATABLE HeapInt {
  int* x;
  HeapInt(int _x) : x(new int(_x)) { }
  ~HeapInt() { if (x != nullptr) delete x; }
  HeapInt(const HeapInt& other) : x(new int(*(other.x))) { }
  HeapInt(HeapInt&& other) noexcept : x(other.x) {
    other.x = nullptr;
  }
  HeapInt& operator=(const HeapInt& other) {
    if (this != &other) {
      if (x != nullptr) delete x;
      x = new int(*(other.x));
    }
    return *this;
  }
  HeapInt& operator=(HeapInt&& other) noexcept {
    if (x != nullptr) delete x;
    x = other.x;
    other.x = nullptr;
    return *this;
  }
  int value() const {
    assert(x != nullptr);
    return *x;
  }
};

#if defined(PARLAY_MUST_SPECIALIZE_IS_TRIVIALLY_RELOCATABLE)
namespace parlay {

template<>
PARLAY_ASSUME_TRIVIALLY_RELOCATABLE(HeapInt);

}
#endif

TEST(TestIntegerSort, TestIntegerSortCopyAndDestructiveMove) {
  auto start_total = std::chrono::high_resolution_clock::now();

  // Step 1: 生成数据
  auto start_data_gen = std::chrono::high_resolution_clock::now();
  auto s = parlay::tabulate(g_n, [](int i) {
    return HeapInt((51 * i + 61) % (1 << 20));
  });
  auto real_sorted = parlay::tabulate(g_n, [](int i) {
    return HeapInt((51 * i + 61) % (1 << 20));
  });
  auto end_data_gen = std::chrono::high_resolution_clock::now();

  // Step 2: 基准排序 (std::sort)
  auto start_std_sort = std::chrono::high_resolution_clock::now();
  std::sort(std::begin(real_sorted), std::end(real_sorted), [](const auto& p1, const auto& p2) {
    return p1.value() < p2.value();
  });
  auto end_std_sort = std::chrono::high_resolution_clock::now();

  // Step 3: Parlay integer_sort
  auto start_parlay_sort = std::chrono::high_resolution_clock::now();
  auto sorted = parlay::integer_sort(s, [](const auto& p) { return static_cast<unsigned int>(p.value()); });
  auto end_parlay_sort = std::chrono::high_resolution_clock::now();

  // Step 4: 验证结果
  auto start_verification = std::chrono::high_resolution_clock::now();
  ASSERT_EQ(sorted.size(), s.size());
  for (size_t i = 0; i < g_n; i++) {
    ASSERT_EQ(real_sorted[i].value(), sorted[i].value());
  }
  auto end_verification = std::chrono::high_resolution_clock::now();

  auto end_total = std::chrono::high_resolution_clock::now();

  // 输出时间统计
  std::cout << "Data generation time: "
            << std::chrono::duration_cast<std::chrono::milliseconds>(end_data_gen - start_data_gen).count()
            << " ms\n";
  std::cout << "Standard sort time: "
            << std::chrono::duration_cast<std::chrono::milliseconds>(end_std_sort - start_std_sort).count()
            << " ms\n";
  std::cout << "Parlay integer_sort time: "
            << std::chrono::duration_cast<std::chrono::milliseconds>(end_parlay_sort - start_parlay_sort).count()
            << " ms\n";
  std::cout << "Verification time: "
            << std::chrono::duration_cast<std::chrono::milliseconds>(end_verification - start_verification).count()
            << " ms\n";
  std::cout << "Total test time: "
            << std::chrono::duration_cast<std::chrono::milliseconds>(end_total - start_total).count()
            << " ms\n";
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);

  // 解析命令行参数，设置 n
  if (argc > 1) {
    g_n = std::atoi(argv[1]); // 转换命令行参数为整数
  }

  return RUN_ALL_TESTS();
}
