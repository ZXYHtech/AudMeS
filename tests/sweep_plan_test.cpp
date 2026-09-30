#include "../sweep_plan.h"

#include <cmath>
#include <iostream>
#include <vector>

int main() {
  const SweepRipple ripple = AnalyzeSweepRipple({20.0, 1000.0, 20000.0},
      {1.0, std::pow(10.0, 1.0 / 20.0), std::pow(10.0, -1.0 / 20.0)});
  const SweepRipple interpolatedRipple = AnalyzeSweepRipple(
      {10.0, 100.0, 1000.0, 10000.0, 40000.0}, {1.0, 1.0, 2.0, 1.0, 1.0});
  if (!ripple.valid || std::fabs(ripple.peakToPeakDb - 2.0) > 1e-10 ||
      !interpolatedRipple.valid ||
      std::fabs(interpolatedRipple.peakToPeakDb - 20.0 * std::log10(2.0)) > 1e-10 ||
      AnalyzeSweepRipple({100.0, 20000.0}, {1.0, 1.0}).valid ||
      AnalyzeSweepRipple({20.0, 20000.0}, {1.0, 0.0}).valid ||
      AnalyzeSweepRipple({20.0, 20.0, 20000.0}, {1.0, 1.0, 1.0}).valid ||
      AnalyzeSweepRipple({20.0, 20000.0}, {1.0, 1.0}, 1000.0, 20.0).valid) {
    std::cerr << "Sweep ripple or invalid-band guard failed\n";
    return 1;
  }
  const std::vector<double> logarithmic =
      BuildSweepFrequencies(20.0, 20000.0, 4, true, 48000.0);
  if (logarithmic.size() != 4 || std::fabs(logarithmic[0] - 20.0) > 1e-9 ||
      std::fabs(logarithmic[1] - 200.0) > 1e-9 ||
      std::fabs(logarithmic[2] - 2000.0) > 1e-9 ||
      std::fabs(logarithmic[3] - 20000.0) > 1e-9) {
    std::cerr << "Logarithmic sweep endpoints or point count failed\n";
    return 1;
  }

  const std::vector<double> linear =
      BuildSweepFrequencies(100.0, 400.0, 4, false, 48000.0);
  if (linear.size() != 4 || std::fabs(linear[1] - 200.0) > 1e-9 ||
      std::fabs(linear[2] - 300.0) > 1e-9 ||
      std::fabs(linear[3] - 400.0) > 1e-9) {
    std::cerr << "Linear sweep spacing failed\n";
    return 1;
  }

  if (BuildSweepFrequencies(20.0, 40000.0, 48, true, 96000.0).size() != 48 ||
      !BuildSweepFrequencies(20.0, 40000.0, 48, true, 44100.0).empty() ||
      !BuildSweepFrequencies(20.0, 20000.0, 1, true, 48000.0).empty() ||
      !BuildSweepFrequencies(0.0, 20000.0, 24, true, 48000.0).empty()) {
    std::cerr << "Sweep input validation failed\n";
    return 1;
  }
  const double minusSix = std::pow(10.0, -6.0 / 20.0);
  const std::vector<double> frequencies = {100.0, 1000.0, 10000.0};
  const SweepAnalysis bandpass =
      AnalyzeSweepChannel(frequencies, {minusSix, 1.0, minusSix});
  if (!bandpass.hasReference || !bandpass.hasLowCutoff || !bandpass.hasHighCutoff ||
      std::fabs(bandpass.lowCutoffHz - std::sqrt(100.0 * 1000.0)) > 1e-6 ||
      std::fabs(bandpass.highCutoffHz - std::sqrt(1000.0 * 10000.0)) > 1e-6) {
    std::cerr << "Sweep -3 dB interpolation failed\n";
    return 1;
  }
  const SweepAnalysis interpolated = AnalyzeSweepChannel(
      {100.0, 500.0, 2000.0, 10000.0}, {minusSix, 1.0, 1.0, minusSix});
  if (!interpolated.hasReference || std::fabs(interpolated.referenceDb) > 1e-9 ||
      !interpolated.hasLowCutoff || !interpolated.hasHighCutoff) {
    std::cerr << "Unsampled reference interpolation failed\n";
    return 1;
  }
  if (AnalyzeSweepChannel({2000.0, 10000.0}, {1.0, minusSix}).hasReference ||
      AnalyzeSweepChannel({100.0, 1000.0}, {1.0, 0.0}).hasReference ||
      AnalyzeSweepChannel({1000.0, 1000.0}, {1.0, 1.0}).hasReference ||
      AnalyzeSweepChannel({100.0, 1000.0}, {1.0}).hasReference) {
    std::cerr << "Invalid sweep reference handling failed\n";
    return 1;
  }
  const SweepAnalysis flat = AnalyzeSweepChannel({100.0, 1000.0, 10000.0},
                                                 {1.0, 1.0, 1.0});
  if (!flat.hasReference || flat.hasLowCutoff || flat.hasHighCutoff) {
    std::cerr << "Flat response must not invent cutoff points\n";
    return 1;
  }
  const std::vector<double> corrected = CorrectSweepRms(
      {100.0, 1000.0, 10000.0}, {0.1, 0.2, 0.1},
      {100.0, 1000.0, 10000.0}, {0.05, 0.1, 0.05});
  if (corrected.size() != 3 || std::fabs(corrected[0] - 2.0) > 1e-10 ||
      std::fabs(corrected[1] - 2.0) > 1e-10 ||
      std::fabs(corrected[2] - 2.0) > 1e-10) {
    std::cerr << "Loopback correction failed\n";
    return 1;
  }
  const std::vector<double> interpolatedCorrection = CorrectSweepRms(
      {1000.0}, {0.1}, {100.0, 10000.0}, {0.01, 1.0});
  if (interpolatedCorrection.size() != 1 ||
      std::fabs(interpolatedCorrection[0] - 1.0) > 1e-10 ||
      !CorrectSweepRms({50.0}, {0.1}, {100.0, 10000.0}, {0.01, 1.0}).empty() ||
      !CorrectSweepRms({1000.0}, {0.1}, {100.0, 10000.0}, {0.0, 1.0}).empty()) {
    std::cerr << "Loopback interpolation or invalid baseline handling failed\n";
    return 1;
  }
  return 0;
}
