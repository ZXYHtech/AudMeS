#ifndef AUDMES_SWEEP_PLAN_H
#define AUDMES_SWEEP_PLAN_H

#include <vector>

// Returns exactly `points` frequencies, including both endpoints.
// An empty result means the requested range cannot be measured safely.
std::vector<double> BuildSweepFrequencies(double startHz, double endHz, int points,
                                          bool logarithmic, double sampleRate);

struct SweepAnalysis {
  std::vector<double> levelsDb;
  bool hasReference = false;
  double referenceDb = 0.0;
  bool hasLowCutoff = false;
  double lowCutoffHz = 0.0;
  bool hasHighCutoff = false;
  double highCutoffHz = 0.0;
};

// Cutoffs are the first -3 dB crossings moving away from the 1 kHz reference.
// Interpolation is logarithmic in frequency; missing crossings remain unset.
SweepAnalysis AnalyzeSweepChannel(const std::vector<double>& frequencies,
                                  const std::vector<double>& rms,
                                  double referenceHz = 1000.0);

// Divide a measured response by a loopback reference in linear RMS units.
// Interpolate the reference on a logarithmic frequency/amplitude scale.
// Returns empty when the reference does not cover the measurement or data is invalid.
std::vector<double> CorrectSweepRms(const std::vector<double>& frequencies,
                                    const std::vector<double>& rms,
                                    const std::vector<double>& referenceFrequencies,
                                    const std::vector<double>& referenceRms);

#endif  // AUDMES_SWEEP_PLAN_H
