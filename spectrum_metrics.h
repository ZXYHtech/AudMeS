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
  double thdnDb = 0.0;
  double sinadDb = 0.0;
  double snrDb = 0.0;
  double noiseFloorDbfsPerHz = 0.0;
  bool hasIntegratedNoise = false;
  double integratedNoiseDbfs = 0.0;
  double aWeightedNoiseDbfs = 0.0;
  double bandLowHz = 0.0;
  double bandHighHz = 0.0;
  double harmonicsDbc[9] = {};
  bool hasHarmonic[9] = {};
};

// Measures within the requested band, limited to available non-DC FFT bins.
// windowChoice follows the FFT page: Rect, Hann, Blackman, Blackman-Harris.
SpectrumMetrics AnalyzeSpectrum(const std::vector<double>& powerBins, int fftSize,
                                double sampleRate, int windowChoice,
                                double lowHz = 20.0, double highHz = 20000.0);

#endif  // AUDMES_SPECTRUM_METRICS_H
