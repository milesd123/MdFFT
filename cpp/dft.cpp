#include "complex.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <stdexcept>

namespace minifft {

// Evaluate every output frequency against every input sample. This mirrors the
// DFT equation closely and serves as a simple O(N^2) correctness reference.
Signal dft(const Signal& input, bool inverse) {
    const std::size_t n = input.size();
    Signal output(n);
    if (n == 0) return output;

    // Forward and inverse transforms rotate in opposite directions.
    const double direction = inverse ? 1.0 : -1.0;
    for (std::size_t k = 0; k < n; ++k) {
        Complex sum{0.0, 0.0};
        for (std::size_t sample = 0; sample < n; ++sample) {
            const double angle = direction * 2.0 * kPi *
                                 static_cast<double>(k * sample) /
                                 static_cast<double>(n);
            sum += input[sample] * Complex{std::cos(angle), std::sin(angle)};
        }
        output[k] = inverse ? sum / static_cast<double>(n) : sum;
    }
    return output;
}

// Produce a sampled real sinusoid at the requested FFT-bin frequency. Using a
// fractional bin is allowed and is useful for demonstrating spectral leakage.
Signal generate_tone(std::size_t sample_count, double bin, double amplitude,
                     double phase_radians) {
    Signal samples(sample_count);
    for (std::size_t n = 0; n < sample_count; ++n) {
        const double angle = 2.0 * kPi * bin * static_cast<double>(n) /
                                 static_cast<double>(sample_count) + phase_radians;
        samples[n] = amplitude * std::sin(angle);
    }
    return samples;
}

// Construct a multi-tone signal by summing independently generated sinusoids.
// The result starts at zero because std::complex is value-initialized.
Signal generate_tone_mix(std::size_t sample_count,
                         const std::vector<double>& bins,
                         const std::vector<double>& amplitudes) {
    if (bins.size() != amplitudes.size()) {
        throw std::invalid_argument("bins and amplitudes must have equal lengths");
    }
    Signal samples(sample_count);
    for (std::size_t tone = 0; tone < bins.size(); ++tone) {
        const Signal component = generate_tone(sample_count, bins[tone], amplitudes[tone]);
        for (std::size_t n = 0; n < sample_count; ++n) samples[n] += component[n];
    }
    return samples;
}

// Compute spectral energy as real^2 + imag^2. Avoiding sqrt makes comparisons
// cheaper without changing which bin has the greatest magnitude.
double magnitude_squared(const Complex& value) {
    return std::norm(value);
}

// Find the most energetic bin inside [first, last_exclusive). Restricting the
// range lets callers ignore DC, negative-frequency mirrors, or unused bins.
std::size_t strongest_bin(const Signal& spectrum, std::size_t first,
                          std::size_t last_exclusive) {
    if (first >= last_exclusive || last_exclusive > spectrum.size()) {
        throw std::invalid_argument("invalid FFT-bin range");
    }
    std::size_t best = first;
    for (std::size_t bin = first + 1; bin < last_exclusive; ++bin) {
        if (magnitude_squared(spectrum[bin]) > magnitude_squared(spectrum[best])) best = bin;
    }
    return best;
}

// Write one row per FFT bin for external plotting and debugging. Magnitude is
// included in both linear and decibel form so no post-processing is required.
void write_spectrum_csv(const std::string& path, const Signal& spectrum) {
    std::ofstream output(path);
    if (!output) throw std::runtime_error("could not open " + path);
    output << "bin,real,imag,magnitude,magnitude_db\n" << std::setprecision(12);
    for (std::size_t bin = 0; bin < spectrum.size(); ++bin) {
        const double magnitude = std::abs(spectrum[bin]);
        // Clamp exact zeros to avoid a negative-infinity CSV value.
        const double db = 20.0 * std::log10(std::max(magnitude, 1e-12));
        output << bin << ',' << spectrum[bin].real() << ',' << spectrum[bin].imag()
               << ',' << magnitude << ',' << db << '\n';
    }
}

}  // namespace minifft
