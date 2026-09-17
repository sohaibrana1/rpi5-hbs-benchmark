import csv
import os
import sys

import matplotlib
matplotlib.use("pdf")

import matplotlib.pyplot as plt


if len(sys.argv) != 8:
    raise SystemExit(
        "usage: script H31 FigA FigB FigC FigD FigE figure_dir"
    )

h31_path = sys.argv[1]
fig_a = sys.argv[2]
fig_b = sys.argv[3]
fig_c = sys.argv[4]
fig_d = sys.argv[5]
fig_e = sys.argv[6]
fig_dir = sys.argv[7]

os.makedirs(fig_dir, exist_ok=True)

CONFIGS = [
    {
        "parameter": "XMSS-SHA2_10_256",
        "figure": fig_a,
        "markers": [
            4, 8, 16, 32, 64, 128, 256, 512
        ],
        "expected_n": 1023,
    },
    {
        "parameter": "XMSS-SHA2_16_256",
        "figure": fig_b,
        "markers": [
            4, 8, 16, 32, 64, 128,
            256, 512, 1024, 2048,
            4096, 8192, 16384, 32768
        ],
        "expected_n": 65535,
    },
    {
        "parameter": "XMSSMT-SHA2_20/2_256",
        "figure": fig_c,
        "markers": [
            1024, 2048, 3072
        ],
        "expected_n": 4096,
    },
    {
        "parameter": "XMSSMT-SHA2_20/4_256",
        "figure": fig_d,
        "markers": list(range(32, 1024, 32)),
        "expected_n": 1024,
    },
]

PARAMETER_ORDER = [
    "XMSS-SHA2_10_256",
    "XMSS-SHA2_16_256",
    "XMSSMT-SHA2_20/2_256",
    "XMSSMT-SHA2_20/4_256",
]

DISPLAY_LABELS = {
    "XMSS-SHA2_10_256":
        "XMSS-SHA2-10-256",
    "XMSS-SHA2_16_256":
        "XMSS-SHA2-16-256",
    "XMSSMT-SHA2_20/2_256":
        "XMSSMT-SHA2-20/2-256",
    "XMSSMT-SHA2_20/4_256":
        "XMSSMT-SHA2-20/4-256",
}

with open(h31_path, newline="", encoding="utf-8") as f:
    rows = list(csv.DictReader(f))

if len(rows) != 71678:
    raise SystemExit(
        f"FAIL: expected 71678 H31 rows, found {len(rows)}"
    )

data = {p: [] for p in PARAMETER_ORDER}

for row in rows:

    p = row["parameter_set"]

    if p not in data:
        raise SystemExit(f"FAIL: unexpected parameter set: {p}")

    data[p].append({
        "index": int(row["signature_index"]),
        "r1": int(row["r1_sign_ns"]),
        "r2": int(row["r2_sign_ns"]),
        "r3": int(row["r3_sign_ns"]),
        "median": int(row["replicate_median_sign_ns"]),
        "minimum": int(row["replicate_min_sign_ns"]),
        "maximum": int(row["replicate_max_sign_ns"]),
        "range": int(row["replicate_range_sign_ns"]),
    })

for config in CONFIGS:

    p = config["parameter"]
    d = data[p]
    expected_n = config["expected_n"]

    if len(d) != expected_n:
        raise SystemExit(
            f"FAIL: {p}: expected {expected_n} rows, "
            f"found {len(d)}"
        )

    indices = [x["index"] for x in d]

    if indices != list(range(expected_n)):
        raise SystemExit(
            f"FAIL: non-contiguous H31 index block for {p}"
        )

    x = indices
    y1 = [v["r1"] / 1_000_000.0 for v in d]
    y2 = [v["r2"] / 1_000_000.0 for v in d]
    y3 = [v["r3"] / 1_000_000.0 for v in d]
    ym = [v["median"] / 1_000_000.0 for v in d]

    observed_max = max(
        max(y1),
        max(y2),
        max(y3),
        max(ym),
    )

    fig, ax = plt.subplots(
        figsize=(7.0, 4.2),
        constrained_layout=True
    )

    ax.plot(
        x,
        y1,
        linewidth=0.45,
        alpha=0.55,
        label="PUB_R1",
    )

    ax.plot(
        x,
        y2,
        linewidth=0.45,
        alpha=0.55,
        label="PUB_R2",
    )

    ax.plot(
        x,
        y3,
        linewidth=0.45,
        alpha=0.55,
        label="PUB_R3",
    )

    ax.plot(
        x,
        ym,
        linewidth=1.15,
        label="Three-key median",
    )

    for marker in config["markers"]:
        ax.axvline(
            marker,
            linestyle="--",
            linewidth=0.45,
            alpha=0.35,
        )

    ax.set_xlabel(
        "Signature index",
        fontsize=10
    )

    ax.set_ylabel(
        "Signing latency (ms)",
        fontsize=10
    )

    ax.tick_params(
        axis="both",
        labelsize=8
    )

    ax.set_xlim(
        0,
        expected_n - 1
    )

    ax.set_ylim(
        0,
        observed_max * 1.03
    )

    ax.grid(
        axis="y",
        linewidth=0.4,
        alpha=0.25
    )

    ax.legend(
        fontsize=8,
        frameon=True,
        loc="best"
    )

    fig.savefig(
        config["figure"],
        format="pdf",
        bbox_inches="tight"
    )

    plt.close(fig)


fig, ax = plt.subplots(
    figsize=(7.0, 4.2),
    constrained_layout=True
)

global_max = 0.0
max_index = 0

for p in PARAMETER_ORDER:

    d = data[p]

    x = [v["index"] for v in d]
    y = [
        v["median"] / 1_000_000.0
        for v in d
    ]

    if not x:
        raise SystemExit(f"FAIL: empty Figure E data for {p}")

    max_index = max(max_index, x[-1])
    global_max = max(global_max, max(y))

    ax.plot(
        x,
        y,
        linewidth=0.9,
        label=DISPLAY_LABELS[p],
    )

ax.set_xlabel(
    "Signature index",
    fontsize=10
)

ax.set_ylabel(
    "Three-key median signing latency (ms)",
    fontsize=10
)

ax.tick_params(
    axis="both",
    labelsize=8
)

ax.set_xlim(
    0,
    max_index
)

ax.set_ylim(
    0,
    global_max * 1.03
)

ax.grid(
    axis="y",
    linewidth=0.4,
    alpha=0.25
)

ax.legend(
    fontsize=8,
    frameon=True,
    loc="best"
)

fig.savefig(
    fig_e,
    format="pdf",
    bbox_inches="tight"
)

plt.close(fig)

print("PASS: Figures A-E generated.")
print("PASS: H31 full index domains supplied to plotting routine.")
print("PASS: no explicit downsampling performed.")
print("PASS: no smoothing performed.")
print("PASS: no imputation performed.")
print("PASS: no observation filtering performed.")
