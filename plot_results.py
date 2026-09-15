import os
import re
import json
import matplotlib.pyplot as plt
import numpy as np


def parse_champsim_detailed(filepath):
    """
    Parses ChampSim statistics from an output log.
    Extracts execution, branch prediction, cache, and DRAM statistics.
    """

    if not os.path.exists(filepath):
        print(f"Warning: File {filepath} not found.")
        return None

    stats = {
        "file_name": os.path.basename(filepath),
        "execution": {},
        "branch_prediction": {},
        "caches": {},
        "dram": {}
    }

    with open(filepath, 'r') as f:
        for line in f:

            # ---------------------------------------------------------
            # Execution Stats
            # ---------------------------------------------------------
            if "CPU 0 cumulative IPC:" in line:
                ipc_m = re.search(r"cumulative IPC:\s+([0-9.]+)", line)
                inst_m = re.search(r"instructions:\s+(\d+)", line)
                cyc_m = re.search(r"cycles:\s+(\d+)", line)

                if ipc_m:
                    stats["execution"]["ipc"] = float(ipc_m.group(1))

                if inst_m:
                    stats["execution"]["instructions"] = int(inst_m.group(1))

                if cyc_m:
                    stats["execution"]["cycles"] = int(cyc_m.group(1))

            # ---------------------------------------------------------
            # Branch Prediction Stats
            # ---------------------------------------------------------
            if "CPU 0 Branch Prediction Accuracy:" in line:
                acc_m = re.search(r"Accuracy:\s+([0-9.]+)%", line)
                mpki_m = re.search(r"MPKI:\s+([0-9.]+)", line)
                rob_m = re.search(r"Occupancy at Mispredict:\s+([0-9.]+)", line)

                if acc_m:
                    stats["branch_prediction"]["accuracy_pct"] = float(
                        acc_m.group(1)
                    )

                if mpki_m:
                    stats["branch_prediction"]["mpki"] = float(
                        mpki_m.group(1)
                    )

                if rob_m:
                    stats["branch_prediction"]["avg_rob_occupancy"] = float(
                        rob_m.group(1)
                    )

            # ---------------------------------------------------------
            # Cache Statistics
            # ---------------------------------------------------------
            cache_match = re.search(
                r"cpu0->([A-Za-z0-9_]+)\s+"
                r"(TOTAL|LOAD|RFO|PREFETCH|WRITE|TRANSLATION)\s+ACCESS:\s+(\d+)\s+"
                r"HIT:\s+(\d+)\s+"
                r"MISS:\s+(\d+)\s+"
                r"MISS_MERGE:\s+(\d+)",
                line
            )

            if cache_match:
                c_name, access_type, access, hit, miss, miss_merge = cache_match.groups()

                if c_name not in stats["caches"]:
                    stats["caches"][c_name] = {}

                access_val = int(access)
                hit_val = int(hit)
                miss_val = int(miss)

                miss_rate = (
                    miss_val / access_val * 100
                    if access_val > 0
                    else 0.0
                )

                hit_rate = (
                    hit_val / access_val * 100
                    if access_val > 0
                    else 0.0
                )

                stats["caches"][c_name][access_type.lower()] = {
                    "access": access_val,
                    "hit": hit_val,
                    "miss": miss_val,
                    "miss_merge": int(miss_merge),
                    "hit_rate_pct": round(hit_rate, 4),
                    "miss_rate_pct": round(miss_rate, 4)
                }

            # ---------------------------------------------------------
            # Cache Average Miss Latency
            # ---------------------------------------------------------
            latency_match = re.search(
                r"cpu0->([A-Za-z0-9_]+)\s+"
                r"AVERAGE MISS LATENCY:\s+([0-9.]+|-)\s+cycles",
                line
            )

            if latency_match:
                c_name, lat_val = latency_match.groups()

                if c_name not in stats["caches"]:
                    stats["caches"][c_name] = {}

                stats["caches"][c_name]["avg_miss_latency_cycles"] = (
                    float(lat_val) if lat_val != '-' else None
                )

            # ---------------------------------------------------------
            # DRAM Metrics
            # ---------------------------------------------------------
            if "Channel 0 RQ ROW_BUFFER_HIT:" in line:
                m = re.search(r"ROW_BUFFER_HIT:\s+(\d+)", line)

                if m:
                    stats["dram"]["rq_row_buffer_hit"] = int(m.group(1))

            if (
                "ROW_BUFFER_MISS:" in line
                and "rq_row_buffer_hit" in stats["dram"]
                and "rq_row_buffer_miss" not in stats["dram"]
            ):
                m = re.search(r"ROW_BUFFER_MISS:\s+(\d+)", line)

                if m:
                    stats["dram"]["rq_row_buffer_miss"] = int(m.group(1))

    return stats


# =====================================================================
# Latest v3 result files
# =====================================================================

