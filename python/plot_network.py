import os
import subprocess
import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
from matplotlib.offsetbox import AnchoredText

# --- Configuration ---
EXECUTABLE_PATH = "../cmake-build-release-wsl/main_simulate"
DATA_DIR = "../data"
SYMBOLS_FILE = "../data/tickers_WIN32.txt"

def run_benchmark(transport: str, rate: int = 50000, duration: int = 20) -> pd.DataFrame:
    print(f"Executing Benchmark for Transport: {transport.upper()}...")
    cmd = [
        EXECUTABLE_PATH,
        "--underlying", "custom",
        "--queue", "spin",     
        "--size", "4096",
        "--transport", transport,
        "--rate", str(rate),
        "--duration", str(duration),
        "--symbols", SYMBOLS_FILE,
        "--out", DATA_DIR
    ]

    subprocess.run(cmd, capture_output=True, text=True, check=True)

    q_file = os.path.join(DATA_DIR, "quote_latencies.csv")
    df = pd.read_csv(q_file)

    df['network_us'] = (df['network_ns'] / 1000.0).replace(0, 0.001)
    df['Transport'] = transport.upper()
    return df

def main():
    try:
        df_udp = run_benchmark('udp')
        df_zmq = run_benchmark('zmq')
    except Exception as e:
        print(f"Error running benchmarks: {e}")
        return

    df_all = pd.concat([df_udp, df_zmq], ignore_index=True)
    stats = []
    for transport in ['UDP', 'ZMQ']:
        d = df_all[df_all['Transport'] == transport]['network_us']
        stats.append(f"{transport} Network:")
        stats.append(f"  Mean: {d.mean():.3f} µs")
        stats.append(f"  Med:  {d.median():.3f} µs")
        stats.append(f"  p99:  {d.quantile(0.99):.3f} µs")
        stats.append(f"  Max:  {d.max():.3f} µs\n")
    stats_text = "\n".join(stats)
    print("\n--- Network Benchmark Results ---")
    print(stats_text)

    sns.set_theme(style="ticks", context="talk")
    fig, ax = plt.subplots(figsize=(14, 7))

    sns.histplot(
        data=df_all,
        x="network_us",
        hue="Transport",
        kde=True,
        element="step",
        log_scale=True,
        stat="density",
        common_norm=False,
        alpha=0.15,
        linewidth=2,
        palette=["#2ca02c", "#d62728"],
        ax=ax
    )

    sns.move_legend(ax, "upper left", bbox_to_anchor=(1.02, 1), title="Protocol", frameon=False)

    p999 = df_all['network_us'].quantile(0.999)
    min_val = df_all['network_us'].min() * 0.8
    ax.set_xlim(min_val, p999 * 1.5)

    ax.set_title("Network Latency: UDP Multicast vs. ZeroMQ TCP", pad=20, fontweight='bold')
    ax.set_xlabel("Network Latency (Microseconds) - Log Scale", fontweight='bold')
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

    # 7. Save and Show
    output_filename = "../plots/network_comparison_kde.png"
    plt.tight_layout()
    plt.savefig(output_filename, dpi=300, bbox_inches='tight')
    print(f"Plot saved successfully to {output_filename}")
    plt.show()

if __name__ == "__main__":
    main()