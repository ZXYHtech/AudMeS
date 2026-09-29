#include "../loopback_reference.h"

#include <cmath>
#include <iostream>

int main() {
  LoopbackReference original;
  original.api = "windows_wasapi";
  original.recordDevice = "Analog 3/4 (E4x4 Pre)";
  original.playDevice = "Playback 1/2 (E4x4 Pre)";
  original.capturedAt = "2026-09-27 18:08:19";
  original.sampleRate = 44100;
  original.outputChannel = 0;
  original.captureChannel = 0;
  original.levelDbfs = -40.0;
  original.frequencies = {20.0, 1000.0, 20000.0};
  original.rms = {0.00037, 0.00038, 0.00037};
  std::string serialized, error;
  if (!SerializeLoopbackReference(original, &serialized, &error)) {
    std::cerr << "Could not serialize valid Loopback reference: " << error << '\n';
    return 1;
  }
  LoopbackReference restored;
  if (!ParseLoopbackReference(serialized, &restored, &error) ||
      restored.frequencies != original.frequencies || restored.rms != original.rms ||
      restored.recordDevice != original.recordDevice ||
      restored.playDevice != original.playDevice ||
      restored.capturedAt != original.capturedAt ||
      !LoopbackMatchesRoute(restored, original.api, original.recordDevice,
                            original.playDevice, 44100, 0, -40.0) ||
      LoopbackMatchesRoute(restored, original.api, original.recordDevice,
                           original.playDevice, 96000, 0, -40.0) ||
      LoopbackMatchesRoute(restored, original.api, original.recordDevice,
                           original.playDevice, 44100, 0, -60.0) ||
      LoopbackMatchesRoute(restored, original.api, "Analog 1/2 (E4x4 Pre)",
                           original.playDevice, 44100, 0, -40.0)) {
    std::cerr << "Loopback roundtrip or route matching failed\n";
    return 1;
  }
  const std::string truncated = serialized.substr(0, serialized.find("20000,"));
  if (ParseLoopbackReference(truncated, &restored, &error) ||
      ParseLoopbackReference("AUDMES_LOOPBACK_V2\n", &restored, &error) ||
      ParseLoopbackReference(serialized + "unexpected\n", &restored, &error)) {
    std::cerr << "Malformed Loopback file was accepted\n";
    return 1;
  }
  LoopbackReference right = original;
  LoopbackReference voltageReference = original;
  voltageReference.measuredVrmsAt1k = 0.01;
  double volts = -1.0;
  if (!SerializeLoopbackReference(voltageReference, &serialized, &error) ||
      !ParseLoopbackReference(serialized, &restored, &error) ||
      restored.measuredVrmsAt1k != 0.01 ||
      !LoopbackRmsToVrmsAt1k(restored, original.rms[1] * 2.0, &volts) ||
      std::fabs(volts - 0.02) > 1e-12 ||
      LoopbackRmsToVrmsAt1k(original, original.rms[1], &volts) ||
      LoopbackRmsToVrmsAt1k(restored, -1.0, &volts) ||
      LoopbackRmsToVrmsAt1k(restored, 1.0, nullptr)) {
    std::cerr << "External voltage persistence or conversion failed\n";
    return 1;
  }
  voltageReference.measuredVrmsAt1k = -1.0;
  if (!LoopbackToneDbfsToVrms(restored, 1000.0,
          20.0 * std::log10(original.rms[1] * std::sqrt(2.0)), 0, &volts) ||
      std::fabs(volts - 0.01) > 1e-12 ||
      LoopbackToneDbfsToVrms(restored, 2000.0, -40.0, 0, &volts) ||
      LoopbackToneDbfsToVrms(restored, 1000.0, -40.0, 1, &volts) ||
      LoopbackToneDbfsToVrms(restored, 1000.0, 1.0, 0, &volts)) {
    std::cerr << "FFT voltage scale or calibration guard failed\n";
    return 1;
  }
  if (ValidateLoopbackReference(voltageReference, &error)) return 1;
  voltageReference.measuredVrmsAt1k = NAN;
  if (ValidateLoopbackReference(voltageReference, &error)) return 1;
  // Restore V1 test input for the later malformed-file checks.
  SerializeLoopbackReference(original, &serialized, &error);
  right.outputChannel = 1;
  right.rms = {0.00031, 0.00032, 0.00031};
  std::string dual;
  LoopbackReference loadedLeft, loadedRight;
  if (!SerializeDualOutputReferences(original, right, &dual, &error) ||
      !ParseDualOutputReferences(dual, &loadedLeft, &loadedRight, &error) ||
      loadedLeft.rms != original.rms || loadedRight.rms != right.rms ||
      loadedRight.outputChannel != 1 ||
      ParseDualOutputReferences(dual + "extra", &loadedLeft, &loadedRight, &error) ||
      ParseDualOutputReferences(dual.substr(0, dual.size() - 1),
                                &loadedLeft, &loadedRight, &error)) {
    std::cerr << "Dual-output roundtrip or corruption check failed\n";
    return 1;
  }
  DualOutputBalance balance;
  if (!AnalyzeDualOutputBalance(loadedLeft, loadedRight, &balance, &error) ||
      std::fabs(balance.leftMinusRightDbAt1k -
                20.0 * std::log10(original.rms[1] / right.rms[1])) > 1e-9 ||
      std::fabs(balance.maxAbsDifferenceDb -
                20.0 * std::log10(original.rms[0] / right.rms[0])) > 1e-9 ||
      balance.maxDifferenceHz != 20.0 ||
      AnalyzeDualOutputBalance(loadedLeft, loadedRight, nullptr, &error)) {
    std::cerr << "Dual-output balance calculation failed\n";
    return 1;
  }
  right.captureChannel = 1;
  if (AnalyzeDualOutputBalance(original, right, &balance, &error)) {
    std::cerr << "Mismatched dual-output inputs were analysed\n";
    return 1;
  }
  if (SerializeDualOutputReferences(original, right, &dual, &error)) {
    std::cerr << "Mismatched dual-output inputs were accepted\n";
    return 1;
  }
  original.outputChannel = 3;
  if (!SerializeLoopbackReference(original, &serialized, &error) ||
      !ParseLoopbackReference(serialized, &restored, &error) ||
      restored.outputChannel != 3 ||
      !LoopbackMatchesRoute(restored, original.api, original.recordDevice,
                            original.playDevice, 44100, 3, -40.0) ||
      LoopbackMatchesRoute(restored, original.api, original.recordDevice,
                           original.playDevice, 44100, 1, -40.0)) {
    std::cerr << "Differential output baseline was not kept separate\n";
    return 1;
  }
  if (!DifferentialLoopbackReady(original, original.api, original.recordDevice,
                                 original.playDevice, 44100) ||
      DifferentialLoopbackReady(original, original.api, original.recordDevice,
                                 original.playDevice, 96000) ||
      DifferentialLoopbackReady(original, original.api, "Other input",
                                 original.playDevice, 44100)) {
    std::cerr << "Differential baseline readiness or route guard failed\n";
    return 1;
  }
  original.outputChannel = 0;
  if (DifferentialLoopbackReady(original, original.api, original.recordDevice,
                                original.playDevice, 44100)) {
    std::cerr << "Single-output baseline passed differential guard\n";
    return 1;
  }
  original.outputChannel = 3;
  original.rms[1] = 0.0;
  if (SerializeLoopbackReference(original, &serialized, &error)) {
    std::cerr << "Invalid reference signal was accepted\n";
    return 1;
  }
  return 0;
}
