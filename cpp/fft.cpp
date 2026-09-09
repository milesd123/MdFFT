#include "complex.hpp"

#include <cmath>
#include <stdexcept>
#include <utility>

namespace minifft {
namespace {

// A positive power of two has exactly one set bit. Clearing that bit with
// value & (value - 1) therefore produces zero only for valid radix-2 sizes.
bool is_power_of_two(std::size_t value) {
    return value != 0 && (value & (value - 1)) == 0;
}

// Transform values in place using iterative radix-2 decimation in time. The
// algorithm first reorders inputs, then applies log2(N) butterfly stages.
void fft_in_place(Signal& values) {
    const std::size_t n = values.size();

    // Increment j in bit-reversed order and swap each pair only once. This
    // makes butterfly partners adjacent in the order required by later stages.
    for (std::size_t i = 1, j = 0; i < n; ++i) {
        std::size_t bit = n >> 1;
        while ((j & bit) != 0) {
            j ^= bit;
            bit >>= 1;
        }
        j ^= bit;
        if (i < j) std::swap(values[i], values[j]);
    }

    // Each stage doubles the transform length: 2, 4, 8, ... N.
    for (std::size_t length = 2; length <= n; length <<= 1) {
        const double angle = -2.0 * kPi / static_cast<double>(length);
        const Complex stage_root{std::cos(angle), std::sin(angle)};
        for (std::size_t block = 0; block < n; block += length) {
            Complex twiddle{1.0, 0.0};
            const std::size_t half = length / 2;
            for (std::size_t offset = 0; offset < half; ++offset) {
                const Complex even = values[block + offset];
                const Complex odd = values[block + offset + half] * twiddle;

                // One butterfly combines an even term with a rotated odd term.
                values[block + offset] = even + odd;
                values[block + offset + half] = even - odd;

                // Multiplication advances to the next root of unity in this stage.
                twiddle *= stage_root;
            }
        }
    }
}

}  // namespace

// Validate the radix-2 requirement and preserve the caller's input by working
// on a copy. The returned bins use the standard forward-transform sign.
Signal fft(const Signal& input) {
    if (!is_power_of_two(input.size())) {
        throw std::invalid_argument("FFT size must be a non-zero power of two");
    }
    Signal output = input;
    fft_in_place(output);
    return output;
}

// Apply the conjugate identity IFFT(X) = conj(FFT(conj(X))) / N. Reusing the
// forward implementation keeps the two transforms consistent and compact.
Signal ifft(const Signal& input) {
    if (!is_power_of_two(input.size())) {
        throw std::invalid_argument("IFFT size must be a non-zero power of two");
    }
    Signal output = input;
    for (Complex& value : output) value = std::conj(value);
    fft_in_place(output);
    for (Complex& value : output) {
        value = std::conj(value) / static_cast<double>(output.size());
    }
    return output;
}

}  // namespace minifft
