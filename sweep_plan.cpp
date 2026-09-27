#include "sweep_plan.h"

#include <cmath>

std::vector<double> BuildSweepFrequencies(double startHz, double endHz, int points,
                                          bool logarithmic, double sampleRate) {
  std::vector<double> frequencies;
  if (!std::isfinite(startHz) || !std::isfinite(endHz) || !std::isfinite(sampleRate) ||
      startHz < 20.0 || endHz <= startHz || endHz > 40000.0 ||
      endHz >= sampleRate * 0.48 || points < 2 || points > 120) return frequencies;

  frequencies.reserve(points);
  const double ratio = endHz / startHz;
  for (int step = 0; step < points; ++step) {
    const double fraction = static_cast<double>(step) / (points - 1);
    frequencies.push_back(logarithmic ? startHz * std::pow(ratio, fraction)
                                      : startHz + (endHz - startHz) * fraction);
  }
  frequencies.front() = startHz;
  frequencies.back() = endHz;
  return frequencies;
}

namespace {
double InterpolateLog(double x0, double y0, double x1, double y1, double x) {
  const double fraction = std::log(x / x0) / std::log(x1 / x0);
  return y0 + (y1 - y0) * fraction;
}

double CrossingHz(double x0, double y0, double x1, double y1) {
  const double fraction = (-3.0 - y0) / (y1 - y0);
  return x0 * std::pow(x1 / x0, fraction);
}
}  // namespace

SweepAnalysis AnalyzeSweepChannel(const std::vector<double>& frequencies,
                                  const std::vector<double>& rms, double referenceHz) {
  SweepAnalysis result;
  if (frequencies.size() != rms.size() || frequencies.empty() ||
      !std::isfinite(referenceHz) || referenceHz <= 0.0) return result;
  for (size_t i = 0; i < frequencies.size(); ++i) {
    if (!std::isfinite(frequencies[i]) || frequencies[i] <= 0.0 ||
        (i && frequencies[i] <= frequencies[i - 1])) return SweepAnalysis();
    const double value = std::isfinite(rms[i]) && rms[i] > 0.0 ?
        20.0 * std::log10(rms[i]) : -150.0;
    result.levelsDb.push_back(value);
  }
  if (referenceHz < frequencies.front() || referenceHz > frequencies.back()) return result;

  size_t referenceIndex = 0;
  while (referenceIndex < frequencies.size() && frequencies[referenceIndex] < referenceHz)
    ++referenceIndex;
  if (referenceIndex < frequencies.size() && frequencies[referenceIndex] == referenceHz) {
    if (!std::isfinite(rms[referenceIndex]) || rms[referenceIndex] <= 0.0) return result;
    result.referenceDb = result.levelsDb[referenceIndex];
  } else {
    if (referenceIndex == 0 || referenceIndex == frequencies.size() ||
        !std::isfinite(rms[referenceIndex - 1]) || rms[referenceIndex - 1] <= 0.0 ||
        !std::isfinite(rms[referenceIndex]) || rms[referenceIndex] <= 0.0) return result;
    result.referenceDb = InterpolateLog(frequencies[referenceIndex - 1],
        result.levelsDb[referenceIndex - 1], frequencies[referenceIndex],
        result.levelsDb[referenceIndex], referenceHz);
  }
  result.hasReference = true;
  std::vector<double> x = frequencies;
  std::vector<double> y = result.levelsDb;
  if (referenceIndex == frequencies.size() || frequencies[referenceIndex] != referenceHz) {
    x.insert(x.begin() + referenceIndex, referenceHz);
    y.insert(y.begin() + referenceIndex, result.referenceDb);
  }
  for (size_t i = 0; i < y.size(); ++i) y[i] -= result.referenceDb;
  for (size_t i = referenceIndex; i > 0; --i) {
    if (y[i - 1] <= -3.0 && y[i] > -3.0) {
      result.lowCutoffHz = CrossingHz(x[i - 1], y[i - 1], x[i], y[i]);
      result.hasLowCutoff = true;
      break;
    }
  }
  for (size_t i = referenceIndex; i + 1 < y.size(); ++i) {
    if (y[i] > -3.0 && y[i + 1] <= -3.0) {
      result.highCutoffHz = CrossingHz(x[i], y[i], x[i + 1], y[i + 1]);
      result.hasHighCutoff = true;
      break;
    }
  }
  return result;
}

std::vector<double> CorrectSweepRms(const std::vector<double>& frequencies,
                                    const std::vector<double>& rms,
                                    const std::vector<double>& referenceFrequencies,
                                    const std::vector<double>& referenceRms) {
  std::vector<double> corrected;
  if (frequencies.empty() || frequencies.size() != rms.size() ||
      referenceFrequencies.size() < 2 ||
      referenceFrequencies.size() != referenceRms.size()) return corrected;
  for (size_t i = 0; i < referenceFrequencies.size(); ++i) {
    if (!std::isfinite(referenceFrequencies[i]) || referenceFrequencies[i] <= 0.0 ||
        !std::isfinite(referenceRms[i]) || referenceRms[i] <= 0.0 ||
        (i && referenceFrequencies[i] <= referenceFrequencies[i - 1])) return {};
  }
  size_t referenceIndex = 0;
  for (size_t i = 0; i < frequencies.size(); ++i) {
    const double hz = frequencies[i];
    if (!std::isfinite(hz) || hz <= 0.0 || !std::isfinite(rms[i]) || rms[i] <= 0.0 ||
        (i && hz <= frequencies[i - 1]) || hz < referenceFrequencies.front() ||
        hz > referenceFrequencies.back()) return {};
    while (referenceIndex + 1 < referenceFrequencies.size() &&
           referenceFrequencies[referenceIndex + 1] < hz) ++referenceIndex;
    double logReference = std::log(referenceRms[referenceIndex]);
    if (hz != referenceFrequencies[referenceIndex]) {
      if (referenceIndex + 1 >= referenceFrequencies.size()) return {};
      logReference = InterpolateLog(referenceFrequencies[referenceIndex], logReference,
          referenceFrequencies[referenceIndex + 1],
          std::log(referenceRms[referenceIndex + 1]), hz);
    }
    const double value = std::exp(std::log(rms[i]) - logReference);
    if (!std::isfinite(value) || value <= 0.0) return {};
    corrected.push_back(value);
  }
  return corrected;
}
