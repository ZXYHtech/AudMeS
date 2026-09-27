// Explicit, opt-in analog loopback check using the application's audio backend.
#include "../RWAudio_IO.h"
#include "../fourier.h"
#include "../spectrum_metrics.h"
#include "../sweep_plan.h"

#include <atomic>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <thread>

float *g_OscBuffer_Left = nullptr, *g_OscBuffer_Right = nullptr;
float *g_SpeBuffer_Left = nullptr, *g_SpeBuffer_Right = nullptr;
long int g_OscBufferPosition = 0, g_SpeBufferPosition = 0;
std::atomic<bool> g_OscBufferChanged(false), g_SpeBufferChanged(false);

namespace {
const int kSamples = 32768;
const double kPi = 3.14159265358979323846;

bool Capture(RWAudio& audio, double hz, double gain, std::vector<double>& left,
             std::vector<double>& right, int wantedRecords = 3) {
  // Only change generator settings while the callback is stopped.
  audio.PlaySetGenerator(hz, hz, RWAudio::SINE, RWAudio::SINE, gain, 0.0);
  g_SpeBufferPosition = 0;
  g_SpeBufferChanged.store(false);
  if (audio.StartSnd() != 0) return false;
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
  int records = 0;
  while (std::chrono::steady_clock::now() < deadline && records < wantedRecords) {
    if (g_SpeBufferChanged.load()) {
      ++records;
      if (records == wantedRecords) {
        left.assign(g_SpeBuffer_Left, g_SpeBuffer_Left + kSamples);
        right.assign(g_SpeBuffer_Right, g_SpeBuffer_Right + kSamples);
      } else {
        g_SpeBufferChanged.store(false);
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }
  audio.StopSnd();
  audio.PlaySetGenerator(hz, hz, RWAudio::SINE, RWAudio::SINE, 0.0, 0.0);
  return records == wantedRecords;
}

double Rms(const std::vector<double>& samples) {
  double sum = 0.0;
  for (double sample : samples) sum += sample * sample;
  return std::sqrt(sum / samples.size());
}

SpectrumMetrics Metrics(const std::vector<double>& samples, double rate) {
  std::vector<double> input(kSamples), real(kSamples), imag(kSamples), power(kSamples / 2);
  double mean = 0.0, windowPower = 0.0;
  for (double sample : samples) mean += sample / kSamples;
  for (int i = 0; i < kSamples; ++i) {
    const double phase = 2 * kPi * i / kSamples;
    const double w = 0.35875 - 0.48829 * std::cos(phase) +
        0.14128 * std::cos(2 * phase) - 0.01168 * std::cos(3 * phase);
    input[i] = (samples[i] - mean) * w;
    windowPower += w * w;
  }
  fft_double(kSamples, 0, input.data(), nullptr, real.data(), imag.data());
  for (int i = 1; i < kSamples / 2; ++i)
    power[i] = 2 * (real[i] * real[i] + imag[i] * imag[i]) / (kSamples * windowPower);
  return AnalyzeSpectrum(power, kSamples, rate, 3);
}
}  // namespace

int main(int argc, char** argv) {
  const bool toneOnly = argc == 6 && std::string(argv[1]) == "--tone";
  const bool run = toneOnly || (argc == 6 && std::string(argv[1]) == "--loopback");
  const unsigned int rate = run ? static_cast<unsigned int>(std::stoul(argv[4])) : 48000;
  if (rate != 44100 && rate != 48000 && rate != 96000) return 2;
  RWAudio audio;
  std::string info;
  if (audio.InitSnd(kSamples, kSamples, info, rate) != 0) return 2;
  std::cout << info << std::endl;
  RWAudioDevList outputs, inputs;
  audio.GetRWAudioDevices(&outputs, &inputs);
  for (size_t i = 0; i < inputs.card_info.size(); ++i)
    std::cout << "INPUT " << inputs.card_pos[i] << " " << inputs.card_info[i].name
              << " mix_rate=" << inputs.card_info[i].preferredSampleRate << '\n';
  for (size_t i = 0; i < outputs.card_info.size(); ++i)
    std::cout << "OUTPUT " << outputs.card_pos[i] << " " << outputs.card_info[i].name
              << " mix_rate=" << outputs.card_info[i].preferredSampleRate << '\n';
  if (!run) {
    std::cout << "No signal emitted. Usage: --loopback|--tone INPUT_ID OUTPUT_ID 44100|48000|96000 result.csv\n";
    return 0;
  }
  const unsigned int inputId = std::stoul(argv[2]), outputId = std::stoul(argv[3]);
  bool inputFound = false, outputFound = false;
  for (size_t i = 0; i < inputs.card_pos.size(); ++i)
    if (inputs.card_pos[i] == inputId) {
      inputFound = inputs.card_info[i].name.find("E4x4") != std::string::npos &&
                   inputs.card_info[i].name.find("Analog 3/4") != std::string::npos;
    }
  for (size_t i = 0; i < outputs.card_pos.size(); ++i)
    if (outputs.card_pos[i] == outputId) {
      outputFound = outputs.card_info[i].name.find("E4x4") != std::string::npos &&
                    outputs.card_info[i].name.find("Playback 1/2") != std::string::npos;
    }
  if (!inputFound || !outputFound) {
    std::cerr << "Expected E4x4 Analog 3/4 input and Playback 1/2 output; no signal emitted.\n";
    return 2;
  }
  std::ofstream csv(argv[5]);
  if (!csv) return 2;
  csv << std::setprecision(12) << "Hz,GainL,GainR\n";
  audio.SetSndDevices(inputId, outputId, rate);
  std::vector<double> left, right;
  if (!Capture(audio, 1000, 0.0, left, right)) return 3;
  const double silence = Rms(left);
  std::cout << "SILENCE rms=" << silence << std::endl;
  for (double db : {-60.0, -40.0, -20.0}) {
    if (!Capture(audio, 1000, std::pow(10.0, db / 20), left, right,
                 toneOnly && db == -20.0 ? 30 : 3)) return 3;
    const SpectrumMetrics metrics = Metrics(left, rate);
    double peak = 0.0;
    for (double sample : left) peak = std::max(peak, std::fabs(sample));
    std::cout << "TONE output_dbfs=" << db << " input_hz=" << metrics.fundamentalHz
              << " input_dbfs=" << metrics.fundamentalDbfs << " peak=" << peak
              << " thd_percent=" << metrics.thdPercent << " thdn_percent=" << metrics.thdnPercent
              << " sinad_db=" << metrics.sinadDb << " snr_db=" << metrics.snrDb
              << " right_rms=" << Rms(right) << std::endl;
    if (peak >= 0.95 || (db >= -40.0 && (!metrics.hasFundamental ||
        std::fabs(metrics.fundamentalHz - 1000.0) > 1.0 || Rms(left) < silence * 3 ||
        metrics.thdnPercent > 10.0))) {
      std::cerr << "Loopback tone check failed; stopped before sweep.\n";
      return 4;
    }
  }
  if (toneOnly) return 0;
  const auto frequencies = BuildSweepFrequencies(20, rate == 96000 ? 40000 : 20000,
                                                  24, true, rate);
  std::vector<double> leftRms;
  for (double frequency : frequencies) {
    if (!Capture(audio, frequency, 0.01, left, right)) return 3;
    const double rms = Rms(left);
    for (double sample : left) if (std::fabs(sample) >= 0.95) return 4;
    leftRms.push_back(rms);
    csv << frequency << ',' << rms << ',' << Rms(right) << '\n';
    csv.flush();
    std::cout << "SWEEP hz=" << frequency << " left_rms=" << rms << std::endl;
  }
  const auto analysis = AnalyzeSweepChannel(frequencies, leftRms);
  std::cout << "SWEEP reference_db=" << analysis.referenceDb
            << " has_low_cutoff=" << analysis.hasLowCutoff << " low_hz=" << analysis.lowCutoffHz
            << " has_high_cutoff=" << analysis.hasHighCutoff << " high_hz=" << analysis.highCutoffHz
            << " points=" << leftRms.size() << std::endl;
  return analysis.hasReference && leftRms.size() == 24 ? 0 : 5;
}
