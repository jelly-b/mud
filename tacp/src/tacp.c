#include "tacp.h"


void registerTacpLoraProtocols() {

}

void unregisterTacpLoraProtocols() {

}

bool isLanExecution(uint8_t data[]) {
	return false;
}

int parseLanExecution(uint8_t data[], struct Protocol *action, TinyId tinyId) {
	return -1;
}


int translateLanExecutionResponse(TinyId requestId, uint8_t *data) {
	return -1;
}

int translateLanExecutionError(TinyId requestId, int errorNumber, uint8_t *data) {
	return -1;
}

int translateLanNotify(struct Protocol *event, uint8_t *data) {
	return -1;
}

int processReceivedTacpData(uint8_t data[]) {
	return -1;
}
