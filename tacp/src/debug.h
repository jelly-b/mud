#ifndef MUD_DEBUG_H
#define MUD_DEBUG_H

#include <stdbool.h>

void setDebugMode(bool debugMode);
void setDebugOutputter(void (*debugOutput)(const char out[]));
void debugOut(const char out[]);
int debugErrorAndReturn(const char out[], int errorNumber);
int debugErrorDetailAndReturn(const char out[], int errorNumber, int errorNumberOfCause);

#endif