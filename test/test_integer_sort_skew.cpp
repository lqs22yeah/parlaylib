#include "gtest/gtest.h"

#include <algorithm>
#include <deque>
#include <numeric>
#include <random>
#include <memory> // for std::unique_ptr

#include <parlay/primitives.h>
#include <parlay/sequence.h>

#include "sorting_utils.h"
#include <chrono>
#include <iostream>
#include <cstdlib> // for std::atoi

int g_n = 100000; // 默认值
double g_zipf_param = 1.0; // Zipf 分布参数默认值

// 生成 Zipf 分布权重
std::vector<long> generate_zipf_weights(int n, double s) {
  std::vector<long> weights(n);
  for (int i = 1; i <= n; ++i) {
    weights[i - 1] = static_cast<long>(1000000 * pow(i, -s)); // Zipf 分布公式
  }
  std::random_device rd;
  std::mt19937 g(rd());
  std::shuffle(weights.begin(), weights.end(), g); // 随机化权重顺序
  return weights;
}

// 确保 std::unique_ptr 满足 Parlay 的 trivially relocatable 性质
static_assert(parlay::is_trivially_relocatable_v<std::unique_ptr<int>>);

TEST(TestIntegerSort, TestIntegerSortEmptyInput) {
  auto s = parlay::sequence<int>(0); // 空序列
  const auto [sorted, counts] = parlay::internal::integer_sort_with_counts(
    make_slice(s),
    [](const int x) { return x; },
    1);
  EXPECT_EQ(sorted.size(), 0); // 验证排序后大小
  EXPECT_EQ(counts.size(), 1); // 验证计数
}

TEST(TestIntegerSort, TestIntegerSortInplaceUniquePtr) {
  // 记录总时间
  auto start_total = std::chrono::high_resolution_clock::now();

  // 生成 Zipf 分布权重
  auto start_weights = std::chrono::high_resolution_clock::now();
  auto weights = generate_zipf_weights(g_n, g_zipf_param);
  auto end_weights = std::chrono::high_resolution_clock::now();

  // 初始化数据
  auto start_data_gen = std::chrono::high_resolution_clock::now();
  std::random_device rd;
  std::mt19937 gen(rd());
  std::discrete_distribution<long> dis(weights.begin(), weights.end());
  auto s = parlay::tabulate(g_n, [&] (long i) {
    return std::make_unique<long long int>(dis(gen));
  });

  auto sorted = parlay::tabulate(g_n, [&](long i) {
    return std::make_unique<long long int>(*s[i]);
  });
  auto end_data_gen = std::chrono::high_resolution_clock::now();

  // std::sort
  auto start_std_sort = std::chrono::high_resolution_clock::now();
  std::sort(std::begin(sorted), std::end(sorted), [](const auto& p1, const auto& p2) {
    return *p1 < *p2;
  });
  auto end_std_sort = std::chrono::high_resolution_clock::now();

  // Parlay 的 inplace 排序
  auto start_parlay_sort = std::chrono::high_resolution_clock::now();
  parlay::integer_sort_inplace(s, [](const auto& p) { return static_cast<unsigned int>(*p); });
  auto end_parlay_sort = std::chrono::high_resolution_clock::now();

  // 验证排序结果
  auto start_verification = std::chrono::high_resolution_clock::now();
  ASSERT_EQ(s.size(), sorted.size());
  for (size_t i = 0; i < g_n; i++) {
    ASSERT_EQ(*s[i], *sorted[i]);
  }
  auto end_verification = std::chrono::high_resolution_clock::now();

  auto end_total = std::chrono::high_resolution_clock::now();

  // 输出时间统计
  std::cout << "Weight generation time: "
            << std::chrono::duration_cast<std::chrono::milliseconds>(end_weights - start_weights).count()
            << " ms\n";
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
  // 记录总时间
  auto start_total = std::chrono::high_resolution_clock::now();

  // 生成 Zipf 分布权重
  auto start_weights = std::chrono::high_resolution_clock::now();
  auto weights = generate_zipf_weights(g_n, g_zipf_param);
  auto end_weights = std::chrono::high_resolution_clock::now();

  // 初始化数据
  auto start_data_gen = std::chrono::high_resolution_clock::now();
  std::random_device rd;
  std::mt19937 gen(rd());
  std::discrete_distribution<long> dis(weights.begin(), weights.end());
  auto s = parlay::tabulate(g_n, [&] (long i) {
    return HeapInt(dis(gen));
  });

  auto real_sorted = parlay::tabulate(g_n, [&](long i) {
    return s[i];
  });
  auto end_data_gen = std::chrono::high_resolution_clock::now();

  // std::sort
  auto start_std_sort = std::chrono::high_resolution_clock::now();
  std::sort(std::begin(real_sorted), std::end(real_sorted), [](const auto& p1, const auto& p2) {
    return p1.value() < p2.value();
  });
  auto end_std_sort = std::chrono::high_resolution_clock::now();

  // Parlay integer_sort
  auto start_parlay_sort = std::chrono::high_resolution_clock::now();
  auto sorted = parlay::integer_sort(s, [](const auto& p) { return static_cast<unsigned int>(p.value()); });
  auto end_parlay_sort = std::chrono::high_resolution_clock::now();

  // 验证排序结果
  auto start_verification = std::chrono::high_resolution_clock::now();
  ASSERT_EQ(sorted.size(), s.size());
  for (size_t i = 0; i < g_n; i++) {
    ASSERT_EQ(real_sorted[i].value(), sorted[i].value());
  }
  auto end_verification = std::chrono::high_resolution_clock::now();

  auto end_total = std::chrono::high_resolution_clock::now();

  // 输出时间统计
  std::cout << "Weight generation time: "
            << std::chrono::duration_cast<std::chrono::milliseconds>(end_weights - start_weights).count()
            << " ms\n";
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
  // 输出前100个排序结果
  std::cout << "First 100 sorted elements:\n";
  for (size_t i = 0; i < 100 && i < s.size(); i++) {
    std::cout << real_sorted[i].value() << " ";
  }
  std::cout << std::endl;
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);

  // 解析命令行参数，设置 n 和 zipf 参数
  if (argc > 1) {
    g_n = std::atoi(argv[1]);
  }
  if (argc > 2) {
    g_zipf_param = std::atof(argv[2]);
  }

  return RUN_ALL_TESTS();
}
