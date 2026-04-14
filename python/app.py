import streamlit as st
import pandas as pd
import subprocess
import os
import plotly.express as px

# --- Configuration ---
EXECUTABLE_PATH = "../cmake-build-release-wsl/main_simulate"
DATA_DIR = "../data"
SYMBOLS_FILE = "../data/tickers.txt"

st.set_page_config(page_title="Benchmark", layout="wide")
st.set_page_config(page_title="Benchmark", layout="wide")
# --- Session State Initialization ---
if 'run_history' not in st.session_state:
    st.session_state.run_history = pd.DataFrame(columns=[
        "Run ID", "Transport", "Queue", "Size", "Rate", "Duration",
        "Median (µs)", "Mean (µs)", "p99 (µs)", "Max (µs)", "Jitter (µs)", "Tail Ratio", "Total Msgs"
    ])

if 'raw_data_history' not in st.session_state:
    st.session_state.raw_data_history = {}

# --- Execution Helper Function ---
def execute_benchmark(t_proto, q_strat, q_size, m_rate, m_dur):
    os.makedirs(DATA_DIR, exist_ok=True)

    # Unique ID generation
    base_run_id = f"{t_proto.upper()}_{q_strat.capitalize()}_{q_size}_{m_rate//1000}k"
    run_id = base_run_id
    counter = 1
    while run_id in st.session_state.raw_data_history:
        run_id = f"{base_run_id}_v{counter}"
        counter += 1

    cmd = [
        EXECUTABLE_PATH,
        "--queue", q_strat,
        "--size", str(q_size),
        "--transport", t_proto,
        "--rate", str(m_rate),
        "--duration", str(m_dur),
        "--symbols", SYMBOLS_FILE,
        "--out", DATA_DIR
    ]

    with st.spinner(f"Executing: {t_proto.upper()} + {q_strat.capitalize()}..."):
        try:
            subprocess.run(cmd, capture_output=True, text=True, check=True)
        except subprocess.CalledProcessError as e:
            st.error(f"Benchmark execution failed for {run_id}.")
            st.markdown("**Command executed:**")
            st.code(" ".join(cmd))
            st.markdown("**C++ Output (stdout & stderr):**")
            st.code(f"STDOUT:\n{e.stdout}\n\nSTDERR:\n{e.stderr}")
            return
        except FileNotFoundError:
            st.error(f"Executable not found at {EXECUTABLE_PATH}. Please verify the build path.")
            return

    # Ingest Output
    q_file = os.path.join(DATA_DIR, "quote_latencies.csv")
    if os.path.exists(q_file):
        df = pd.read_csv(q_file)
        if not df.empty:
            df['queue_us'] = df['queue_ns'] / 1000.0
            df['network_us'] = df['network_ns'] / 1000.0
            df['total_us'] = df['total_ns'] / 1000.0
            df['Run ID'] = run_id
            df['Transport'] = t_proto.upper()
            df['Queue'] = q_strat.capitalize()

            median_val = df['total_us'].median()
            p99_val = df['total_us'].quantile(0.99)
            tail_ratio = p99_val / median_val if median_val > 0 else 0

            new_metric_row = {
                "Run ID": run_id,
                "Transport": t_proto.upper(),
                "Queue": q_strat.capitalize(),
                "Size": q_size,
                "Rate": m_rate,
                "Duration": m_dur,
                "Median (µs)": round(median_val, 2),
                "Mean (µs)": round(df['total_us'].mean(), 2),
                "p99 (µs)": round(p99_val, 2),
                "Max (µs)": round(df['total_us'].max(), 2),
                "Jitter (µs)": round(df['total_us'].std(), 2),
                "Tail Ratio": round(tail_ratio, 2),
                "Total Msgs": len(df)
            }

            st.session_state.run_history = pd.concat(
                [st.session_state.run_history, pd.DataFrame([new_metric_row])],
                ignore_index=True
            )
            st.session_state.raw_data_history[run_id] = df
        else:
            st.warning(f"Execution succeeded for {run_id}, but output data is empty.")

# --- Header ---
st.title("Disseminator Benchmark")
st.markdown("Analyze and compare the latency profiles of lock-free queue wait strategies and network transports.")

