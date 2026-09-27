#include "../sweep_plan.h"

#include <cmath>
#include <iostream>
#include <vector>

int main() {
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
  return 0;
}