all_log_files = [

    # -------------------------------------------------------------
    # Plot 1: Hmmer, associativity sweep
    # -------------------------------------------------------------
    'v3_res_p1_lru_a4.txt',
    'v3_res_p1_lru_a8.txt',
    'v3_res_p1_lru_a16.txt',

    'v3_res_p1_hawkeye_a4.txt',
    'v3_res_p1_hawkeye_a8.txt',
    'v3_res_p1_hawkeye_a16.txt',

    # -------------------------------------------------------------
    # Plot 2: Default 16-way LLC
    # -------------------------------------------------------------
    'v3_res_p2_lru_hmmer.txt',
    'v3_res_p2_hawkeye_hmmer.txt',

    'v3_res_p2_lru_mcf.txt',
    'v3_res_p2_hawkeye_mcf.txt',

    'v3_res_p2_lru_astar.txt',
    'v3_res_p2_hawkeye_astar.txt'
]


# =====================================================================
# Parse all available files
# =====================================================================

parsed_data = {}

for fname in all_log_files:
    data = parse_champsim_detailed(fname)

    if data:
        parsed_data[fname] = data


# =====================================================================
# Save detailed JSON results
# =====================================================================

with open('champsim_detailed_results_v3.json', 'w') as f:
    json.dump(parsed_data, f, indent=2)

print("Saved detailed JSON results to: champsim_detailed_results_v3.json")


# =====================================================================
# Save human-readable report
# =====================================================================

with open('champsim_detailed_results_v3.txt', 'w') as f:

    f.write("========================================================\n")
    f.write("          CHAMPSIM EVALUATION DETAILED REPORT           \n")
    f.write("========================================================\n\n")

    for fname, data in parsed_data.items():

        f.write(f"--- File: {fname} ---\n")

        exec_st = data.get("execution", {})

        f.write(
            f"  IPC: {exec_st.get('ipc', 'N/A')}\n"
        )

        f.write(
            f"  Instructions: {exec_st.get('instructions', 'N/A')}\n"
        )

        f.write(
            f"  Cycles: {exec_st.get('cycles', 'N/A')}\n"
        )

        bp = data.get("branch_prediction", {})

        f.write(
            f"  Branch Predictor Accuracy: "
            f"{bp.get('accuracy_pct', 'N/A')}%\n"
        )

        f.write(
            f"  Branch MPKI: {bp.get('mpki', 'N/A')}\n"
        )

        llc = (
            data
            .get("caches", {})
            .get("LLC", {})
            .get("total", {})
        )

        f.write(
            f"  LLC Total Accesses: "
            f"{llc.get('access', 'N/A')}\n"
        )

        f.write(
            f"  LLC Hits: "
            f"{llc.get('hit', 'N/A')}\n"
        )

        f.write(
            f"  LLC Misses: "
            f"{llc.get('miss', 'N/A')}\n"
        )

        f.write(
            f"  LLC Miss Rate: "
            f"{llc.get('miss_rate_pct', 'N/A')}%\n"
        )

        llc_lat = (
            data
            .get("caches", {})
            .get("LLC", {})
            .get("avg_miss_latency_cycles", "N/A")
        )

        f.write(
            f"  LLC Avg Miss Latency: "
            f"{llc_lat} cycles\n"
        )

        f.write("\n" + "-" * 50 + "\n\n")


print("Saved text summary report to: champsim_detailed_results_v3.txt")


# =====================================================================
# Helper function
# =====================================================================

def get_val(fname, key_path):

    if fname not in parsed_data:
        print(f"Warning: {fname} was not parsed.")
        return 0.0

    curr = parsed_data[fname]

    for k in key_path:

        if isinstance(curr, dict) and k in curr:
            curr = curr[k]

        else:
            print(
                f"Warning: Could not find {'.'.join(key_path)} "
                f"in {fname}"
            )
            return 0.0

    return float(curr)


# =====================================================================
# Helper function specifically for LLC miss rate
# =====================================================================

def get_llc_miss_rate(fname):

    return get_val(
        fname,
        ["caches", "LLC", "total", "miss_rate_pct"]
    )


# =====================================================================
# Plot 1
#
# LLC miss rate vs LLC associativity
#
# Benchmark:
# 456.hmmer-191B.champsimtrace.xz
#
# Associativities:
# 4, 8, 16
#
# Fixed LLC capacity:
# 2 MB
# =====================================================================

assoc_keys = ['4', '8', '16']


lru_files_p1 = {
    '4': 'v3_res_p1_lru_a4.txt',
    '8': 'v3_res_p1_lru_a8.txt',
    '16': 'v3_res_p1_lru_a16.txt'
}


hawkeye_files_p1 = {
    '4': 'v3_res_p1_hawkeye_a4.txt',
    '8': 'v3_res_p1_hawkeye_a8.txt',
    '16': 'v3_res_p1_hawkeye_a16.txt'
}


# Get LLC miss rates

lru_miss_rates = [
    get_llc_miss_rate(lru_files_p1[a])
    for a in assoc_keys
]


hawkeye_miss_rates = [
    get_llc_miss_rate(hawkeye_files_p1[a])
    for a in assoc_keys
]


print("\n========================================================")
print("PLOT 1 DATA")
print("========================================================")

