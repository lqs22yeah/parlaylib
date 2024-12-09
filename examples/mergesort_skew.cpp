#include <iostream>
#include <string>
#include <random>

#include <parlay/io.h>
#include <parlay/primitives.h>
#include <parlay/random.h>
#include <parlay/sequence.h>

#include "mergesort.h"

// **************************************************************
// Driver
// **************************************************************
std::vector<long> generate_zipf_weights(int n, double s) {
    std::vector<long> weights(n);
    
    // Sum of the values to normalize later
    double sum = 0;
    
    // Calculate raw Zipf values for each index
    for (int i = 1; i <= n; ++i) {
        weights[i-1] = static_cast<long>(1000000*pow(i, -s));
        sum += weights[i-1];
    }
    
    // // Normalize the weights so they sum to 1
    // for (int i = 0; i < n; ++i) {
    //     weights[i] = static_cast<long>(weights[i] / sum * 10000000);
    // }
    std::random_device rd;          // 1
    std::mt19937 g(rd());           // 2
    std::shuffle(weights.begin(), weights.end(), g); // 3

    return weights;
}
int main(int argc, char* argv[]) {
  auto usage = "Usage: mergesort <n>";
  if (argc != 2) std::cout << usage << std::endl;
  else {
    long n;
    try { n = std::stol(argv[1]); }
    catch (...) { std::cout << usage << std::endl; return 1; }
    parlay::random_generator gen;
    double s=1;
    auto weights = generate_zipf_weights(n, s);
    std::discrete_distribution<long> dis(weights.begin(),weights.end());
    

    // generate random long values
    auto data = parlay::tabulate(n, [&] (long i) {
      auto r = gen[i];
      return dis(r);});
    
    parlay::internal::timer t("Time");
    parlay::sequence<long> result;
    for (int i=0; i < 5; i++) {
      result = data;
      t.start();
      merge_sort(result);
      t.next("mergesort");
    }

    auto first_ten = result.head(200);

    std::cout << "first 200 elements: " << parlay::to_chars(first_ten) << std::endl;
  }
}

