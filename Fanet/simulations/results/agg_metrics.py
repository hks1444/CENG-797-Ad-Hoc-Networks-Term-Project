#!/usr/bin/env python3
"""
Aggregate OMNeT++ scavetool CSV-R export across multiple runs.

How to Run:
    1) Extract values:
        opp_scavetool export \
        -F CSV-R \
        -o all_metrics.csv \
        -f 'type=~vector AND ("rlReward" OR "e2eDelayPeriodicSec" OR "availability")' \
        *.vec
    2) Run:
        python3 agg_metrics.py all_metrics.csv --time-ms --delay-ms --n-runs 5
Metrics:
  - delay: from vector name e2eDelayPeriodicSec
  - availability: Availability = fraction of hosts that are end-to-end reachable and functional at time t, inferred solely from delay existence.
  - rlReward: from vector name rlReward

Workflow:
  1) Expand vectors to long format (rep, host, t, value).
  2) Per-run aggregation at each t over hosts (mean).
  3) Overall aggregation at each t across runs (mean + 95% CI using t critical).

Outputs (out/):
  long_delay.csv                  (rep,host,t,delay_sec)
  long_availability.csv           (rep,host,t,availability)
  long_rlreward.csv               (rep,host,t,rlReward)
  per_run_delay.csv               (rep,t,mean_delay_sec,n_hosts_used)
  per_run_availability.csv        (rep,t,mean_availability,n_hosts_used)
  per_run_rlreward.csv            (rep,t,mean_rlReward,n_hosts_used)
  overall_delay.csv               (t,mean,ci95_low,ci95_high,n_runs_used)    # delay in ms if --delay-ms
  overall_availability.csv        (t,mean,ci95_low,ci95_high,n_runs_used)
  overall_rlreward.csv            (t,mean,ci95_low,ci95_high,n_runs_used)
  plot_delay.png
  plot_availability.png
  plot_rlreward.png
"""

import argparse
import math
import re
from pathlib import Path

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

RUN_REP_RE = re.compile(r"^[^-]+-(\d+)-")  # "General-0-20251221-..."
HOST_RE = re.compile(r"host\[(\d+)\]")

def parse_rep(run_str: str) -> int | None:
    m = RUN_REP_RE.match(str(run_str))
    return int(m.group(1)) if m else None

def parse_host(module_str: str) -> int | None:
    m = HOST_RE.search(str(module_str))
    return int(m.group(1)) if m else None

def parse_vec_field(s: str) -> np.ndarray:
    # vectime/vecvalue are space-separated; may include "NaN"
    if s is None or (isinstance(s, float) and np.isnan(s)):
        return np.array([], dtype=float)
    s = str(s).strip()
    if not s:
        return np.array([], dtype=float)
    parts = s.split()
    out = np.empty(len(parts), dtype=float)
    for i, p in enumerate(parts):
        if p.lower() == "nan":
            out[i] = np.nan
        else:
            out[i] = float(p)
    return out

def tcrit_975(df: int) -> float:
    # 97.5% quantile for Student-t. Hardcode common dfs; fallback to normal approx.
    table = {
        1: 12.706204736432095,
        2: 4.302652729911275,
        3: 3.182446305284263,
        4: 2.7764451051977987,
        5: 2.570581835636305,
        6: 2.4469118511449692,
        7: 2.3646242515927844,
        8: 2.306004135204166,
        9: 2.2621571627409915,
        10: 2.2281388519649385,
        15: 2.131449545559323,
        20: 2.0859634472658364,
        25: 2.059538552753294,
        30: 2.0422724563012373,
        60: 2.0002978210582616,
    }
    if df in table:
        return table[df]
    return 1.959963984540054

def clamp0(x: float) -> float:
    return max(0.0, x)

def expand_vectors(dfv: pd.DataFrame, value_col_name: str) -> pd.DataFrame:
    """
    Expand vectime/vecvalue rows to long format:
      rep, host, t, <value_col_name>
    Keeps NaNs in values (caller decides what to do).
    """
    records = []
    for _, row in dfv.iterrows():
        t = parse_vec_field(row["vectime"])
        y = parse_vec_field(row["vecvalue"])
        if len(t) == 0 or len(y) == 0:
            continue
        n = min(len(t), len(y))
        t = t[:n]
        y = y[:n]
        rep = int(row["rep"])
        host = int(row["host"])
        for ti, yi in zip(t, y):
            records.append((rep, host, float(ti), float(yi)))
    return pd.DataFrame(records, columns=["rep", "host", "t", value_col_name])

