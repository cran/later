#ifndef _LATER_H_
#define _LATER_H_

bool execCallbacks(double timeoutSecs = 0, bool runAll = true);
bool execCallbacksForTopLevel();
bool at_top_level();
bool idle();
extern "C" void execLaterNative(void (*func)(void*), void* data, double secs);

#endif // _LATER_H_
