import csv
import os
import sys
from decimal import Decimal, ROUND_HALF_UP

if len(sys.argv) != 9:
    raise SystemExit(
        "usage: script H25 H27 H29 H33 T1 T2 T3 T4"
    )

h25_path, h27_path, h29_path, h33_path, t1, t2, t3, t4 = sys.argv[1:9]

MS_QUANT = Decimal("0.000001")
CV_QUANT = Decimal("0.001")
MILLION = Decimal("1000000")

DISPLAY = {
    "XMSS-SHA2_10_256": "XMSS-SHA2-10-256",
    "XMSS-SHA2_16_256": "XMSS-SHA2-16-256",
    "XMSSMT-SHA2_20/2_256": "XMSSMT-SHA2-20/2-256",
    "XMSSMT-SHA2_20/4_256": "XMSSMT-SHA2-20/4-256",
}

REP = {
    "PUB_R1": "R1",
    "PUB_R2": "R2",
    "PUB_R3": "R3",
}

METRIC = {
    "sign_ns": "Sign",
    "verify_ns": "Verify",
}


def read_csv(path):
    with open(path, newline="", encoding="utf-8") as f:
        return list(csv.DictReader(f))


def ms(text):
    value = Decimal(text) / MILLION
    return format(
        value.quantize(MS_QUANT, rounding=ROUND_HALF_UP),
        ".6f",
    )


def cv(text):
    value = Decimal(text)
    return format(
        value.quantize(CV_QUANT, rounding=ROUND_HALF_UP),
        ".3f",
    )


def require(mapping, key, label):
    if key not in mapping:
        raise SystemExit(f"FAIL: unmapped {label}: {key}")
    return mapping[key]


def write_new(path, lines):
    with open(path, "x", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(lines))
        f.write("\n")


h25 = read_csv(h25_path)
h27 = read_csv(h27_path)
h29 = read_csv(h29_path)
h33 = read_csv(h33_path)

if len(h25) != 24:
    raise SystemExit(f"FAIL: H25 rows={len(h25)}, expected 24")
if len(h27) != 8:
    raise SystemExit(f"FAIL: H27 rows={len(h27)}, expected 8")
if len(h29) != 102:
    raise SystemExit(f"FAIL: H29 rows={len(h29)}, expected 102")
if len(h33) != 66:
    raise SystemExit(f"FAIL: H33 rows={len(h33)}, expected 66")

# -------------------------------------------------------------------
# S-BDS1
# -------------------------------------------------------------------

lines = [
    r"\begin{table}[p]",
    r"\centering",
    r"\scriptsize",
    r"\setlength{\tabcolsep}{2.5pt}",
    r"\resizebox{\textwidth}{!}{%",
    r"\begin{tabular}{@{}lllrrrrrrrrrr@{}}",
    r"\toprule",
    (
        r"Configuration & Replicate & Metric & $n$ & Mean (ms) & "
        r"Median (ms) & SD (ms) & CV (\%) & Min (ms) & "
        r"P90 (ms) & P95 (ms) & P99 (ms) & Max (ms) \\"
    ),
    r"\midrule",
]

for i, row in enumerate(h25, start=1):
    values = [
        require(DISPLAY, row["parameter_set"], "parameter set"),
        require(REP, row["key_rep"], "replicate"),
        require(METRIC, row["metric"], "metric"),
        row["n"],
        ms(row["mean_ns"]),
        ms(row["median_ns"]),
        ms(row["sd_ns"]),
        cv(row["cv_pct"]),
        ms(row["min_ns"]),
        ms(row["p90_ns"]),
        ms(row["p95_ns"]),
        ms(row["p99_ns"]),
        ms(row["max_ns"]),
    ]

    lines.append(
        " & ".join(values)
        + rf" \\ % DATA_ROW {i:03d}"
    )

lines += [
    r"\bottomrule",
    r"\end{tabular}}",
    (
        r"\caption{Operation-level descriptive statistics for the "
        r"sequential-index extension. Each row summarizes one independently "
        r"generated measurement-key run. Latencies are reported in "
        r"milliseconds; CV denotes sample coefficient of variation.}"
    ),
    r"\label{tab:supp-bds-per-run}",
    r"\end{table}",
]

write_new(t1, lines)

# -------------------------------------------------------------------
# S-BDS2
# -------------------------------------------------------------------

lines = [
    r"\begin{table}[p]",
    r"\centering",
    r"\scriptsize",
    r"\setlength{\tabcolsep}{2.5pt}",
    r"\resizebox{\textwidth}{!}{%",
    r"\begin{tabular}{@{}llrrrrrrr@{}}",
    r"\toprule",
    (
        r"Configuration & Metric & Keys & R1 median (ms) & "
        r"R2 median (ms) & R3 median (ms) & Mean median (ms) & "
        r"Median range (ms) & CV of means (\%) \\"
    ),
    r"\midrule",
]

