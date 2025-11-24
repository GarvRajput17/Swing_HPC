import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

# 1. Load the Data
try:
    df = pd.read_csv("benchmark_results.csv")
except FileNotFoundError:
    print("Error: benchmark_results.csv not found. Run the C++ simulation first.")
    exit()

# Convert Bytes to more readable labels for the X-axis later
def format_bytes(size):
    power = 2**10
    n = size
    power_labels = {0 : '', 1: 'K', 2: 'M', 3: 'G', 4: 'T'}
    count = 0
    while n > power:
        n /= power
        count += 1
    return f"{int(n)}{power_labels[count]}B"

# Get unique algorithms
algorithms = df['Algorithm'].unique()
colors = plt.cm.tab10(np.linspace(0, 1, len(algorithms)))

# --- PLOT 1: Latency (Time vs Size) ---
plt.figure(figsize=(12, 6))
plt.title("Allreduce Latency (Lower is Better)")
plt.xlabel("Message Size (Bytes)")
plt.ylabel("Time (microseconds)")

for algo, color in zip(algorithms, colors):
    subset = df[df['Algorithm'] == algo]
    plt.plot(subset['VectorSizeBytes'], subset['Time_us'], marker='o', label=algo, color=color)

plt.xscale('log', base=2)
plt.yscale('log') # Log scale is usually better for latency spanning orders of magnitude
plt.grid(True, which="both", ls="-", alpha=0.2)
plt.legend()
plt.tight_layout()
plt.savefig("graph_latency.png")
print("Generated graph_latency.png")

# --- PLOT 2: Goodput (Bandwidth vs Size) ---
plt.figure(figsize=(12, 6))
plt.title("Allreduce Goodput (Higher is Better)")
plt.xlabel("Message Size (Bytes)")
plt.ylabel("Goodput (Gbps)")

for algo, color in zip(algorithms, colors):
    subset = df[df['Algorithm'] == algo]
    plt.plot(subset['VectorSizeBytes'], subset['Goodput_Gbps'], marker='s', label=algo, color=color)

plt.xscale('log', base=2)
# We don't usually log-scale the Y-axis for Bandwidth, but you can if ranges are extreme
plt.grid(True, which="both", ls="-", alpha=0.2)
plt.legend()
plt.tight_layout()
plt.savefig("graph_goodput.png")
print("Generated graph_goodput.png")