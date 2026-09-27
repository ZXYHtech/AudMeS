#ifndef AUDMES_SPECTRUM_METRICS_H
#define AUDMES_SPECTRUM_METRICS_H

#include <vector>

// Each bin contains mean-square signal power (RMS squared), with DC omitted.
// The caller must normalize FFT power by the mean square of the applied window.
struct SpectrumMetrics {
  bool hasFundamental = false;
  bool hasThdn = false;
  bool hasSnr = false;
  bool hasNoiseFloor = false;
  double fundamentalHz = 0.0;
  double fundamentalDbfs = 0.0;
  double thdPercent = 0.0;
  double thdnPercent = 0.0;
  double sinadDb = 0.0;
  double snrDb = 0.0;
  double noiseFloorDbfsPerHz = 0.0;
  double harmonicsDbc[4] = {0.0, 0.0, 0.0, 0.0};
  bool hasHarmonic[4] = {false, false, false, false};
};

// Measures the left channel over 20 Hz to min(20 kHz, Nyquist).
// windowChoice follows the FFT page: Rect, Hann, Blackman, Blackman-Harris.
SpectrumMetrics AnalyzeSpectrum(const std::vector<double>& powerBins, int fftSize,
                                double sampleRate, int windowChoice);

#endif  // AUDMES_SPECTRUM_METRICS_H
