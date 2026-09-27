#include "loopback_reference.h"

#include <cmath>
#include <iomanip>
#include <limits>
#include <locale>
#include <sstream>

#include "sweep_plan.h"

namespace {
bool Fail(std::string* error, const char* message) {
  if (error) *error = message;
  return false;
}

bool SafeLine(const std::string& value) {
  if (value.empty() || value.size() > 512) return false;
  for (unsigned char character : value)
    if (character < 32 || character == 127) return false;
  return true;
}

bool ReadLine(std::istringstream& input, std::string* line) {
  if (!std::getline(input, *line)) return false;
  if (!line->empty() && line->back() == '\r') line->pop_back();
  return true;
}

bool ReadKey(std::istringstream& input, const std::string& key, std::string* value) {
  std::string line;
  if (!ReadLine(input, &line) || line.compare(0, key.size() + 1, key + "=") != 0)
    return false;
  *value = line.substr(key.size() + 1);
  return true;
}

template <typename Number>
bool ParseNumber(const std::string& value, Number* number) {
  std::istringstream input(value);
  input.imbue(std::locale::classic());
  input >> *number;
  if (!input) return false;
  input >> std::ws;
  return input.eof();
}
}  // namespace

bool ValidateLoopbackReference(const LoopbackReference& reference, std::string* error) {
  if (!SafeLine(reference.api) || !SafeLine(reference.recordDevice) ||
      !SafeLine(reference.playDevice) || !SafeLine(reference.capturedAt))
    return Fail(error, "Missing or invalid device/API/time metadata.");
  if (reference.sampleRate < 8000 || reference.sampleRate > 384000 ||
      reference.outputChannel < 0 || reference.outputChannel > 3 ||
      reference.captureChannel < 0 || reference.captureChannel > 1 ||
      !std::isfinite(reference.levelDbfs) || reference.levelDbfs < -80.0 ||
      reference.levelDbfs > -20.0)
    return Fail(error, "Invalid sample rate, channel, or output level.");
  if (reference.frequencies.size() < 2 || reference.frequencies.size() > 120 ||
      reference.frequencies.size() != reference.rms.size())
    return Fail(error, "Invalid number of sweep points.");
  for (size_t i = 0; i < reference.frequencies.size(); ++i) {
    if (!std::isfinite(reference.frequencies[i]) || reference.frequencies[i] < 20.0 ||
        reference.frequencies[i] >= reference.sampleRate * 0.48 ||
        !std::isfinite(reference.rms[i]) || reference.rms[i] <= 0.0 ||
        (i && reference.frequencies[i] <= reference.frequencies[i - 1]))
      return Fail(error, "Invalid sweep frequency or RMS value.");
  }
  const SweepAnalysis analysis = AnalyzeSweepChannel(reference.frequencies, reference.rms);
  if (!analysis.hasReference || analysis.referenceDb < -100.0)
    return Fail(error, "No usable 1 kHz reference above the noise threshold.");
  return true;
}

bool LoopbackMatchesRoute(const LoopbackReference& reference, const std::string& api,
                          const std::string& recordDevice, const std::string& playDevice,
                          unsigned int sampleRate, int outputChannel, double levelDbfs) {
  return reference.api == api && reference.recordDevice == recordDevice &&
      reference.playDevice == playDevice && reference.sampleRate == sampleRate &&
      reference.outputChannel == outputChannel && std::isfinite(levelDbfs) &&
      std::fabs(reference.levelDbfs - levelDbfs) < 1e-6;
}

bool SerializeLoopbackReference(const LoopbackReference& reference, std::string* contents,
                                std::string* error) {
  if (!contents || !ValidateLoopbackReference(reference, error)) return false;
  std::ostringstream output;
  output.imbue(std::locale::classic());
  output << "AUDMES_LOOPBACK_V1\n"
         << "api=" << reference.api << '\n'
         << "record_device=" << reference.recordDevice << '\n'
         << "play_device=" << reference.playDevice << '\n'
         << "captured_at=" << reference.capturedAt << '\n'
         << "sample_rate=" << reference.sampleRate << '\n'
         << "output_channel=" << reference.outputChannel << '\n'
         << "capture_channel=" << reference.captureChannel << '\n'
         << std::setprecision(17)
         << "level_dbfs=" << reference.levelDbfs << '\n'
         << "points=" << reference.frequencies.size() << '\n'
         << "Hz,Rms\n";
  for (size_t i = 0; i < reference.frequencies.size(); ++i)
    output << reference.frequencies[i] << ',' << reference.rms[i] << '\n';
  *contents = output.str();
  return true;
}

