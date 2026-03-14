#!/usr/bin/env python3
import subprocess
import os
import matplotlib.pyplot as plt 
import re


def extract_elapsed_real_time(output: str):
    """Return elapsed_real_time in seconds from cpu_stats output, or None if not found."""
    # Example line:
    # bubble serial: {elapsed_cpu_time: 0.123456 s, elapsed_real_time: 0.234567 s}
    # or with cpu cycles before that field.
    for line in output.splitlines():
        if "elapsed_real_time" in line:
            m = re.search(r"elapsed_real_time:\s*([0-9]+(?:\.[0-9]+)?)\s*s", line)
            if m:
                return float(m.group(1))
    return None

def build_thread_list():
    """Build a power-of-two thread list up to MAX_THREADS, clipped to available CPUs.

    Override examples:
      MAX_THREADS=128 python3 plot_times.py
      THREAD_LIST=2,4,8,16,32,64,128 python3 plot_times.py
    """
    raw_list = os.getenv("THREAD_LIST", "").strip()
    if raw_list:
        values = sorted({int(x) for x in raw_list.split(",") if x.strip()})
        return [v for v in values if v >= 2]

    available = os.cpu_count() or 1
    max_threads = int(os.getenv("MAX_THREADS", "16"))
    max_threads = max(2, min(max_threads, available))

    threads = []
    t = 2
    while t <= max_threads:
        threads.append(t)
        t *= 2
    return threads


nr_threads_list = build_thread_list()

def run_and_plot(exec_name: str, start_range: int, end_range: int):
    print(f"Benchmarking: {exec_name}")
    
    time_serial = []
    time_parallel = []
    N_list = []

    ## SERIAL + PARALLEL with one one thread
    for i in range(start_range, end_range):
        N = i
        N_list.append(N)
        
        # use one thread
        my_env = os.environ.copy()
        my_env["ONLY_SERIAL"] = "true"
        if "ONLY_PARALLEL" in my_env:
            del my_env["ONLY_PARALLEL"]

        result = subprocess.run([exec_name, str(N)], stdout=subprocess.PIPE, stderr=subprocess.PIPE, env=my_env)
        output = result.stdout.decode('utf-8')
        t = extract_elapsed_real_time(output)
        if t is None:
            # Keep vectors aligned even when one run fails to print expected stats.
            time_serial.append(float('nan'))
            print(f"WARN: missing serial timing for N={N} in {exec_name}", flush=True)
        else:
            time_serial.append(t)
            print(f"{N};{2**N};{exec_name};serial;1;{t}", flush=True)

    ## PARALLEL with multiple threads
    for n_threads in nr_threads_list:
        my_env = os.environ.copy()
        # set the number of threads
        my_env["OMP_NUM_THREADS"] = str(n_threads)
        # don't run serial code and test code
        my_env["ONLY_PARALLEL"] = "true"
        if "ONLY_SERIAL" in my_env:
            del my_env["ONLY_SERIAL"]

        time_parallel_with_n_threads = []
        for i in range(start_range, end_range):
            N = i
            result = subprocess.run([exec_name, str(N)], stdout=subprocess.PIPE, stderr=subprocess.PIPE, env=my_env)
            output = result.stdout.decode('utf-8')
            t = extract_elapsed_real_time(output)
            if t is None:
                time_parallel_with_n_threads.append(float('nan'))
                print(f"WARN: missing parallel timing for N={N}, p={n_threads} in {exec_name}", flush=True)
            else:
                time_parallel_with_n_threads.append(t)
                print(f"{N};{2**N};{exec_name};parallel;{n_threads};{t}", flush=True)

        time_parallel.append(time_parallel_with_n_threads)

    # print(len(time_serial))
    # print(len(time_parallel))
    # print(list(map(len, time_parallel)))
    
    ## plot the results
    plt.plot(N_list, time_serial, label="Serial")
    for i in range(len(nr_threads_list)):
        plt.plot(N_list, time_parallel[i], label=f"Parallel {nr_threads_list[i]} threads")

    plt.xlabel('N')
    plt.ylabel('Time (s)')
    plt.yscale('log')
    plt.legend()
    plt.savefig(f'plot_{os.path.basename(exec_name).split(".")[0]}.png', dpi=200, bbox_inches='tight')
    plt.clf() 
if __name__ == "__main__":
    print(f"Using thread list: {nr_threads_list}", flush=True)
    run_and_plot("./bubble.run", 2, 16)
    run_and_plot("./mergesort.run", 10, 28)
    run_and_plot("./odd-even.run", 10, 19)