for i, assoc in enumerate(assoc_keys):

    print(
        f"{assoc}-way: "
        f"LRU = {lru_miss_rates[i]:.4f}% | "
        f"Hawkeye = {hawkeye_miss_rates[i]:.4f}%"
    )


# Create Plot 1

plt.figure(figsize=(8, 6))

plt.plot(
    assoc_keys,
    lru_miss_rates,
    marker='o',
    linewidth=2,
    label='LRU'
)

plt.plot(
    assoc_keys,
    hawkeye_miss_rates,
    marker='s',
    linewidth=2,
    label='Hawkeye'
)

plt.title(
    'LLC Miss Rate vs. Associativity\n'
    '456.hmmer-191B'
)

plt.xlabel('LLC Associativity (ways)')

plt.ylabel('LLC Miss Rate (%)')

plt.xticks(assoc_keys)

plt.grid(
    True,
    linestyle='--',
    alpha=0.6
)

plt.legend()

plt.tight_layout()

plt.savefig(
    'plot1_llc_miss_rate_vs_associativity_v3.png',
    dpi=300
)

plt.close()

print(
    "\nPlot 1 saved as: "
    "plot1_llc_miss_rate_vs_associativity_v3.png"
)


# =====================================================================
# Plot 2
#
# LLC miss-rate reduction over LRU
#
# Default LLC:
#   2 MB
#   16-way
#   2048 sets
#
# Formula:
#
# (LRU miss rate - Hawkeye miss rate)
# ----------------------------------- * 100
#          LRU miss rate
# =====================================================================


traces = [
    '456.hmmer',
    '429.mcf',
    '473.astar'
]


lru_files_p2 = [
    'v3_res_p2_lru_hmmer.txt',
    'v3_res_p2_lru_mcf.txt',
    'v3_res_p2_lru_astar.txt'
]


hawkeye_files_p2 = [
    'v3_res_p2_hawkeye_hmmer.txt',
    'v3_res_p2_hawkeye_mcf.txt',
    'v3_res_p2_hawkeye_astar.txt'
]


# Get miss rates

p2_lru_miss_rates = [
    get_llc_miss_rate(fname)
    for fname in lru_files_p2
]


p2_hawkeye_miss_rates = [
    get_llc_miss_rate(fname)
    for fname in hawkeye_files_p2
]


# =====================================================================
# Calculate miss-rate reduction
# =====================================================================

miss_rate_reduction = []

for lru_rate, hawkeye_rate in zip(
    p2_lru_miss_rates,
    p2_hawkeye_miss_rates
):

    if lru_rate != 0:

        reduction = (
            (lru_rate - hawkeye_rate)
            / lru_rate
            * 100
        )

    else:

        reduction = 0.0

    miss_rate_reduction.append(reduction)


# =====================================================================
# Print Plot 2 data
# =====================================================================

print("\n========================================================")
print("PLOT 2 DATA")
print("========================================================")

for i, trace in enumerate(traces):

    print(
        f"{trace}: "
        f"LRU miss rate = {p2_lru_miss_rates[i]:.4f}%, "
        f"Hawkeye miss rate = {p2_hawkeye_miss_rates[i]:.4f}%, "
        f"reduction = {miss_rate_reduction[i]:.4f}%"
    )


# =====================================================================
# Create Plot 2
# =====================================================================

x = np.arange(len(traces))

plt.figure(figsize=(8, 6))

bars = plt.bar(
    x,
    miss_rate_reduction,
    width=0.55
)


plt.title(
    'LLC Miss-Rate Reduction over LRU\n'
    '(Default 2 MB, 16-Way LLC)'
)

plt.xlabel('Benchmark')

plt.ylabel('Miss-Rate Reduction over LRU (%)')

plt.xticks(
    x,
    traces
)

plt.grid(
    axis='y',
    linestyle='--',
    alpha=0.6
)


# Add percentage values above bars

for bar, value in zip(
    bars,
    miss_rate_reduction
):

    plt.text(
        bar.get_x() + bar.get_width() / 2,
        bar.get_height(),
        f'{value:.2f}%',
        ha='center',
        va='bottom'
    )


plt.tight_layout()

plt.savefig(
    'plot2_llc_miss_rate_reduction_v3.png',
    dpi=300
)

plt.close()


print(
    "\nPlot 2 saved as: "
    "plot2_llc_miss_rate_reduction_v3.png"
)


# =====================================================================
# Final summary
# =====================================================================

print("\n========================================================")
print("FINAL RESULTS")
print("========================================================")

print("\nPlot 1: LLC Miss Rate vs Associativity")

for i, assoc in enumerate(assoc_keys):

    print(
        f"  {assoc}-way | "
        f"LRU: {lru_miss_rates[i]:.4f}% | "
        f"Hawkeye: {hawkeye_miss_rates[i]:.4f}%"
    )


print("\nPlot 2: Miss-Rate Reduction over LRU")

for i, trace in enumerate(traces):

    print(
        f"  {trace}: "
        f"{miss_rate_reduction[i]:.4f}%"
    )

print("\nDone.")