bool ParseLoopbackReference(const std::string& contents, LoopbackReference* reference,
                            std::string* error) {
  if (!reference || contents.size() > 1024 * 1024)
    return Fail(error, "Missing destination or reference file too large.");
  std::istringstream input(contents);
  input.imbue(std::locale::classic());
  std::string line, value;
  LoopbackReference loaded;
  if (!ReadLine(input, &line) || line != "AUDMES_LOOPBACK_V1" ||
      !ReadKey(input, "api", &loaded.api) ||
      !ReadKey(input, "record_device", &loaded.recordDevice) ||
      !ReadKey(input, "play_device", &loaded.playDevice) ||
      !ReadKey(input, "captured_at", &loaded.capturedAt) ||
      !ReadKey(input, "sample_rate", &value))
    return Fail(error, "Invalid Loopback file header or version.");
  long long integer = 0;
  if (!ParseNumber(value, &integer) || integer < 0 ||
      integer > std::numeric_limits<unsigned int>::max())
    return Fail(error, "Invalid sample rate in Loopback file.");
  loaded.sampleRate = static_cast<unsigned int>(integer);
  if (!ReadKey(input, "output_channel", &value) || !ParseNumber(value, &loaded.outputChannel) ||
      !ReadKey(input, "capture_channel", &value) || !ParseNumber(value, &loaded.captureChannel) ||
      !ReadKey(input, "level_dbfs", &value) || !ParseNumber(value, &loaded.levelDbfs) ||
      !ReadKey(input, "points", &value) || !ParseNumber(value, &integer) ||
      integer < 2 || integer > 120 || !ReadLine(input, &line) || line != "Hz,Rms")
    return Fail(error, "Invalid Loopback settings or CSV header.");
  for (int i = 0; i < integer; ++i) {
    if (!ReadLine(input, &line)) return Fail(error, "Truncated Loopback sweep data.");
    const size_t comma = line.find(',');
    double hz = 0.0, rms = 0.0;
    if (comma == std::string::npos || line.find(',', comma + 1) != std::string::npos ||
        !ParseNumber(line.substr(0, comma), &hz) ||
        !ParseNumber(line.substr(comma + 1), &rms))
      return Fail(error, "Invalid Loopback sweep row.");
    loaded.frequencies.push_back(hz);
    loaded.rms.push_back(rms);
  }
  while (ReadLine(input, &line))
    if (!line.empty()) return Fail(error, "Unexpected extra data in Loopback file.");
  if (!ValidateLoopbackReference(loaded, error)) return false;
  *reference = loaded;
  return true;
}

bool ValidateDualOutputReferences(const LoopbackReference& left,
                                  const LoopbackReference& right, std::string* error) {
  if (!ValidateLoopbackReference(left, error) || !ValidateLoopbackReference(right, error))
    return false;
  if (left.outputChannel != 0 || right.outputChannel != 1 ||
      left.captureChannel != right.captureChannel ||
      left.api != right.api || left.recordDevice != right.recordDevice ||
      left.playDevice != right.playDevice || left.sampleRate != right.sampleRate ||
      left.levelDbfs != right.levelDbfs || left.frequencies != right.frequencies)
    return Fail(error, "Dual-output references must share route, input, level, and sweep plan.");
  return true;
}

bool SerializeDualOutputReferences(const LoopbackReference& left,
                                   const LoopbackReference& right,
                                   std::string* contents, std::string* error) {
  if (!contents || !ValidateDualOutputReferences(left, right, error)) return false;
  std::string l, r;
  if (!SerializeLoopbackReference(left, &l, error) ||
      !SerializeLoopbackReference(right, &r, error)) return false;
  *contents = "AUDMES_LOOPBACK_DUAL_V1\nleft_bytes=" + std::to_string(l.size()) +
      "\n" + l + "right_bytes=" + std::to_string(r.size()) + "\n" + r;
  return true;
}

bool ParseDualOutputReferences(const std::string& contents, LoopbackReference* left,
                               LoopbackReference* right, std::string* error) {
  if (!left || !right || contents.size() > 1024 * 1024)
    return Fail(error, "Missing destination or dual-output file too large.");
  const std::string header = "AUDMES_LOOPBACK_DUAL_V1\n";
  if (contents.compare(0, header.size(), header) != 0)
    return Fail(error, "Invalid dual-output file header or version.");
  size_t position = header.size();
  auto readPart = [&](const std::string& key, std::string* part) -> bool {
    const size_t end = contents.find('\n', position);
    if (end == std::string::npos ||
        contents.compare(position, key.size(), key) != 0) return false;
    size_t length = 0;
    if (!ParseNumber(contents.substr(position + key.size(),
                                     end - position - key.size()), &length) ||
        length == 0 || length > 512 * 1024 || length > contents.size() - end - 1)
      return false;
    position = end + 1;
    *part = contents.substr(position, length);
    position += length;
    return true;
  };
  std::string l, r;
  if (!readPart("left_bytes=", &l) || !readPart("right_bytes=", &r) ||
      position != contents.size())
    return Fail(error, "Invalid dual-output section lengths.");
  LoopbackReference loadedLeft, loadedRight;
  if (!ParseLoopbackReference(l, &loadedLeft, error) ||
      !ParseLoopbackReference(r, &loadedRight, error) ||
      !ValidateDualOutputReferences(loadedLeft, loadedRight, error)) return false;
  *left = loadedLeft;
  *right = loadedRight;
  return true;
}
