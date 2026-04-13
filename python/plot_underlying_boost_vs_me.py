import os
import subprocess
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import seaborn as sns
from matplotlib.offsetbox import AnchoredText

# --- Configuration ---
EXECUTABLE_PATH = "../cmake-build-release-wsl/main_simulate"
DATA_DIR = "../data"
SYMBOLS_FILE = "../data/tickers_WIN32.txt"

def run_benchmark(underlying_queue: str, rate: int = 100000, duration: int = 10) -> pd.DataFrame:
    print(f"Executing C++ Benchmark for: {underlying_queue.upper()} Queue...")
    cmd = [
        EXECUTABLE_PATH,
        "--underlying", underlying_queue,
        "--queue", "spin",
        "--size", "4096",
        "--transport", "udp",
        "--rate", str(rate),
        "--duration", str(duration),
        "--symbols", SYMBOLS_FILE,
        "--out", DATA_DIR
    ]

    subprocess.run(cmd, capture_output=True, text=True, check=True)

    q_file = os.path.join(DATA_DIR, "quote_latencies.csv")
    df = pd.read_csv(q_file)

    df['queue_us'] = (df['queue_ns'] / 1000.0).replace(0, 0.001)
    df['Implementation'] = underlying_queue.capitalize()
    return df

def main():
    try:
        df_custom = run_benchmark('custom')
        df_boost = run_benchmark('boost')
    except Exception as e:
        print(f"Error running benchmarks: {e}")
        return

    df_all = pd.concat([df_custom, df_boost], ignore_index=True)

    stats = []
    for impl in ['Custom', 'Boost']:
        d = df_all[df_all['Implementation'] == impl]['queue_us']
        stats.append(f"{impl} Queue:")
        stats.append(f"  Mean: {d.mean():.3f} µs")
        stats.append(f"  Med:  {d.median():.3f} µs")
        stats.append(f"  p99:  {d.quantile(0.99):.3f} µs")
        stats.append(f"  Max:  {d.max():.3f} µs\n")
    stats_text = "\n".join(stats)
    print("\n--- Benchmark Results ---")
    print(stats_text)

    sns.set_theme(style="ticks", context="talk")
    fig, ax = plt.subplots(figsize=(14, 7))

    sns.histplot(
        data=df_all,
        x="queue_us",
        hue="Implementation",
        kde=True,
        log_scale=True,
        stat="density",
        common_norm=False,
        alpha=0.4,
        linewidth=1,
        palette=["#1f77b4", "#ff7f0e"],
        ax=ax
    )

    sns.move_legend(ax, "upper left", bbox_to_anchor=(1.02, 1), title="Implementation", frameon=False)

    p999 = df_all['queue_us'].quantile(0.99)
    min_val = df_all['queue_us'].min() * 0.8
    ax.set_xlim(min_val, p999 * 1.5)

    ax.set_title("Lock-Free Queue Internal Latency: Custom vs Boost (Zoomed to 99.9th %)", pad=20, fontweight='bold')
    ax.set_xlabel("Queue Latency (Microseconds) - Log Scale", fontweight='bold')
    ax.set_ylabel("Probability Density", fontweight='bold')

    ax.grid(True, which="major", ls="-", alpha=0.2)
    ax.grid(True, which="minor", ls=":", alpha=0.2)
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

    output_filename = "../plots/queue_benchmark_kde.png"
    plt.tight_layout()
    plt.savefig(output_filename, dpi=300, bbox_inches='tight')
    print(f"Plot saved successfully to {output_filename}")
    plt.show()

if __name__ == "__main__":
    main()