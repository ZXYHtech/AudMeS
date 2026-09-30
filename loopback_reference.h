#ifndef AUDMES_LOOPBACK_REFERENCE_H
#define AUDMES_LOOPBACK_REFERENCE_H

#include <string>
#include <vector>

struct LoopbackReference {
  std::string api;
  std::string recordDevice;
  std::string playDevice;
  std::string capturedAt;
  unsigned int sampleRate = 0;
  int outputChannel = -1;
  int captureChannel = -1;
  double levelDbfs = 0.0;
  // Externally measured 1 kHz voltage across the calibrated input; zero means unknown.
  double measuredVrmsAt1k = 0.0;
  std::vector<double> frequencies;
  std::vector<double> rms;
};

struct DualOutputBalance {
  double leftMinusRightDbAt1k = 0.0;
  double maxAbsDifferenceDb = 0.0;
  double maxDifferenceHz = 0.0;
};

bool ValidateLoopbackReference(const LoopbackReference& reference, std::string* error);
bool LoopbackRmsToVrmsAt1k(const LoopbackReference& reference, double digitalRms,
                          double* volts);
bool LoopbackToneDbfsToVrms(const LoopbackReference& reference, double frequencyHz,
                           double toneDbfs, int captureChannel, double* volts);
bool LoopbackMatchesRoute(const LoopbackReference& reference, const std::string& api,
                          const std::string& recordDevice, const std::string& playDevice,
                          unsigned int sampleRate, int outputChannel, double levelDbfs);
bool DifferentialLoopbackReady(const LoopbackReference& reference, const std::string& api,
                               const std::string& recordDevice, const std::string& playDevice,
                               unsigned int sampleRate);
bool SerializeLoopbackReference(const LoopbackReference& reference, std::string* contents,
                                std::string* error);
bool ParseLoopbackReference(const std::string& contents, LoopbackReference* reference,
                            std::string* error);
bool ValidateDualOutputReferences(const LoopbackReference& left,
                                  const LoopbackReference& right, std::string* error);
bool AnalyzeDualOutputBalance(const LoopbackReference& left,
                              const LoopbackReference& right,
                              DualOutputBalance* balance, std::string* error);
bool SerializeDualOutputReferences(const LoopbackReference& left,
                                   const LoopbackReference& right,
                                   std::string* contents, std::string* error);
bool ParseDualOutputReferences(const std::string& contents, LoopbackReference* left,
                               LoopbackReference* right, std::string* error);

#endif  // AUDMES_LOOPBACK_REFERENCE_H
