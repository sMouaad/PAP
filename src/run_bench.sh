#!/bin/bash
# Submit with: oarsub -l host=1/core=16 "$(pwd)/run_bench.sh"
set -e

cd "$(dirname "$0")"

echo "===== Machine info ====="
lscpu | grep -E "Model name|Socket\(s\)|Core\(s\) per socket|Thread\(s\) per core|^CPU\(s\)"
echo "========================"

echo "===== Build ====="
make -B
echo "===== Build done ====="

echo "===== Smoke tests ====="
OMP_NUM_THREADS=1 ./bubble.run 5
OMP_NUM_THREADS=1 ./mergesort.run 5
OMP_NUM_THREADS=1 ./odd-even.run 5
echo "===== Smoke tests done ====="

echo "===== Benchmarks ====="
python3 -m pip install matplotlib --user -q
export OMP_DYNAMIC=false
export OMP_PROC_BIND=close
export OMP_PLACES=cores
# Override when needed before submission, e.g.:
# BENCH_PROFILE=quick MAX_THREADS=16 oarsub ... run_bench.sh
export BENCH_PROFILE=${BENCH_PROFILE:-full}
export MAX_THREADS=${MAX_THREADS:-16}
echo "BENCH_PROFILE=$BENCH_PROFILE"
echo "MAX_THREADS=$MAX_THREADS"
python3 plot_times.py
echo "===== Benchmarks done ====="

echo "===== Plot files ====="
ls -lh plot_bubble.png plot_mergesort.png plot_odd-even.png
echo "========================"
