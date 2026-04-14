import os
import subprocess
import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
from matplotlib.offsetbox import AnchoredText
import platform

if platform.system() == "Windows":
    EXECUTABLE_PATH = "../cmake-build-release-wsl/main_simulate"
else:
    EXECUTABLE_PATH = "../cmake-build-release/main_simulate"
DATA_DIR = "../data"
SYMBOLS_FILE = "../data/tickers.txt"
PLOT_DIR = "../plots"

def run_benchmark(rate: int = 50000, duration: int = 10):
    print("Executing C++ Baseline Benchmark (UDP/Spin)...")
    cmd = [
        EXECUTABLE_PATH,
        "--underlying", "custom",
        "--queue", "spin",
        "--size", "4096",
        "--transport", "udp",
        "--rate", str(rate),
        "--duration", str(duration),
        "--symbols", SYMBOLS_FILE,
        "--out", DATA_DIR
    ]
    subprocess.run(cmd, capture_output=True, text=True, check=True)

def plot_latencies(csv_path: str, label: str, output_dir: str):
    if not os.path.exists(csv_path):
        print(f"Skipping {label}: {csv_path} not found.")
        return

    print(f"\nProcessing {label} Latencies...")
    df = pd.read_csv(csv_path)

    df['queue_us'] = (df['queue_ns'] / 1000.0).replace(0, 0.001)
    df['network_us'] = (df['network_ns'] / 1000.0).replace(0, 0.001)
    df['total_us'] = (df['total_ns'] / 1000.0).replace(0, 0.001)

    df_melted = df[['queue_us', 'network_us']].rename(
        columns={'queue_us': 'Queue', 'network_us': 'Network'}
    ).melt(var_name='Component', value_name='Latency (µs)')

    stats = []
    for comp in ['Queue', 'Network']:
        d = df_melted[df_melted['Component'] == comp]['Latency (µs)']
        stats.append(f"{comp} Latency:")
        stats.append(f"  Mean: {d.mean():.3f} µs")
        stats.append(f"  Med:  {d.median():.3f} µs")
        stats.append(f"  p99:  {d.quantile(0.99):.3f} µs")
        stats.append(f"  Max:  {d.max():.3f} µs\n")

    stats.append(f"Total Pipeline:")
    stats.append(f"  Med:  {df['total_us'].median():.3f} µs")
    stats.append(f"  p99:  {df['total_us'].quantile(0.99):.3f} µs")

    stats_text = "\n".join(stats)
    print(stats_text)

    sns.set_theme(style="ticks", context="talk")
    fig, ax = plt.subplots(figsize=(14, 7))

    sns.histplot(
        data=df_melted,
        x="Latency (µs)",
        hue="Component",
        kde=True,
        element="step",
        log_scale=True,
        stat="density",
        common_norm=False,
        alpha=0.15,
        linewidth=2,
        palette=["#1f77b4", "#ff7f0e"],
        ax=ax
    )

    sns.move_legend(ax, "upper left", bbox_to_anchor=(1.02, 1), title="Pipeline Component", frameon=False)

    p999 = df['total_us'].quantile(0.999)
    min_val = df_melted['Latency (µs)'].min() * 0.8
    ax.set_xlim(min_val, p999 * 1.5)

    ax.set_title(f"Micro-Latency Breakdown: {label}", pad=20, fontweight='bold')
    ax.set_xlabel("Latency (Microseconds) - Log Scale", fontweight='bold')
    ax.set_ylabel("Probability Density", fontweight='bold')

    ax.grid(True, which="major", ls="-", alpha=0.3)
    ax.grid(True, which="minor", ls=":", alpha=0.1)
    sns.despine()

    anchored_text = AnchoredText(
        stats_text.strip(), loc='upper right',
        prop=dict(family='monospace', size=10),
        frameon=True
    )
    anchored_text.patch.set_boxstyle("round,pad=0.5,rounding_size=0.2")
    anchored_text.patch.set_alpha(0.9)
    anchored_text.patch.set_edgecolor('#cccccc')
    ax.add_artist(anchored_text)

    save_path = os.path.join(output_dir, f"{label.lower()}_latency_breakdown_kde.png")
    plt.tight_layout()
    plt.savefig(save_path, dpi=300, bbox_inches='tight')
    print(f"Plot saved successfully to {save_path}")
    plt.close()

def main():
    if not os.path.exists(PLOT_DIR):
        os.makedirs(PLOT_DIR)

    try:
        run_benchmark()
    except Exception as e:
        print(f"Error executing benchmark: {e}")
        return

    plot_latencies(os.path.join(DATA_DIR, "quote_latencies.csv"), "Quotes", PLOT_DIR)
    plot_latencies(os.path.join(DATA_DIR, "trade_latencies.csv"), "Trades", PLOT_DIR)

if __name__ == "__main__":
    main()