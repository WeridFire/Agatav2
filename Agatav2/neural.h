#pragma once

#include <string>

// Minimal neural inference interface (stubbed for now)
// Option A: value network plugged into alpha-beta. These APIs allow
// enabling/disabling NN evaluation and initializing a model path.

// Enable/disable neural evaluation at runtime
void nn_set_enabled(bool enabled);
bool nn_is_enabled();

// Optional model path; call before enabling. Returns true on success.
bool nn_set_model_path(const std::string& path);
const std::string& nn_get_model_path();

// Initialize/load the model if needed. Safe to call multiple times.
bool nn_init();

// Evaluate current position and return centipawn score from side-to-move perspective.
// Builds features from the engine's global state. Returns 0 if model unavailable.
int nn_value_cp();
