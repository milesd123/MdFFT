#include "complex.hpp"

#include <cstddef>
#include <iostream>
#include <random>
#include <string>

namespace {

// Encode one bit as either FFT bin 5 or bin 12, then pass it through a minimal
// acoustic channel consisting of Gaussian noise and one delayed echo.
minifft::Signal make_symbol(bool bit, std::size_t count, double noise_sigma,
                            double echo_gain, std::mt19937& generator) {
    const double bin = bit ? 12.0 : 5.0;
    const auto clean = minifft::generate_tone(count, bin);
    auto received = clean;
    std::normal_distribution<double> noise(0.0, noise_sigma);
    constexpr std::size_t kEchoDelay = 3;
    for (std::size_t n = 0; n < count; ++n) {
        // Samples before the delay have no echo because the reflected path has
        // not arrived yet.
        const double echo = n >= kEchoDelay ? echo_gain * clean[n - kEchoDelay].real() : 0.0;
        received[n] = clean[n].real() + echo + noise(generator);
    }
    return received;
}

// Recover a BFSK bit by comparing energy in the two assigned frequency bins.
// Squared magnitudes avoid an unnecessary square root in the decision rule.
bool decode_symbol(const minifft::Signal& received) {
    const auto spectrum = minifft::fft(received);
    return minifft::magnitude_squared(spectrum[12]) >
           minifft::magnitude_squared(spectrum[5]);
}

}  // namespace

// Transmit a fixed bit string through the simulated channel and report its BER.
// Optional arguments set noise standard deviation and echo gain, respectively.
int main(int argc, char** argv) {
    const double noise_sigma = argc > 1 ? std::stod(argv[1]) : 0.45;
    const double echo_gain = argc > 2 ? std::stod(argv[2]) : 0.30;
    constexpr std::size_t kSamplesPerSymbol = 64;
    const std::string message = "0100110101101001011011000110010101110011";  // ASCII "Miles"
    std::string recovered;
    // A fixed seed makes failures reproducible while still exercising random noise.
    std::mt19937 generator(0xA0CE4FF7U);
    std::size_t errors = 0;

    for (const char character : message) {
        const bool sent = character == '1';
        const bool received = decode_symbol(
            make_symbol(sent, kSamplesPerSymbol, noise_sigma, echo_gain, generator));
        recovered += received ? '1' : '0';
        errors += sent != received;
    }

    std::cout << "MiniFFT acoustic BFSK demo\n"
              << "noise sigma: " << noise_sigma << ", echo gain: " << echo_gain << '\n'
              << "transmitted: " << message << '\n'
              << "recovered:   " << recovered << '\n'
              << "bit errors:  " << errors << '/' << message.size() << " (BER "
              << static_cast<double>(errors) / static_cast<double>(message.size()) << ")\n";
    return errors == 0 ? 0 : 2;
}
