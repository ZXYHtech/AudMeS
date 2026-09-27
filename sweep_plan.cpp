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
