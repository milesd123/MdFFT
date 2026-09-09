# MiniFFT

MiniFFT is my learning-focused Fourier transform project with three layers:

1. a direct `O(N^2)` DFT in modern C++;
2. an iterative radix-2 `O(N log N)` FFT/IFFT in C++;
3. a fixed-point eight-point FFT in SystemVerilog.

An optional acoustic BFSK demonstration detects two tones in noise and a delayed
echo. It is a small connection to underwater acoustic communications, not a
claim to be a complete modem.

## Build and run C++

Requirements: CMake 3.16+ and a C++20 compiler.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/signal_generator .
./build/fsk_demo
ctest --test-dir build --output-on-failure
```

The signal generator verifies the FFT against the direct DFT, checks the IFFT,
creates a spectrum with tones at bins 17 and 43, benchmarks both algorithms,
and writes RTL test vectors. Its output goes to `results/` and
`testbench/vectors/`.

The BFSK demo uses FFT bin 5 for zero and bin 12 for one. Optional arguments
control Gaussian-noise standard deviation and echo gain:

```bash
./build/fsk_demo 0.65 0.4
```

## Simulate the RTL

With Icarus Verilog installed:

```bash
iverilog -g2012 -o build/fft8_tb rtl/butterfly.sv rtl/fft8.sv testbench/fft8_tb.sv
vvp build/fft8_tb
```

The testbench reads vectors produced by C++ and permits three integer units of
error for Q1.15 twiddle-factor quantization.

## Hardware design

The RTL accepts eight signed, real 16-bit samples. It divides them into even
and odd four-point transforms, then combines them using:

`X[k] = E[k] + W8^k O[k]` and `X[k+4] = E[k] - W8^k O[k]`.

Twiddle factors use Q1.15 fixed point. Outputs are 32 bits, with no stage
scaling or saturation. `start` captures a transform and `done` is asserted for
one cycle. This is a simple combinational datapath plus output registers; a
future version could reuse one butterfly over several cycles or pipeline it.

## To-do experiments...

- Change the generated tones and predict their bins before running.
- Plot `results/spectrum.csv` and identify conjugate-symmetric peaks.
- Compare the complexity curves in `results/benchmarks.csv`.
- Increase BFSK noise and echo strength and measure bit-error rate.
- Add per-stage RTL scaling and compare overflow against precision.
- Later, replace fixed size eight with a parameterized architecture.
