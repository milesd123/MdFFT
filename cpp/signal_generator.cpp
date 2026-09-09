#include "complex.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>

namespace fs = std::filesystem;
using Clock = std::chrono::steady_clock;

namespace {

// Return the largest absolute complex difference between two equal-length
// signals. The demo uses this to validate FFT and reconstruction accuracy.
double max_error(const minifft::Signal& left, const minifft::Signal& right) {
    double error = 0.0;
    for (std::size_t i = 0; i < left.size(); ++i) {
        error = std::max(error, std::abs(left[i] - right[i]));
    }
    return error;
}

// Time a callable repeatedly and return the median duration in microseconds.
// The median reduces the effect of scheduler interruptions and other outliers.
template <typename Function>
double median_runtime_us(Function&& function, int repetitions) {
    std::vector<double> runtimes;
    for (int iteration = 0; iteration < repetitions; ++iteration) {
        const auto start = Clock::now();
        const auto result = function();
        const auto finish = Clock::now();
        // Make an observable read so an optimizing compiler cannot discard the call.
        volatile double guard = result.front().real();
        (void)guard;
        runtimes.push_back(std::chrono::duration<double, std::micro>(finish - start).count());
    }
    std::sort(runtimes.begin(), runtimes.end());
    return runtimes[runtimes.size() / 2];
}

// Generate a quantized eight-sample tone and its direct-DFT reference outputs.
// Hex memory files let the SystemVerilog testbench consume identical data.
void write_rtl_vectors(const fs::path& directory) {
    fs::create_directories(directory);
    const auto input = minifft::generate_tone(8, 1.0, 1000.0);
    // Quantize before calculating the reference so C++ and RTL see identical samples.
    minifft::Signal quantized(8);
    for (std::size_t i = 0; i < 8; ++i) quantized[i] = std::lround(input[i].real());
    const auto expected = minifft::dft(quantized);

    std::ofstream samples(directory / "input.mem");
    std::ofstream real(directory / "expected_real.mem");
    std::ofstream imag(directory / "expected_imag.mem");
    if (!samples || !real || !imag) throw std::runtime_error("could not write RTL vectors");

    samples << std::hex << std::setfill('0');
    real << std::hex << std::setfill('0');
    imag << std::hex << std::setfill('0');
    for (std::size_t i = 0; i < 8; ++i) {
        const auto sample = static_cast<std::int16_t>(std::lround(quantized[i].real()));
        const auto re = static_cast<std::int32_t>(std::lround(expected[i].real()));
        const auto im = static_cast<std::int32_t>(std::lround(expected[i].imag()));
        // Unsigned casts preserve the signed values' two's-complement bit patterns.
        samples << std::setw(4) << static_cast<std::uint16_t>(sample) << '\n';
        real << std::setw(8) << static_cast<std::uint32_t>(re) << '\n';
        imag << std::setw(8) << static_cast<std::uint32_t>(im) << '\n';
    }
}

}  // namespace

// Run the software validation and benchmark workflow. An optional first
// argument selects the project root where results and RTL vectors are written.
int main(int argc, char** argv) {
    const fs::path root = argc > 1 ? fs::path(argv[1]) : fs::path(".");
    fs::create_directories(root / "results");

    // Real tones also appear at mirrored negative-frequency bins 239 and 213.
    const auto signal = minifft::generate_tone_mix(256, {17.0, 43.0}, {1.0, 0.45});
    const auto direct = minifft::dft(signal);
    const auto fast = minifft::fft(signal);
    const double error = max_error(direct, fast);
    if (error > 1e-8) {
        std::cerr << "DFT/FFT validation failed: " << error << '\n';
        return 1;
    }
    const double reconstruction_error = max_error(signal, minifft::ifft(fast));
    if (reconstruction_error > 1e-10) {
        std::cerr << "IFFT validation failed: " << reconstruction_error << '\n';
        return 1;
    }

    minifft::write_spectrum_csv((root / "results/spectrum.csv").string(), fast);
    write_rtl_vectors(root / "testbench/vectors");

    std::ofstream benchmarks(root / "results/benchmarks.csv");
    if (!benchmarks) throw std::runtime_error("could not write benchmarks.csv");
    benchmarks << "size,dft_us,fft_us,speedup\n" << std::fixed << std::setprecision(3);
    for (const std::size_t size : {8U, 16U, 32U, 64U, 128U, 256U, 512U, 1024U}) {
        const auto input = minifft::generate_tone_mix(size, {1.0, 3.0}, {1.0, 0.25});
        const int repetitions = size <= 128 ? 7 : 3;
        const double dft_us = median_runtime_us([&] { return minifft::dft(input); }, repetitions);
        const double fft_us = median_runtime_us([&] { return minifft::fft(input); }, repetitions);
        benchmarks << size << ',' << dft_us << ',' << fft_us << ',' << dft_us / fft_us << '\n';
    }

    std::cout << "DFT/FFT max error: " << error << '\n'
              << "FFT/IFFT reconstruction error: " << reconstruction_error << '\n'
              << "Expected spectrum peaks: bins 17, 43, 213, and 239\n"
              << "Generated results and RTL reference vectors.\n";
}