def per_run_mean(long_df: pd.DataFrame, value_col: str, out_value_col: str) -> pd.DataFrame:
    """
    Mean over hosts at each (rep,t). Drops NaN in value_col.
    """
    return (
        long_df.replace([np.inf, -np.inf], np.nan)
               .dropna(subset=["t"])
               .dropna(subset=[value_col])
               .groupby(["rep", "t"], as_index=False)
               .agg(**{out_value_col: (value_col, "mean"),
                       "n_hosts_used": (value_col, "count")})
               .sort_values(["rep", "t"])
    )

def overall_mean_ci(per_run_df: pd.DataFrame, per_run_value_col: str, clamp_low_to_zero: bool) -> pd.DataFrame:
    """
    For each t: mean across runs of per-run means + 95% CI using t critical.
    Each run counts equally.
    """
    overall = (
        per_run_df.groupby("t", as_index=False)
                  .agg(mean=(per_run_value_col, "mean"),
                       sd_across_runs=(per_run_value_col, "std"),
                       n_runs_used=(per_run_value_col, "count"))
                  .sort_values("t")
    )

    ci_low = []
    ci_high = []
    for _, r in overall.iterrows():
        n = int(r["n_runs_used"])
        m = float(r["mean"])
        sd = float(r["sd_across_runs"]) if not np.isnan(r["sd_across_runs"]) else 0.0
        if n <= 1:
            ci_low.append(np.nan)
            ci_high.append(np.nan)
            continue
        se = sd / math.sqrt(n)
        tcrit = tcrit_975(n - 1)
        lo = m - tcrit * se
        hi = m + tcrit * se
        if clamp_low_to_zero:
            lo = clamp0(lo)
            hi = max(lo, hi)  # keep ordering sane
        ci_low.append(lo)
        ci_high.append(hi)

    overall["ci95_low"] = ci_low
    overall["ci95_high"] = ci_high
    return overall

