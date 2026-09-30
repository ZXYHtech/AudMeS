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

struct SweepRipple {
  bool valid = false;
  double peakToPeakDb = 0.0;
};
// Requires coverage of the entire requested band and valid positive samples.
// Reports sampled ripple with log-interpolated band endpoints; not a continuous bound.
SweepRipple AnalyzeSweepRipple(const std::vector<double>& frequencies,
                               const std::vector<double>& rms,
                               double lowHz = 20.0, double highHz = 20000.0);

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
