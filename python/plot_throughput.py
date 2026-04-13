import os
import subprocess
import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import matplotlib.ticker as ticker

EXECUTABLE_PATH = "../cmake-build-release-wsl/main_simulate"
DATA_DIR = "../data"
SYMBOLS_FILE = "../data/tickers_WIN32.txt"

TARGET_RATES = [100_000, 500_000, 1_000_000, 1_500_000, 2_000_000, 2_500_000, 3_000_000]
DURATION = 5

def run_throughput_test(transport: str, rate: int) -> int:
    print(f"Testing {transport.upper()} at {rate:,} msgs/sec...")
    cmd = [
        EXECUTABLE_PATH,
        "--underlying", "custom",
        "--queue", "spin",
        "--size", "65536",
        "--transport", transport,
        "--rate", str(rate),
        "--duration", str(DURATION),
        "--symbols", SYMBOLS_FILE,
        "--out", DATA_DIR
    ]

    try:
        subprocess.run(cmd, capture_output=True, text=True, check=True)
    except subprocess.CalledProcessError as e:
        print(f"  -> Crash/Error on {transport} at {rate}: {e.stderr}")
        return 0

    total_received = 0

    q_file = os.path.join(DATA_DIR, "quote_latencies.csv")
    if os.path.exists(q_file):
        total_received += sum(1 for line in open(q_file)) - 1

    t_file = os.path.join(DATA_DIR, "trade_latencies.csv")
    if os.path.exists(t_file):
        total_received += sum(1 for line in open(t_file)) - 1

    return total_received

def main():
    results = []

    for transport in ['udp', 'zmq']:
        for rate in TARGET_RATES:
            received = run_throughput_test(transport, rate)
            achieved_rate = received / DURATION

            results.append({
                'Transport': transport.upper(),
                'Target Rate': rate,
                'Achieved Rate': achieved_rate
            })

    df = pd.DataFrame(results)
    print("\n--- Throughput Raw Data ---")
    for index, row in df.iterrows():
        print(f"Transport: {row['Transport']:<4} | Target: {row['Target Rate']:>9,} | Achieved: {row['Achieved Rate']:>9,.0f} msgs/sec")
    print("---------------------------\n")

    print("\n--- Throughput Results ---")
    print(df.to_string(index=False))

    sns.set_theme(style="whitegrid", context="talk")
    fig, ax = plt.subplots(figsize=(12, 7))

    max_rate = max(TARGET_RATES)
    ax.plot([0, max_rate], [0, max_rate], color='black', linestyle='--', linewidth=2, label='Ideal (0% Drops/Stalls)', alpha=0.6)

    sns.lineplot(
        data=df,
        x='Target Rate',
        y='Achieved Rate',
        hue='Transport',
        style='Transport',
        markers=['o', 's'],
        dashes=False,
        linewidth=3,
        markersize=10,
        palette=["#2ca02c", "#d62728"],
        ax=ax
    )

    ax.set_title("Maximum Throughput Analysis: UDP vs ZeroMQ", pad=20, fontweight='bold')
    ax.set_xlabel("Target Message Rate (msgs/sec)", fontweight='bold')
    ax.set_ylabel("Achieved Delivery Rate (msgs/sec)", fontweight='bold')

    formatter = ticker.FuncFormatter(lambda x, pos: f'{x*1e-6:.1f}M')
    ax.xaxis.set_major_formatter(formatter)
    ax.yaxis.set_major_formatter(formatter)

    ax.set_xlim(left=0)
    ax.set_ylim(bottom=0)

    sns.despine()
    ax.legend(title="Protocol", loc="upper left", frameon=True)

    output_filename = "../plots/throughput_analysis.png"
    plt.tight_layout()
    plt.savefig(output_filename, dpi=300)
    print(f"\nPlot saved successfully to {output_filename}")
    plt.show()

if __name__ == "__main__":
    main()