#include "fllama.h"
#include "llama.h"
#include "ggml.h"
#include "common.h"


#include <cassert>
#include <cinttypes>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <unordered_set>
#include <thread>
#include <atomic>
#include <mutex>

#if defined(_MSC_VER)
#pragma warning(disable: 4244 4267) // possible loss of data
#endif

static std::atomic_bool stop_generation(false);
static std::mutex continue_mutex;

static llama_model * model;
static llama_context * ctx;
static llama_context * ctx_guidance;
static llama_sampling_context * ctx_sampling;

static std::vector<llama_token> embd;
static std::vector<llama_token> embd_inp;

static int n_remain;
static int n_past;
static int n_consumed;


// A very short-lived native function.
//
// For very short-lived functions, it is fine to call them on the main isolate.
// They will block the Dart execution while running the native function, so
// only do this for native functions which are guaranteed to be short-lived.
FFI_PLUGIN_EXPORT extern "C" intptr_t sum(intptr_t a, intptr_t b) { return a + b; }

// A longer-lived native function, which occupies the thread calling it.
//
// Do not call these kind of native functions in the main isolate. They will
// block Dart execution. This will cause dropped frames in Flutter applications.
// Instead, call these native functions on a separate isolate.
FFI_PLUGIN_EXPORT extern "C" intptr_t sum_long_running(intptr_t a, intptr_t b) {
    ggml_time_init();
    return GGML_MAX_CONTEXTS;

  // Simulate work.
#if _WIN32
  Sleep(5000);
#else
  usleep(5000 * 1000);
#endif
  return a + b;
}

FFI_PLUGIN_EXPORT extern "C" intptr_t llama_cpp_get_constant(void) {
  return GGML_MAX_CONTEXTS;
}
