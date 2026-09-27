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
  original.rms[1] = 0.0;
  if (SerializeLoopbackReference(original, &serialized, &error)) {
    std::cerr << "Invalid reference signal was accepted\n";
    return 1;
  }
  return 0;
}