def plot_with_ci(overall_df: pd.DataFrame, x_label: str, y_label: str, title: str, out_path: Path):
    x = overall_df["t"].to_numpy()
    y = overall_df["mean"].to_numpy()
    lo = overall_df["ci95_low"].to_numpy()
    hi = overall_df["ci95_high"].to_numpy()

    plt.figure()
    plt.plot(x, y, label="mean across runs")
    if np.isfinite(lo).any() and np.isfinite(hi).any():
        plt.fill_between(x, lo, hi, alpha=0.2, label="95% CI")
    plt.xlabel(x_label)
    plt.ylabel(y_label)
    plt.title(title)
    plt.legend()
    plt.tight_layout()
    plt.savefig(out_path, dpi=200)

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("csv", help="scavetool export CSV-R file")
    ap.add_argument("--out", default="out", help="output directory")
    ap.add_argument("--delay-name", default="e2eDelayPeriodicSec", help="delay vector name")
    ap.add_argument("--reward-name", default="rlReward", help="reward vector name")
    ap.add_argument("--reps", default="", help="comma-separated repetition numbers to keep (default: auto-detect first 5)")
    ap.add_argument("--n-runs", type=int, default=5, help="how many distinct reps to keep if --reps not given")
    ap.add_argument("--exclude-hosts", default="3", help="comma-separated host indices to exclude")
    ap.add_argument("--drop-nonpositive-delay", action="store_true", help="treat delay<=0 as missing (NaN)")
    ap.add_argument("--time-ms", action="store_true", help="convert t from seconds to milliseconds")
    ap.add_argument("--delay-ms", action="store_true", help="convert delay from seconds to milliseconds for output/plot")
    args = ap.parse_args()

    in_path = Path(args.csv)
    out_dir = Path(args.out)
    out_dir.mkdir(parents=True, exist_ok=True)

    df = pd.read_csv(in_path)

    # only vectors
    dfv = df[df["type"] == "vector"].copy()
    if dfv.empty:
        raise SystemExit("No vector rows found (type != 'vector' everywhere).")

    dfv["rep"] = dfv["run"].map(parse_rep)
    dfv["host"] = dfv["module"].map(parse_host)
    dfv = dfv.dropna(subset=["rep", "host", "vectime", "vecvalue"])

    # pick reps
    if args.reps.strip():
        keep_reps = sorted({int(x) for x in args.reps.split(",") if x.strip()})
    else:
        reps = sorted({int(x) for x in dfv["rep"].unique()})
        keep_reps = reps[: max(1, args.n_runs)]

    dfv = dfv[dfv["rep"].isin(keep_reps)].copy()

    # exclude hosts
    exclude_hosts = {int(x) for x in args.exclude_hosts.split(",") if x.strip()}
    if exclude_hosts:
        dfv = dfv[~dfv["host"].isin(exclude_hosts)].copy()

    # --- delay long ---
    df_delay = dfv[dfv["name"] == args.delay_name].copy()
    if df_delay.empty:
        raise SystemExit(f"No delay vectors found with name={args.delay_name}")

    long_delay = expand_vectors(df_delay, "delay_sec")
    long_delay = long_delay.replace([np.inf, -np.inf], np.nan).dropna(subset=["t"])
    if args.drop_nonpositive_delay:
        long_delay.loc[long_delay["delay_sec"] <= 0, "delay_sec"] = np.nan

    # derive availability from delay validity (finite => 1, else 0)
    long_avail = long_delay[["rep", "host", "t"]].copy()
    long_avail["availability"] = np.isfinite(long_delay["delay_sec"].to_numpy()).astype(float)

    # --- rlReward long ---
    df_reward = dfv[dfv["name"] == args.reward_name].copy()
    long_reward = pd.DataFrame(columns=["rep", "host", "t", "rlReward"])
    if not df_reward.empty:
        long_reward = expand_vectors(df_reward, "rlReward")
        long_reward = long_reward.replace([np.inf, -np.inf], np.nan).dropna(subset=["t"])

    # time unit conversion (x axis)
    if args.time_ms:
        long_delay["t"] *= 1000.0
        long_avail["t"] *= 1000.0
        if not long_reward.empty:
            long_reward["t"] *= 1000.0

    long_delay = long_delay[long_delay["t"] >= 5000]
    long_reward = long_reward[long_reward["t"] >= 5000]

    # save longs
    long_delay.to_csv(out_dir / "long_delay.csv", index=False)
    long_avail.to_csv(out_dir / "long_availability.csv", index=False)
    if not long_reward.empty:
        long_reward.to_csv(out_dir / "long_rlreward.csv", index=False)

    # per-run means
    per_run_delay = per_run_mean(long_delay, "delay_sec", "mean_delay_sec")
    per_run_avail = per_run_mean(long_avail, "availability", "mean_availability")
    per_run_delay.to_csv(out_dir / "per_run_delay.csv", index=False)
    per_run_avail.to_csv(out_dir / "per_run_availability.csv", index=False)

    per_run_reward = pd.DataFrame(columns=["rep", "t", "mean_rlReward", "n_hosts_used"])
    if not long_reward.empty:
        per_run_reward = per_run_mean(long_reward, "rlReward", "mean_rlReward")
        per_run_reward.to_csv(out_dir / "per_run_rlreward.csv", index=False)

    # overall means + CI (clamp low >= 0 for delay + availability)
    overall_delay = overall_mean_ci(per_run_delay, "mean_delay_sec", clamp_low_to_zero=True)
    overall_avail = overall_mean_ci(per_run_avail, "mean_availability", clamp_low_to_zero=True)

    # unit conversion for delay y-axis
    if args.delay_ms:
        overall_delay["mean"] *= 1000.0
        overall_delay["ci95_low"] *= 1000.0
        overall_delay["ci95_high"] *= 1000.0

    overall_delay.to_csv(out_dir / "overall_delay.csv", index=False)
    overall_avail.to_csv(out_dir / "overall_availability.csv", index=False)

    overall_reward = pd.DataFrame(columns=["t", "mean", "ci95_low", "ci95_high", "n_runs_used"])
    if not long_reward.empty and not per_run_reward.empty:
        overall_reward = overall_mean_ci(per_run_reward, "mean_rlReward", clamp_low_to_zero=False)
        overall_reward.to_csv(out_dir / "overall_rlreward.csv", index=False)

    # plots (3 separate graphs)
    xlab = "t (ms)" if args.time_ms else "t (s)"

    ylab_delay = "Delay (ms)" if args.delay_ms else "Delay (s)"
    plot_with_ci(
        overall_delay,
        x_label=xlab,
        y_label=ylab_delay,
        title=f"{args.delay_name}: mean across runs (reps={keep_reps})",
        out_path=out_dir / "plot_delay.png",
    )

    plot_with_ci(
        overall_avail,
        x_label=xlab,
        y_label="Availability (fraction)",
        title=f"availability (derived from {args.delay_name} NaN/valid): mean across runs (reps={keep_reps})",
        out_path=out_dir / "plot_availability.png",
    )

    if not overall_reward.empty:
        plot_with_ci(
            overall_reward,
            x_label=xlab,
            y_label="rlReward",
            title=f"{args.reward_name}: mean across runs (reps={keep_reps})",
            out_path=out_dir / "plot_rlreward.png",
        )

if __name__ == "__main__":
    main()
