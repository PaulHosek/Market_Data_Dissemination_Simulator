import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import os

def plot_latencies(csv_path, label, output_dir):
    if not os.path.exists(csv_path):
        print(f"Skipping {label}: {csv_path} not found.")
        return

    df = pd.read_csv(csv_path)

    df['queue_us'] = df['queue_ns'] / 1000.0
    df['network_us'] = df['network_ns'] / 1000.0
    df['total_us'] = df['total_ns'] / 1000.0

    print(f"\n--- Statistics for {label} (Total Latency in µs) ---")
    print(df['total_us'].describe(percentiles=[.5, .9, .95, .99]))

    plt.figure(figsize=(12, 6))

    sns.kdeplot(df['queue_us'], fill=True, color="blue", label="Queue Wait Time", alpha=0.5)
    sns.kdeplot(df['network_us'], fill=True, color="orange", label="Network & Parsing Time", alpha=0.5)

    total_median = df['total_us'].median()
    total_99 = df['total_us'].quantile(0.99)
    plt.axvline(total_median, color='black', linestyle='--', label=f"Total Median: {total_median:.2f}µs")
    plt.axvline(total_99, color='red', linestyle='-', label=f"Total 99th %tile: {total_99:.2f}µs")

    plt.title(f'Micro-Latency Breakdown: {label}')
    plt.xlabel('Latency (microseconds)')
    plt.ylabel('Density')
    plt.legend()
    plt.grid(True, alpha=0.3)

    plt.xlim(0, total_99 * 1.5)

    save_path = os.path.join(output_dir, f"{label.lower()}_latency_breakdown.png")
    plt.savefig(save_path)
    print(f"Saved plot to {save_path}")
    plt.close()

if __name__ == "__main__":
    output_dir = "../plots"
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)

    plot_latencies("../data/quote_latencies.csv", "Quotes", output_dir)
    plot_latencies("../data/trade_latencies.csv", "Trades", output_dir)