#include <stddef.h>
#include <stdio.h>

#include "debug.h"

#define NO_CAUSE_INFO_FOUND 0

static void (*debugOutput)(const char out[]) = NULL;
static bool debugMode = false;

void setDebugMode(bool _debugMode) {
	debugMode = _debugMode;
}

void setDebugOutputter(void (*_debugOutput)(const char out[])) {
	debugOutput = _debugOutput;
}

void debugOut(const char out[]) {
	if(!debugMode || !debugOutput)
		return;

	debugOutput(out);
}

int debugErrorAndReturn(const char out[], int errorNumber) {
	return debugErrorDetailAndReturn(out, errorNumber, NO_CAUSE_INFO_FOUND);
}

int debugErrorDetailAndReturn(const char out[], int errorNumber, int errorNumberOfCause) {
	if(!debugMode || !debugOut)
		return;

	char buff[128];
	if (errorNumberOfCause == NO_CAUSE_INFO_FOUND) {
		sprintf(buff, "Debug Info: %s. Error number: %d.", out, errorNumber);
	} else {
		sprintf(buff, "Debug Info: %s. Error number: %d. Error number of cause: %d.",
			out, errorNumber, errorNumberOfCause);
	}

	debugOut(buff);

	return errorNumber;
}
