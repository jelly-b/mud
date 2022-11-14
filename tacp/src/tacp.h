#ifndef TACP_TACP_H
#define TACP_TACP_H

#include <stdint.h>
#include <stdbool.h>

#include "things_tiny_id.h"
#include "protocols.h"
#include "lora.h"

int (*assembleDomain)(struct Protocol *, void *);
int (*processDomain)(void *);

void registerTacpLoraProtocols();
void unregisterTacpLoraProtocols();

int parseLanExecution(uint8_t data[], struct Protocol *action, TinyId tinyId);
int translateLanExecutionResponse(TinyId requestId, uint8_t *data);
int translateLanExecutionError(TinyId requestId, int errorNumber, uint8_t *data);
int translateLanNotify(struct Protocol *event, uint8_t *data);

int processReceivedTacpData(uint8_t data[]);

#endif