# --- Sidebar: Control Panel ---
with st.sidebar:
    st.header("1. Configuration")

    transport = st.selectbox("Transport Protocol", ["udp", "zmq"], index=0)
    queue_type = st.selectbox("Wait Strategy", ["spin", "waitable"], index=0)
    queue_size = st.selectbox("Queue Size", [128, 512, 1024, 4096, 16384, 65536], index=3)
    rate = st.number_input("Message Rate (msgs/sec)", min_value=1000, max_value=1000000, value=50000, step=10000)
    duration = st.slider("Duration (seconds)", min_value=1, max_value=60, value=5)

    st.divider()
    st.header("2. Execution")

    run_single = st.button("Run Selected Configuration", use_container_width=True)
    run_all = st.button("Run ALL 4 Combinations", type="primary", use_container_width=True,
                        help="Runs UDP+Spin, UDP+Waitable, ZMQ+Spin, ZMQ+Waitable sequentially.")

    st.divider()
    st.header("3. Chart Settings")
    log_y = st.checkbox("Logarithmic Y-Axis", value=False)
    hist_mode = st.selectbox("Histogram Mode", ["count", "probability density"], index=0)

    st.divider()
    if st.button("Clear History", use_container_width=True):
        st.session_state.run_history = st.session_state.run_history.iloc[0:0]
        st.session_state.raw_data_history = {}
        st.rerun()

# --- Handle Button Clicks ---
if run_single:
    execute_benchmark(transport, queue_type, queue_size, rate, duration)

if run_all:
    for t in ["udp", "zmq"]:
        for q in ["spin", "waitable"]:
            execute_benchmark(t, q, queue_size, rate, duration)
    st.success("All 4 combinations executed successfully!")

# --- Data Visualization ---
tab1, tab2 = st.tabs(["Comparative Analysis (All Runs)", "Detailed Breakdown (Specific Run)"])

with tab1:
    if not st.session_state.run_history.empty:
        st.dataframe(st.session_state.run_history, use_container_width=True, hide_index=True)
        all_data = pd.concat(st.session_state.raw_data_history.values(), ignore_index=True)

        c1, c2 = st.columns([1, 3])
        with c1:
            color_by = st.radio("Group/Color comparisons by:", ["Run ID", "Transport", "Queue"])

        st.divider()
        col_box, col_cdf = st.columns(2)

        with col_box:
            st.subheader("Latency Distribution (Box Plot)")
            fig_compare = px.box(
                all_data, x="Run ID", y="total_us", color=color_by, points=False,
                labels={'total_us': 'Total Latency (µs)'}
            )
            if log_y:
                fig_compare.update_yaxes(type="log")
            st.plotly_chart(fig_compare, use_container_width=True)

        with col_cdf:
            st.subheader("Latency CDF")
            fig_cdf = px.ecdf(
                all_data, x="total_us", color=color_by,
                labels={'total_us': 'Latency (µs)', 'probability': 'Probability (%)'}
            )
            if log_y:
                fig_cdf.update_xaxes(type="log")
            st.plotly_chart(fig_cdf, use_container_width=True)

    else:
        st.info("No benchmark runs recorded in this session. Execute a run from the sidebar.")

with tab2:
    if st.session_state.raw_data_history:
        # Give the user a dropdown to select which historical run to view
        available_runs = list(st.session_state.raw_data_history.keys())
        selected_run = st.selectbox("Select a Run to Inspect", available_runs, index=len(available_runs)-1)

        df_selected = st.session_state.raw_data_history[selected_run]

        st.subheader(f"Internal Breakdown: {selected_run}")

        col_hist, col_split = st.columns(2)

        with col_hist:
            fig_hist = px.histogram(
                df_selected, x="total_us", nbins=150, histnorm=hist_mode if hist_mode != "count" else None,
                title=f"Total Latency Histogram ({hist_mode.title()})",
                labels={'total_us': 'Total Latency (µs)'},
                color_discrete_sequence=['#2E86C1']
            )
            fig_hist.update_layout(bargap=0.05)
            st.plotly_chart(fig_hist, use_container_width=True)

        with col_split:
            df_melted = df_selected[['queue_us', 'network_us']].melt(var_name='Component', value_name='Latency (µs)')
            fig_split_box = px.box(
                df_melted, x="Component", y="Latency (µs)",
                title="Queue vs. Network Breakdown"
            )
            if log_y:
                fig_split_box.update_yaxes(type="log")
            st.plotly_chart(fig_split_box, use_container_width=True)
    else:
        st.info("Execute a benchmark to view detailed visualizations.")