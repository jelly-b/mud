#ifndef MUD_THING_H
#define MUD_THING_H

#include <stdint.h>
#include <stdbool.h>

#include "things_tiny_id.h"
#include "protocols.h"

typedef enum DacState {
	INITIAL,
	ALLOCATED,
	CONFIGURED
} DacState;

typedef struct ThingInfo {
	uint8_t thingIdSize;
	char *thingId;
	DacState dacState;
	uint8_t addressSize;
	uint8_t *address;
} ThingInfo;

void registerResetter(void (*reset)());
void registerTimer(void (*schedule)(int delay, void (*doTask)()));
void registerRadioConfigurer(void (*configureRadio)(uint8_t address[]));
void registerRadioAddressChanger(void (*changeRadioAddress)(uint8_t address[]));
void registerThingInfoLoader(void (*loadThingInfo)(ThingInfo *thingInfo));
void registerThingInfoSaver(void (*saveThingInfo)(ThingInfo *thingInfo));
void registerProtocolsConfigurer(void (*configureProtocols)());
void registerRadioSender(void (*send)(uint8_t address[], uint8_t data[], int dataSize));

void registerActionProtocol(ProtocolDescription protocolDescription,
	uint8_t (*assembleDomain)(Protocol *, void *), uint8_t (*processDomain)(void *));
bool unregisterActionProtocol(uint8_t mnemomic);

int toBeAThing();
int processReceivedData(uint8_t data[], int dataSize);

#endif