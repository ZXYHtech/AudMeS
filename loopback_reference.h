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
  std::vector<double> frequencies;
  std::vector<double> rms;
};

bool ValidateLoopbackReference(const LoopbackReference& reference, std::string* error);
bool LoopbackMatchesRoute(const LoopbackReference& reference, const std::string& api,
                          const std::string& recordDevice, const std::string& playDevice,
                          unsigned int sampleRate, int outputChannel, double levelDbfs);
bool SerializeLoopbackReference(const LoopbackReference& reference, std::string* contents,
                                std::string* error);
bool ParseLoopbackReference(const std::string& contents, LoopbackReference* reference,
                            std::string* error);

#endif  // AUDMES_LOOPBACK_REFERENCE_H
