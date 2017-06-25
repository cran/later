#ifndef _WIN32

#include <Rcpp.h>
#include <Rinternals.h>
#include <R_ext/eventloop.h>
#include <unistd.h>
#include <queue>

#include "later.h"
#include "callback_registry.h"

using namespace Rcpp;

extern void* R_GlobalContext;
extern void* R_TopLevelContext;

extern CallbackRegistry callbackRegistry;


// Whether we have initialized the input handler.
int initialized = 0;

// The handles to the read and write ends of a pipe. We use this pipe
// to signal R's input handler callback mechanism that we want to be
// called back.
int pipe_in, pipe_out;

// Whether the file descriptor is ready for reading, i.e., whether
// the input handler callback is scheduled to be called. We use this
// to avoid unnecessarily writing to the pipe.
int hot = 0;

// The buffer we're using for the pipe. This doesn't have to be large,
// in theory it only ever holds zero or one byte.
size_t BUF_SIZE = 256;
void *buf;

static void async_input_handler(void *data) {
  if (!at_top_level()) {
    // It's not safe to run arbitrary callbacks when other R code
    // is already running. Wait until we're back at the top level.
    return;
  }
  
  execCallbacks();

  if (idle()) {
    if (read(pipe_out, buf, BUF_SIZE) < 0) {
      // TODO: This sets a warning but it doesn't get displayed until
      // after the next R command is executed. Can we make it sooner?
      Rf_warning("Failed to read out of pipe for later package");
    }
    hot = 0;
  }
}

InputHandler* inputHandlerHandle;

void ensureInitialized() {
  if (!initialized) {
    buf = malloc(BUF_SIZE);
    
    int pipes[2];
    if (pipe(pipes)) {
      free(buf);
      Rf_error("Failed to create pipe");
      return;
    }
    pipe_out = pipes[0];
    pipe_in = pipes[1];
    
    inputHandlerHandle = addInputHandler(R_InputHandlers, pipe_out, async_input_handler, 20);
    
    initialized = 1;
  }
}

void deInitialize() {
  if (initialized) {
    removeInputHandler(&R_InputHandlers, inputHandlerHandle);
    initialized = 0;
  }
}

void doExecLater(Rcpp::Function callback, double delaySecs) {
  callbackRegistry.add(callback, delaySecs);
  
  if (!hot) {
    ssize_t cbytes = write(pipe_in, "a", 1);
    (void)cbytes; // squelch compiler warning
    hot = 1;
  }
}

void doExecLater(void (*callback)(void*), void* data, double delaySecs) {
  callbackRegistry.add(callback, data, delaySecs);
  
  if (!hot) {
    ssize_t cbytes = write(pipe_in, "a", 1);
    (void)cbytes; // squelch compiler warning
    hot = 1;
  }
}

#endif // ifndef _WIN32
