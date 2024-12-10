#!/bin/bash

# 输出文件，用于记录结果
OUTPUT_FILE="integer_sorting_results.txt"
echo "Testing results for different n values" > $OUTPUT_FILE  # 写入文件头

# 定义初始值
n=1000
max_n=10000000

# 循环处理不同的 n
while [ $n -le $max_n ]; do
  echo "Running for n = $n..."
  
  # 运行程序并将输出记录到文件
  echo "Running for n = $n" >> $OUTPUT_FILE
  ./test_integer_sort $n >> $OUTPUT_FILE

  echo "skew:Running for n = $n" >> $OUTPUT_FILE
  ./test_integer_sort_skew $n >> $OUTPUT_FILE
  
  # 更新 n，每次乘以 10
  n=$((n * 10))
done

echo "Testing complete. Results saved to $OUTPUT_FILE."
