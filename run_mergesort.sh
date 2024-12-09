#!/bin/bash

# 输出文件名
output_file="mergesort_results.txt"

# 清空输出文件（如果存在的话）
echo "Running mergesort and mergesort_skew for various n values" > $output_file
echo "-----------------------------------------------" >> $output_file

# 启动循环，从 1 到 100000000，n每次乘以10
n=1
while [ $n -le 100000000 ]; do
    echo "Running for n=$n" >> $output_file
    
    # 运行 mergesort
    echo "Running mergesort for n=$n" >> $output_file
    ./mergesort $n >> $output_file
    
    # 运行 mergesort_skew
    echo "Running mergesort_skew for n=$n" >> $output_file
    ./mergesort_skew $n >> $output_file
    
    # 增加 n 为之前的10倍
    n=$((n * 10))
done

echo "All tasks completed. Results saved to $output_file."
