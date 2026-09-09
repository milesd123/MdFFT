#pragma once

#include <complex>
#include <cstddef>
#include <string>
#include <vector>

namespace minifft {

using Complex = std::complex<double>;
using Signal = std::vector<Complex>;

constexpr double kPi = 3.141592653589793238462643383279502884;

// Compute the discrete Fourier transform directly from its definition. Passing
// inverse=true reverses the rotation and applies the conventional 1/N scaling.
Signal dft(const Signal& input, bool inverse = false);

// Compute a radix-2 fast Fourier transform, or reconstruct a signal with its
// inverse. Both routines require a non-empty power-of-two input size.
Signal fft(const Signal& input);
Signal ifft(const Signal& input);

// Generate a real sinusoid whose frequency is expressed as an FFT-bin index.
// A whole-number bin completes exactly that many cycles in sample_count samples.
Signal generate_tone(std::size_t sample_count, double bin, double amplitude = 1.0,
                     double phase_radians = 0.0);

// Add several generated tones sample by sample. Each bin must have a matching
// amplitude entry, which keeps the signal description explicit at call sites.
Signal generate_tone_mix(std::size_t sample_count,
                         const std::vector<double>& bins,
                         const std::vector<double>& amplitudes);

// Return |z|^2 without taking a square root, which is sufficient for comparing
// spectral energy. strongest_bin applies that comparison over a half-open range.
double magnitude_squared(const Complex& value);
std::size_t strongest_bin(const Signal& spectrum, std::size_t first,
                          std::size_t last_exclusive);

// Export complex FFT results and derived magnitudes for plotting or inspection.
// The dB column is floored at a tiny value so log10 never receives zero.
void write_spectrum_csv(const std::string& path, const Signal& spectrum);

}  // namespace minifft
