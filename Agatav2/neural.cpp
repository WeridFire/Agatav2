// Minimal MLP value network implementation (CPU-only, no dependencies)
// - Feature builder uses the engine's global state (externs declared here)
// - Model format: a simple text file with layer sizes and weights
//   Layout:
//     input_size hidden_size output_size
//     W1 (hidden_size x input_size) row-major
//     b1 (hidden_size)
//     W2 (1 x hidden_size)
//     b2 (1)
//   Values are space-separated floats.

#include "neural.h"

#include <atomic>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>
#ifdef _MSC_VER
#include <intrin.h>
#endif

typedef unsigned long long U64;
extern U64 bitboards[12];
extern int side; // 0 white, 1 black
extern int enpassant; // square or no_sq
extern int castle; // wk|wq|bk|bq

static std::atomic<bool> g_nn_enabled{ false };
static std::string g_model_path;

static int g_in = 0, g_hidden = 0;
static std::vector<float> g_W1; // hidden x in
static std::vector<float> g_b1; // hidden
static std::vector<float> g_W2; // 1 x hidden
static float g_b2 = 0.f;
static bool g_loaded = false;

static inline float relu(float x) { return x > 0.f ? x : 0.f; }
static inline float clamp(float x, float a, float b) { return std::max(a, std::min(b, x)); }

// Build a simple feature vector (flat), size = 12*64 + 4 + 8 + 1 = 781
// - 12 one-hot planes for pieces (P..k), flattened to 12*64
// - 4 castling bits (wk,wq,bk,bq)
// - 8 EP file one-hot (0..7) if enpassant != no_sq
// - 1 side-to-move bit (1 if white else 0)
static void build_features(std::vector<float>& x) {
    x.assign(12 * 64 + 4 + 8 + 1, 0.f);

    // pieces
    for (int p = 0; p < 12; ++p) {
        U64 bb = bitboards[p];
        while (bb) {
            unsigned long idx = 0;
#ifdef _MSC_VER
#if defined(_M_X64)
            _BitScanForward64(&idx, bb);
#else
            // Fallback for 32-bit: find index by scanning
            U64 temp = bb & -bb; // isolate LS1B
            while (((1ULL << idx) & temp) == 0ULL && idx < 64) ++idx;
#endif
#else
            // Generic fallback: scan
            U64 temp = bb & -bb;
            while (((1ULL << idx) & temp) == 0ULL && idx < 64) ++idx;
#endif
            bb &= (bb - 1);
            if (idx < 64) x[p * 64 + (int)idx] = 1.f;
        }
    }

    int off = 12 * 64;
    // castling bits order: wk,wq,bk,bq
    x[off + 0] = (castle & 1) ? 1.f : 0.f;
    x[off + 1] = (castle & 2) ? 1.f : 0.f;
    x[off + 2] = (castle & 4) ? 1.f : 0.f;
    x[off + 3] = (castle & 8) ? 1.f : 0.f;

    // EP file one-hot
    int ep_off = off + 4;
    if (enpassant >= 0 && enpassant < 64) {
        int file = enpassant % 8;
        x[ep_off + file] = 1.f;
    }

    // side to move
    x[ep_off + 8] = (side == 0) ? 1.f : 0.f;
}

static bool load_model(const std::string& path) {
    std::ifstream f(path);
    if (!f) return false;
    int outSz = 0;
    f >> g_in >> g_hidden >> outSz;
    if (!f || outSz != 1) return false;

    g_W1.resize((size_t)g_hidden * (size_t)g_in);
    g_b1.resize((size_t)g_hidden);
    g_W2.resize((size_t)g_hidden);

    for (int i = 0; i < g_hidden * g_in; ++i) f >> g_W1[(size_t)i];
    for (int i = 0; i < g_hidden; ++i) f >> g_b1[(size_t)i];
    for (int i = 0; i < g_hidden; ++i) f >> g_W2[(size_t)i];
    f >> g_b2;
    if (!f) return false;

    g_loaded = true;
    return true;
}

void nn_set_enabled(bool enabled) { g_nn_enabled.store(enabled, std::memory_order_relaxed); }
bool nn_is_enabled() { return g_nn_enabled.load(std::memory_order_relaxed); }

bool nn_set_model_path(const std::string& path) { g_model_path = path; return true; }
const std::string& nn_get_model_path() { return g_model_path; }

bool nn_init() {
    if (g_model_path.empty()) return false;
    g_loaded = false;
    return load_model(g_model_path);
}

int nn_value_cp() {
    if (!g_loaded) return 0;

    std::vector<float> x;
    build_features(x);

    // dimension check (allow mismatch by trunc/pad)
    if ((int)x.size() != g_in) x.resize((size_t)g_in, 0.f);

    // hidden = relu(W1*x + b1)
    std::vector<float> h((size_t)g_hidden, 0.f);
    const float* px = x.data();
    const float* W1 = g_W1.data();
    const float* b1 = g_b1.data();
    for (int i = 0; i < g_hidden; ++i) {
        double acc = b1[i];
        const float* wrow = W1 + (size_t)i * (size_t)g_in;
        for (int j = 0; j < g_in; ++j) acc += wrow[j] * px[j];
        h[(size_t)i] = relu((float)acc);
    }

    // y = tanh(W2*h + b2) ∈ [-1,1]; convert to cp
    double acc = g_b2;
    for (int i = 0; i < g_hidden; ++i) acc += g_W2[(size_t)i] * h[(size_t)i];
    float v = std::tanh((float)acc);
    int cp = (int)std::round(v * 800.f);
    cp = (int)clamp((float)cp, -30000.f, 30000.f);
    return cp;
}
