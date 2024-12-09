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
std::vector<long> generate_exponential_weights(int n,double lambda){
  std::vector<long> weights(n);
  for (int i=0;i<n;i++){
    weights[i]=static_cast<long>(10000000*exp(-lambda*i));
  }
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
    double lambda=0.01;
    auto weights=generate_exponential_weights(n,lambda);
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

