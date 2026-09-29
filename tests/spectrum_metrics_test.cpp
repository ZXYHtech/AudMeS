#include "../spectrum_metrics.h"
#include "../fourier.h"

#include <cmath>
#include <iostream>
#include <vector>

namespace {

bool Near(double actual, double expected, double tolerance) {
  return std::fabs(actual - expected) <= tolerance;
}

bool Check(bool condition, const char* message) {
  if (!condition) std::cerr << message << '\n';
  return condition;
}

}  // namespace

int main() {
  // 1 Hz bins, a full-scale 1 kHz sine, -40 dBc H2 and white background.
  const int size = 8192;
  std::vector<double> power(size / 2, 1e-10);
  power[1000] += 0.5;
  power[2000] += 0.00005;
  SpectrumMetrics result = AnalyzeSpectrum(power, size, 8192.0, 0);
  bool ok = true;
  std::vector<double> bandwidthPower(32768 / 2, 1e-12);
  bandwidthPower[1000] = 0.5;
  bandwidthPower[12000] = 0.00005;
  const SpectrumMetrics narrow = AnalyzeSpectrum(bandwidthPower, 32768, 32768.0, 0, 20.0, 10000.0);
  const SpectrumMetrics wide = AnalyzeSpectrum(bandwidthPower, 32768, 32768.0, 0, 20.0, 20000.0);
  ok &= Check(narrow.hasThdn && wide.hasThdn && wide.thdnPercent > narrow.thdnPercent * 10.0 &&
      narrow.bandHighHz == 10000.0 && wide.bandHighHz == 16383.0 &&
      !AnalyzeSpectrum(bandwidthPower, 32768, 32768.0, 0, 20000.0, 10000.0).hasFundamental &&
      !AnalyzeSpectrum(bandwidthPower, 32768, NAN, 0).hasFundamental,
      "Integration bandwidth selection or Nyquist limit failed");
  std::vector<double> allHarmonics(size / 2, 0.0);
  allHarmonics[100] = 0.5;
  for (int harmonic = 2; harmonic <= 10; ++harmonic)
    allHarmonics[100 * harmonic] = 0.5e-6;
  const SpectrumMetrics extended = AnalyzeSpectrum(allHarmonics, size, 8192.0, 0);
  for (int harmonic = 2; harmonic <= 10; ++harmonic)
    ok &= Check(extended.hasHarmonic[harmonic - 2] &&
        Near(extended.harmonicsDbc[harmonic - 2], -60.0, 1e-9),
        "H2-H10 independent readout failed");
  ok &= Check(extended.hasThdn && Near(extended.thdPercent, 0.3, 1e-9) &&
      Near(extended.thdnDb, 20.0 * std::log10(extended.thdnPercent / 100.0), 1e-9),
      "THD+N dB and percent consistency failed");
  ok &= Check(result.hasFundamental && Near(result.fundamentalHz, 1000.0, 0.01),
              "Fundamental detection failed");
  ok &= Check(Near(result.fundamentalDbfs, 0.0, 0.01), "dBFS normalization failed");
  ok &= Check(result.hasHarmonic[0] && Near(result.harmonicsDbc[0], -40.0, 0.01),
              "Second harmonic level failed");
  ok &= Check(Near(result.thdPercent, 1.0, 0.1), "THD calculation failed");
  ok &= Check(result.hasThdn && result.thdnPercent >= result.thdPercent &&
                  Near(result.sinadDb, 40.0, 0.2),
              "THD+N or SINAD calculation failed");
  ok &= Check(result.hasSnr && result.snrDb > 60.0,
              "SNR should exclude harmonic energy");
  ok &= Check(result.hasNoiseFloor &&
                  Near(result.noiseFloorDbfsPerHz, 10.0 * std::log10(2e-10), 0.01),
              "Noise density calculation failed");

  // A tone between FFT bins should shift the reported frequency to the right.
  std::vector<double> offBin(size / 2, 1e-12);
  offBin[999] = 0.05;
  offBin[1000] = 0.30;
  offBin[1001] = 0.15;
  offBin[2001] = 0.00005;
  SpectrumMetrics shifted = AnalyzeSpectrum(offBin, size, 8192.0, 1);
  ok &= Check(shifted.hasFundamental && shifted.fundamentalHz > 1000.0 &&
                  shifted.fundamentalHz < 1000.5,
              "Off-bin frequency interpolation failed");
  ok &= Check(shifted.hasHarmonic[0] && shifted.harmonicsDbc[0] < -35.0,
              "Off-bin harmonic band lookup failed");

  // With no detectable tone, ratios are unavailable but the floor is measurable.
  std::vector<double> quiet(size / 2, 1e-12);
  SpectrumMetrics empty = AnalyzeSpectrum(quiet, size, 8192.0, 3);
  ok &= Check(!empty.hasFundamental && !empty.hasThdn && !empty.hasSnr &&
                  empty.hasNoiseFloor,
              "Noise-only handling failed");

  // Harmonics above Nyquist must not be counted or displayed.
  std::vector<double> highTone(size / 2, 1e-12);
  highTone[3000] = 0.5;
  SpectrumMetrics high = AnalyzeSpectrum(highTone, size, 8192.0, 0);
  ok &= Check(high.hasFundamental && !high.hasHarmonic[0] &&
                  Near(high.thdPercent, 0.0, 0.01),
              "Out-of-band harmonics were included");

  // Exercise the same FFT and Hann power normalization used by the UI.
  const double pi = 3.14159265358979323846;
  std::vector<double> samples(size), real(size), imag(size);
  double windowPower = 0.0;
  for (int i = 0; i < size; ++i) {
    const double window = (1.0 - std::cos(2.0 * pi * i / size)) / size;
    samples[i] = 0.5 * std::sin(2.0 * pi * 1000.0 * i / size) * window;
    windowPower += window * window;
  }
  ok &= Check(fft_double(size, 0, samples.data(), nullptr, real.data(), imag.data()) == 1,
              "Synthetic FFT failed");
  std::vector<double> fftPower(size / 2, 0.0);
  for (int bin = 1; bin < size / 2; ++bin) {
    fftPower[bin] = 2.0 * (real[bin] * real[bin] + imag[bin] * imag[bin]) /
                    (size * windowPower);
  }
  SpectrumMetrics measured = AnalyzeSpectrum(fftPower, size, 8192.0, 1);
  ok &= Check(measured.hasFundamental && Near(measured.fundamentalHz, 1000.0, 0.01) &&
                  Near(measured.fundamentalDbfs, -6.0206, 0.05),
              "FFT to dBFS calibration failed");

  return ok ? 0 : 1;
}
