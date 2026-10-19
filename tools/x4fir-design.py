#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2026 Deutsches Forschungszentrum für Künstliche Intelligenz GmbH (DFKI)
"""Design the VSPA Tx x4 interpolation FIR (7.68 -> 30.72 MS/s, 25 PRB n5) and write it in the DMEM
layout X4_interp_tap64_filter expects (see the header of vspa-lib/src/X4_interp_tap64_filter.sx):
  H[0][0],H[0][0],H[1][0],H[1][0],H[2][0],H[2][0],H[3][0],H[3][0], H[0][1],... H[3][15],H[3][15]
with output[i][j] = sum_k H[j][k]*input[i+k], i.e. prototype g[4*(15-k)+j] = H[j][k].
Prototype: 64-tap Kaiser-windowed sinc, cutoff fs_in/2 = 3.84 MHz, each polyphase branch scaled to DC gain 1.

Usage: x4fir-design.py [--check FILE] [--beta B] [OUT]   (default OUT: stdout)
--check FILE: print the response of an existing fir_interpolation_x4.txt under the same convention."""
import cmath, math, sys

FS, N, L = 30.72e6, 64, 4

def bessel_i0(x):
    s, t, k = 1.0, 1.0, 1
    while t > 1e-12 * s:
        t *= (x / (2 * k)) ** 2; s += t; k += 1
    return s

def design(beta):
    fc = 3.84e6 / FS                      # normalised cutoff (cycles/sample)
    m = (N - 1) / 2
    g = []
    for n in range(N):
        x = n - m
        s = 2 * fc * (math.sin(2 * math.pi * fc * x) / (2 * math.pi * fc * x) if x else 1.0)
        w = bessel_i0(beta * math.sqrt(1 - (2 * x / (N - 1)) ** 2)) / bessel_i0(beta)
        g.append(s * w)
    H = [[g[4 * (15 - k) + j] for k in range(16)] for j in range(L)]
    H = [[v / sum(row) for v in row] for row in H]   # DC gain 1 per branch
    return H

def proto(H):
    g = [0.0] * N
    for j in range(L):
        for k in range(16):
            g[4 * (15 - k) + j] = H[j][k]
    return g

def report(H, name):
    g = proto(H)
    r = lambda f: abs(sum(v * cmath.exp(-2j * math.pi * f / FS * n) for n, v in enumerate(g)))
    h0 = r(0)
    pb = " ".join(f"{f}:{20 * math.log10(r(f * 1e6) / h0):+.2f}" for f in (0.5, 1, 1.5, 2, 2.25))
    sb = max(20 * math.log10(r(x * 1e4) / h0) for x in range(543, 1536))
    peak = max(sum(abs(v) for v in row) for row in H)
    print(f"{name}: passband {pb} dB | stopband 5.43-15.36 MHz max {sb:+.1f} dB | max sum|H[j]| {peak:.3f}",
          file=sys.stderr)

def load(path):
    t = [float(x) for x in open(path).read().replace("\n", "").split(",") if x.strip()][0::2]
    return [[t[4 * k + j] for k in range(16)] for j in range(L)]

if __name__ == "__main__":
    a = sys.argv[1:]
    if "--check" in a:
        report(load(a[a.index("--check") + 1]), "check"); sys.exit()
    beta = float(a[a.index("--beta") + 1]) if "--beta" in a else 7.0
    a = [x for i, x in enumerate(a) if x != "--beta" and (i == 0 or a[i - 1] != "--beta")]
    H = design(beta)
    report(H, f"Kaiser beta {beta}")
    out = open(a[0], "w") if a else sys.stdout
    lines = [f"{H[j][k]:.6f}," for k in range(16) for j in range(L) for _ in range(2)]
    lines[-1] = lines[-1].rstrip(",")
    out.write("\n".join(lines) + "\n")