for i, row in enumerate(h27, start=1):
    values = [
        require(DISPLAY, row["parameter_set"], "parameter set"),
        require(METRIC, row["metric"], "metric"),
        row["replicate_count"],
        ms(row["r1_median_ns"]),
        ms(row["r2_median_ns"]),
        ms(row["r3_median_ns"]),
        ms(row["mean_replicate_medians_ns"]),
        ms(row["range_replicate_medians_ns"]),
        cv(row["cv_replicate_means_pct"]),
    ]

    lines.append(
        " & ".join(values)
        + rf" \\ % DATA_ROW {i:03d}"
    )

lines += [
    r"\bottomrule",
    r"\end{tabular}}",
    (
        r"\caption{Across-key replicate consistency for the sequential-index "
        r"extension. Each row uses three independently generated measurement "
        r"keys. Mean median is the mean of the three run medians; median "
        r"range is the range of those medians; CV of means is the sample "
        r"coefficient of variation of the three run means.}"
    ),
    r"\label{tab:supp-bds-replicate}",
    r"\end{table}",
]

write_new(t2, lines)

# -------------------------------------------------------------------
# helper for longtable
# -------------------------------------------------------------------

def longtable_header(caption, label, heading):
    return [
        r"\scriptsize",
        r"\setlength{\LTleft}{0pt}",
        r"\setlength{\LTright}{0pt}",
        r"\setlength{\tabcolsep}{2.4pt}",
        r"\begin{longtable}{@{}llrrrrrrr@{}}",
        rf"\caption{{{caption}}}",
        rf"\label{{{label}}}\\",
        r"\toprule",
        heading,
        r"\midrule",
        r"\endfirsthead",
        r"\toprule",
        heading,
        r"\midrule",
        r"\endhead",
        r"\midrule",
        r"\multicolumn{9}{r}{Continued on next page}\\",
        r"\endfoot",
        r"\bottomrule",
        r"\endlastfoot",
    ]


def longtable_tail():
    return [
        r"\end{longtable}",
        r"\normalsize",
    ]


heading = (
    r"Configuration & Replicate & Boundary & Mean (ms) & "
    r"Median (ms) & CV (\%) & Boundary (ms) & Max (ms) & Max rel. \\"
)

caption = (
    "Nine-position signing-latency windows around predefined XMSSMT planned "
    "lower-subtree-period boundaries. Boundaries are descriptive reference "
    "positions and do not establish unique or causal internal BDS transitions."
)

lines = longtable_header(
    caption,
    "tab:supp-bds-xmssmt-windows",
    heading,
)

for i, row in enumerate(h29, start=1):
    values = [
        require(DISPLAY, row["parameter_set"], "parameter set"),
        require(REP, row["key_rep"], "replicate"),
        row["boundary_index"],
        ms(row["mean_sign_ns"]),
        ms(row["median_sign_ns"]),
        cv(row["cv_sign_pct"]),
        ms(row["boundary_sign_ns"]),
        ms(row["max_sign_ns"]),
        row["max_relative_index"],
    ]

    lines.append(
        " & ".join(values)
        + rf" \\ % DATA_ROW {i:03d}"
    )

lines += longtable_tail()
write_new(t3, lines)

# -------------------------------------------------------------------
# S-BDS4
# -------------------------------------------------------------------

heading = (
    r"Configuration & Replicate & Reference & Mean (ms) & "
    r"Median (ms) & CV (\%) & Reference (ms) & Max (ms) & Max rel. \\"
)

caption = (
    "Nine-position signing-latency windows around predefined XMSS "
    "powers-of-two reference indices. Reference indices are descriptive "
    "inspection positions and do not identify proven internal BDS transition "
    "events."
)

lines = longtable_header(
    caption,
    "tab:supp-bds-xmss-windows",
    heading,
)

for i, row in enumerate(h33, start=1):
    values = [
        require(DISPLAY, row["parameter_set"], "parameter set"),
        require(REP, row["key_rep"], "replicate"),
        row["reference_index"],
        ms(row["mean_sign_ns"]),
        ms(row["median_sign_ns"]),
        cv(row["cv_sign_pct"]),
        ms(row["reference_sign_ns"]),
        ms(row["max_sign_ns"]),
        row["max_relative_index"],
    ]

    lines.append(
        " & ".join(values)
        + rf" \\ % DATA_ROW {i:03d}"
    )

lines += longtable_tail()
write_new(t4, lines)

print("PASS: generated S-BDS1 from all 24 H25 rows.")
print("PASS: generated S-BDS2 from all 8 H27 rows.")
print("PASS: generated S-BDS3 from all 102 H29 rows.")
print("PASS: generated S-BDS4 from all 66 H33 rows.")
print("PASS: Decimal/ROUND_HALF_UP used for all displayed conversions.")
