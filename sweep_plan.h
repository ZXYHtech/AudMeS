#ifndef AUDMES_SWEEP_PLAN_H
#define AUDMES_SWEEP_PLAN_H

#include <vector>

// Returns exactly `points` frequencies, including both endpoints.
// An empty result means the requested range cannot be measured safely.
std::vector<double> BuildSweepFrequencies(double startHz, double endHz, int points,
                                          bool logarithmic, double sampleRate);

#endif  // AUDMES_SWEEP_PLAN_H
