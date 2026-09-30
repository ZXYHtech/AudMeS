#include "spectrum_metrics.h"

#include <algorithm>
#include <cstddef>
#include <cmath>

namespace {

double BandPower(const std::vector<double>& power, int first, int last) {
  double sum = 0.0;
  for (int bin = first; bin <= last; ++bin) sum += power[bin];
  return sum;
}

int NotchRadius(int windowChoice) {
  // First-null half-width, plus one guard bin for an off-bin tone.
  switch (windowChoice) {
    case 1:
      return 3;  // Hann
    case 2:
      return 4;  // Blackman
    case 3:
      return 5;  // Blackman-Harris
    default:
      return 2;  // Rectangular
  }
}

double ARaw(double hz) {
  const double f2 = hz * hz;
  const double c1 = 20.6 * 20.6;
  const double c2 = 107.7 * 107.7;
  const double c3 = 737.9 * 737.9;
  const double c4 = 12194.0 * 12194.0;
  return c4 * f2 * f2 /
      ((f2 + c1) * std::sqrt(f2 + c2) * std::sqrt(f2 + c3) * (f2 + c4));
}

double APowerGain(double hz) {
  const double gain = ARaw(hz) / ARaw(1000.0);
  return gain * gain;
}

}  // namespace

SpectrumMetrics AnalyzeSpectrum(const std::vector<double>& powerBins, int fftSize,
                                double sampleRate, int windowChoice,
                                double lowHz, double highHz) {
  SpectrumMetrics result;
  if (fftSize < 128 || !std::isfinite(sampleRate) || sampleRate <= 0.0 ||
      !std::isfinite(lowHz) || !std::isfinite(highHz) || lowHz <= 0.0 ||
      highHz <= lowHz || lowHz >= sampleRate / 2.0 ||
      powerBins.size() != static_cast<std::size_t>(fftSize / 2)) return result;

  const double binHz = sampleRate / fftSize;
  const int first = std::max(1, static_cast<int>(std::ceil(lowHz / binHz)));
  const int last = std::min(static_cast<int>(powerBins.size()) - 1,
      static_cast<int>(std::floor(std::min(highHz, sampleRate / 2.0) / binHz)));
  if (first >= last) return result;
  result.bandLowHz = first * binHz;
  result.bandHighHz = last * binHz;

  int peak = first;
  for (int bin = first; bin <= last; ++bin) {
    if (!std::isfinite(powerBins[bin]) || powerBins[bin] < 0.0) return result;
    if (powerBins[bin] > powerBins[peak]) peak = bin;
  }

  const int radius = NotchRadius(windowChoice);
  const double fullScaleSinePower = 0.5;
  const double minimumTonePower = fullScaleSinePower * 1e-9;  // -90 dBFS
  // A tone too close to DC cannot be separated from its harmonics with this FFT.
  const bool tonePresent = powerBins[peak] > minimumTonePower && peak > 2 * radius;
  double fractionalPeak = peak;
  if (tonePresent && peak > first && peak < last && powerBins[peak - 1] > 0.0 &&
      powerBins[peak + 1] > 0.0) {
    const double left = std::log(powerBins[peak - 1]);
    const double center = std::log(powerBins[peak]);
    const double right = std::log(powerBins[peak + 1]);
    const double curvature = left - 2.0 * center + right;
    if (curvature < 0.0) {
      const double offset = 0.5 * (left - right) / curvature;
      if (std::isfinite(offset) && std::fabs(offset) <= 0.5) fractionalPeak += offset;
    }
  }
  const double peakPower =
      tonePresent
          ? BandPower(powerBins, std::max(first, peak - radius), std::min(last, peak + radius))
          : 0.0;

  std::vector<unsigned char> excluded(powerBins.size(), 0);
  if (tonePresent) {
    result.hasFundamental = true;
    result.fundamentalHz = fractionalPeak * binHz;
    result.fundamentalDbfs = 10.0 * std::log10(peakPower / fullScaleSinePower);
    for (int bin = std::max(first, peak - radius); bin <= std::min(last, peak + radius); ++bin)
      excluded[bin] = 1;
  }

  double harmonicPower = 0.0;
  for (int harmonic = 2; tonePresent && harmonic <= 10; ++harmonic) {
    const int center = static_cast<int>(std::lround(fractionalPeak * harmonic));
    if (center - radius > last) break;
    if (center - radius <= peak + radius) continue;
    const int lo = std::max(first, center - radius);
    const int hi = std::min(last, center + radius);
    if (lo > hi) continue;
    const double band = BandPower(powerBins, lo, hi);
    harmonicPower += band;
    if (band > 0.0) {
      result.hasHarmonic[harmonic - 2] = true;
      result.harmonicsDbc[harmonic - 2] = 10.0 * std::log10(band / peakPower);
    }
    for (int bin = lo; bin <= hi; ++bin) excluded[bin] = 1;
  }
  if (tonePresent) result.thdPercent = 100.0 * std::sqrt(harmonicPower / peakPower);

  double residualPower = 0.0;
  double noisePower = 0.0;
  double weightedNoisePower = 0.0;
  std::vector<double> noiseBins;
  noiseBins.reserve(last - first + 1);
  for (int bin = first; bin <= last; ++bin) {
    const double power = powerBins[bin];
    if (!tonePresent || bin < peak - radius || bin > peak + radius) residualPower += power;
    if (!excluded[bin]) {
      noisePower += power;
      weightedNoisePower += power * APowerGain(bin * binHz);
      noiseBins.push_back(power);
    }
  }

  if (tonePresent && residualPower > 0.0) {
    result.hasThdn = true;
    result.thdnPercent = 100.0 * std::sqrt(residualPower / peakPower);
    result.thdnDb = 10.0 * std::log10(residualPower / peakPower);
    result.sinadDb = 10.0 * std::log10((peakPower + residualPower) / residualPower);
  }
  if (tonePresent && noisePower > 0.0) {
    result.hasSnr = true;
    result.snrDb = 10.0 * std::log10(peakPower / noisePower);
  }
  if (noisePower > 0.0 && weightedNoisePower > 0.0) {
    result.hasIntegratedNoise = true;
    result.integratedNoiseDbfs = 10.0 * std::log10(noisePower / fullScaleSinePower);
    result.aWeightedNoiseDbfs =
        10.0 * std::log10(weightedNoisePower / fullScaleSinePower);
  }
  if (!noiseBins.empty()) {
    const std::size_t middle = noiseBins.size() / 2;
    std::nth_element(noiseBins.begin(), noiseBins.begin() + middle, noiseBins.end());
    const double medianPower = noiseBins[middle];
    if (medianPower > 0.0) {
      result.hasNoiseFloor = true;
      result.noiseFloorDbfsPerHz =
          10.0 * std::log10((medianPower / binHz) / fullScaleSinePower);
    }
  }
  return result;
}
