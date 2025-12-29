#!/usr/bin/env python3
"""
Aggregate OMNeT++ scavetool CSV-R export across multiple runs.

opp_scavetool export \
  -F CSV-R \
  -o all_metrics.csv \
  -f 'type=~vector AND ("rlReward" OR "e2eDelayPeriodicSec" OR "availability" OR "Ec" OR "Rc" OR "globalNumClusterHeads" OR "TotalClusterMsgSent")' \
  *.vec

python3 agg_metrics.py all_metrics.csv \
  --n-runs 5 \
  --time-ms \
  --delay-ms \
  --t-min 5000
Supports vectors:
- e2eDelayPeriodicSec (delay)
- availability (derived from delay existence)
- rlReward
- Ec
- Rc
- globalNumClusterHeads
- TotalClusterMsgSent
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
    return table.get(df, 1.959963984540054)

def clamp0(x: float) -> float:
    return max(0.0, x)

def expand_vectors(dfv: pd.DataFrame, value_col_name: str) -> pd.DataFrame:
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
        tc = tcrit_975(n - 1)
        lo = m - tc * se
        hi = m + tc * se
        if clamp_low_to_zero:
            lo = clamp0(lo)
            hi = max(lo, hi)
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

def process_metric(dfv: pd.DataFrame,
                   vec_name: str,
                   value_col: str,
                   per_run_col: str,
                   out_dir: Path,
                   keep_reps: list[int],
                   time_ms: bool,
                   t_min: float | None,
                   clamp_low_to_zero: bool,
                   y_scale: float = 1.0,
                   y_label: str = "",
                   plot_file: str = ""):
    """
    Generic pipeline:
      vectors -> long -> (optional time scaling/filter) -> per-run -> overall -> csv + plot
    """
    dfm = dfv[dfv["name"] == vec_name].copy()
    if dfm.empty:
        return None, None, None

    long_df = expand_vectors(dfm, value_col).replace([np.inf, -np.inf], np.nan).dropna(subset=["t"])

    if time_ms:
        long_df["t"] *= 1000.0

    if t_min is not None:
        long_df = long_df[long_df["t"] >= t_min].copy()

    long_df.to_csv(out_dir / f"long_{value_col}.csv", index=False)

    per_run = per_run_mean(long_df, value_col, per_run_col)
    per_run.to_csv(out_dir / f"per_run_{value_col}.csv", index=False)

    overall = overall_mean_ci(per_run, per_run_col, clamp_low_to_zero=clamp_low_to_zero)

    if y_scale != 1.0:
        overall["mean"] *= y_scale
        overall["ci95_low"] *= y_scale
        overall["ci95_high"] *= y_scale

    overall.to_csv(out_dir / f"overall_{value_col}.csv", index=False)

    xlab = "t (ms)" if time_ms else "t (s)"
    plot_with_ci(
        overall,
        x_label=xlab,
        y_label=y_label if y_label else value_col,
        title=f"{vec_name}: mean across runs (reps={keep_reps})",
        out_path=out_dir / (plot_file if plot_file else f"plot_{value_col}.png"),
    )

    return long_df, per_run, overall

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("csv", help="scavetool export CSV-R file")
    ap.add_argument("--out", default="out", help="output directory")

    ap.add_argument("--delay-name", default="e2eDelayPeriodicSec", help="delay vector name")
    ap.add_argument("--reward-name", default="rlReward", help="reward vector name")
    ap.add_argument("--ec-name", default="Ec", help="energy change vector name (vector name)")
    ap.add_argument("--rc-name", default="Rc", help="role change vector name (vector name)")

    ap.add_argument("--gch-name", default="globalNumClusterHeads", help="global CH count vector name")
    ap.add_argument("--gms-name", default="TotalClusterMsgSent", help="global total sent vector name")

    ap.add_argument("--reps", default="", help="comma-separated repetition numbers to keep (default: auto-detect first N)")
    ap.add_argument("--n-runs", type=int, default=5, help="how many distinct reps to keep if --reps not given")
    ap.add_argument("--exclude-hosts", default="3", help="comma-separated host indices to exclude (still applies to globals; harmless)")
    ap.add_argument("--drop-nonpositive-delay", action="store_true", help="treat delay<=0 as missing (NaN)")
    ap.add_argument("--time-ms", action="store_true", help="convert t from seconds to milliseconds")
    ap.add_argument("--delay-ms", action="store_true", help="convert delay from seconds to milliseconds for output/plot")
    ap.add_argument("--t-min", type=float, default=5000.0, help="drop samples with t < this (in same unit as --time-ms)")

    args = ap.parse_args()

    in_path = Path(args.csv)
    out_dir = Path(args.out)
    out_dir.mkdir(parents=True, exist_ok=True)

    df = pd.read_csv(in_path)

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

    # t-min in correct units
    t_min = args.t_min
    # (caller provides in same unit as --time-ms, already OK)

    # ---- Delay + derived availability ----
    df_delay = dfv[dfv["name"] == args.delay_name].copy()
    if df_delay.empty:
        raise SystemExit(f"No delay vectors found with name={args.delay_name}")

    long_delay = expand_vectors(df_delay, "delay_sec").replace([np.inf, -np.inf], np.nan).dropna(subset=["t"])
    if args.drop_nonpositive_delay:
        long_delay.loc[long_delay["delay_sec"] <= 0, "delay_sec"] = np.nan

    if args.time_ms:
        long_delay["t"] *= 1000.0

    if t_min is not None:
        long_delay = long_delay[long_delay["t"] >= t_min].copy()

    long_delay.to_csv(out_dir / "long_delay.csv", index=False)

    long_avail = long_delay[["rep", "host", "t"]].copy()
    long_avail["availability"] = np.isfinite(long_delay["delay_sec"].to_numpy()).astype(float)
    long_avail.to_csv(out_dir / "long_availability.csv", index=False)

    per_run_delay = per_run_mean(long_delay, "delay_sec", "mean_delay_sec")
    per_run_avail = per_run_mean(long_avail, "availability", "mean_availability")
    per_run_delay.to_csv(out_dir / "per_run_delay.csv", index=False)
    per_run_avail.to_csv(out_dir / "per_run_availability.csv", index=False)

    overall_delay = overall_mean_ci(per_run_delay, "mean_delay_sec", clamp_low_to_zero=True)
    overall_avail = overall_mean_ci(per_run_avail, "mean_availability", clamp_low_to_zero=True)

    if args.delay_ms:
        overall_delay["mean"] *= 1000.0
        overall_delay["ci95_low"] *= 1000.0
        overall_delay["ci95_high"] *= 1000.0

    overall_delay.to_csv(out_dir / "overall_delay.csv", index=False)
    overall_avail.to_csv(out_dir / "overall_availability.csv", index=False)

    xlab = "t (ms)" if args.time_ms else "t (s)"
    ylab_delay = "Delay (ms)" if args.delay_ms else "Delay (s)"

    plot_with_ci(
        overall_delay, x_label=xlab, y_label=ylab_delay,
        title=f"{args.delay_name}: mean across runs (reps={keep_reps})",
        out_path=out_dir / "plot_delay.png",
    )
    plot_with_ci(
        overall_avail, x_label=xlab, y_label="Availability (fraction)",
        title=f"availability (derived from {args.delay_name} validity): mean across runs (reps={keep_reps})",
        out_path=out_dir / "plot_availability.png",
    )

    # ---- Other metrics: rlReward, Ec, Rc, globalNumClusterHeads, TotalClusterMsgSent ----
    # rlReward
    process_metric(
        dfv=dfv, vec_name=args.reward_name,
        value_col="rlReward", per_run_col="mean_rlReward",
        out_dir=out_dir, keep_reps=keep_reps,
        time_ms=args.time_ms, t_min=t_min,
        clamp_low_to_zero=False,
        y_label="rlReward", plot_file="plot_rlreward.png",
    )

    # Ec
    process_metric(
        dfv=dfv, vec_name=args.ec_name,
        value_col="Ec", per_run_col="mean_Ec",
        out_dir=out_dir, keep_reps=keep_reps,
        time_ms=args.time_ms, t_min=0,
        clamp_low_to_zero=False,
        y_label="Ec", plot_file="plot_Ec.png",
    )

    # Rc
    process_metric(
        dfv=dfv, vec_name=args.rc_name,
        value_col="Rc", per_run_col="mean_Rc",
        out_dir=out_dir, keep_reps=keep_reps,
        time_ms=args.time_ms, t_min=t_min,
        clamp_low_to_zero=True,   # if Rc is a count, clamp low is fine; change if Rc can be negative
        y_label="Rc", plot_file="plot_Rc.png",
    )

    # globalNumClusterHeads (still appears per-host in CSV-R; averaging over hosts is fine but redundant)
    process_metric(
        dfv=dfv, vec_name=args.gch_name,
        value_col="globalNumCH", per_run_col="mean_globalNumCH",
        out_dir=out_dir, keep_reps=keep_reps,
        time_ms=args.time_ms, t_min=0,
        clamp_low_to_zero=True,
        y_label="Global #CH", plot_file="plot_globalNumCH.png",
    )

    # TotalClusterMsgSent (monotonic counter; mean across hosts makes no sense if every host records same global total.
    # But if only one host records it, mean still works. Better: take max over hosts per (rep,t).
    df_gms = dfv[dfv["name"] == args.gms_name].copy()
    if not df_gms.empty:
        long_gms = expand_vectors(df_gms, "TotalClusterMsgSent").replace([np.inf, -np.inf], np.nan).dropna(subset=["t"])
        if args.time_ms:
            long_gms["t"] *= 1000.0
        if t_min is not None:
            long_gms = long_gms[long_gms["t"] >= 0].copy()
        long_gms.to_csv(out_dir / "long_TotalClusterMsgSent.csv", index=False)

        per_run_gms = (
            long_gms.dropna(subset=["TotalClusterMsgSent"])
                    .groupby(["rep", "t"], as_index=False)
                    .agg(mean_TotalClusterMsgSent=("TotalClusterMsgSent", "max"),  # use max to avoid averaging duplicates
                         n_hosts_used=("TotalClusterMsgSent", "count"))
                    .sort_values(["rep", "t"])
        )
        per_run_gms.to_csv(out_dir / "per_run_TotalClusterMsgSent.csv", index=False)

        overall_gms = overall_mean_ci(per_run_gms, "mean_TotalClusterMsgSent", clamp_low_to_zero=True)
        overall_gms.to_csv(out_dir / "overall_TotalClusterMsgSent.csv", index=False)

        plot_with_ci(
            overall_gms,
            x_label=xlab,
            y_label="TotalClusterMsgSent",
            title=f"{args.gms_name}: mean across runs (reps={keep_reps})",
            out_path=out_dir / "plot_TotalClusterMsgSent.png",
        )

if __name__ == "__main__":
    main()
