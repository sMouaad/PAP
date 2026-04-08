#!/bin/bash
# Benchmark script for LBM MPI exercises
set -e

MPI_OPTS="--mca pml ob1 --mca btl tcp,self --bind-to core"
RUNS=3
OUTDIR="bench_results"

mkdir -p "$OUTDIR"

# extract minimum "Total time" from $RUNS runs
run_bench() {
    local np=$1
    local ex=$2
    local extra=$3
    local best=999999

    for r in $(seq 1 $RUNS); do
        t=$(mpirun $MPI_OPTS -np $np ./lbm -e $ex -n $extra 2>&1 | grep "Total time" | awk '{print $3}')
        if (( $(echo "$t < $best" | bc -l) )); then
            best=$t
        fi
    done
    echo "$best"
}

# strong scaling...
# scaling 16 => 4x each dim => 1600x320
echo "===== STRONG SCALING ====="
echo "np,ex1,ex2,ex3,ex4,ex5,ex6" > "$OUTDIR/strong.csv"

for np in 1 2 4 8 16; do
    line="$np"
    for ex in 1 2 3 4 5 6; do
        # 2D exercises need np to factor into 2 dims; skip if np=1 for 2D
        # (they'll just run as 1x1 which is fine for baseline)
        echo -n "  strong: np=$np ex=$ex ..."
        t=$(run_bench $np $ex "--scaling 16")
        echo " $t s"
        line="$line,$t"
    done
    echo "$line" >> "$OUTDIR/strong.csv"
done

echo ""
echo "===== WEAK SCALING ====="
# each process keeps 400x80 cells (scaling = np)
echo "np,ex1,ex2,ex3,ex4,ex5,ex6" > "$OUTDIR/weak.csv"

for np in 1 2 4 8 16; do
    line="$np"
    for ex in 1 2 3 4 5 6; do
        echo -n "  weak: np=$np ex=$ex ..."
        t=$(run_bench $np $ex "--scaling $np")
        echo " $t s"
        line="$line,$t"
    done
    echo "$line" >> "$OUTDIR/weak.csv"
done

echo ""
echo "Done. Results in $OUTDIR/strong.csv and $OUTDIR/weak.csv"
