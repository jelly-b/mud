#ifndef MUD_THING_H
#define MUD_THING_H

#include <stdint.h>
#include <stdbool.h>

#include "things_tiny_id.h"
#include "protocols.h"

#define THING_ERROR_LACK_OF_HOOKS -1
#define THING_ERROR_DAC_INTRODUCE -2
#define THING_ERROR_INIT_PROTOCOL_ATTRIBUTES -3
#define THING_ERROR_SET_PROTOCOL_ATTRIBUTE -4
#define THING_ERROR_SET_PROTOCOL_TEXT -5
#define THING_ERROR_PROTOCOL_TRANSLATION -6
#define THING_ERROR_DAC_IS_CONFIGURED -7

typedef enum DacState {
	INITIAL,
	INTRODUCTING,
	ALLOCATING,
	ALLOCATED,
	CONFIGURED
} DacState;

typedef struct ThingInfo {
	uint8_t thingIdSize;
	char *thingId;
	DacState dacState;
	uint8_t *address;
	uint8_t *gatewayUplinkAddress;
	uint8_t *gatewayDownlinkAddress;
} ThingInfo;

void registerResetter(void (*reset)());
void registerTimer(void (*schedule)(int delay, void (*doTask)()));
void registerRadioConfigurer(void (*configureRadio)(uint8_t address[]));
void registerRadioAddressChanger(void (*changeRadioAddress)(uint8_t address[]));
void registerThingInfoLoader(void (*loadThingInfo)(ThingInfo *thingInfo));
void registerThingInfoSaver(void (*saveThingInfo)(ThingInfo *thingInfo));
void registerProtocolsConfigurer(void (*configureProtocols)());
void registerRadioSender(void (*send)(uint8_t address[], uint8_t data[], int dataSize));
void unregisterThingHooks();

void registerActionProtocol(ProtocolDescription protocolDescription,
		uint8_t (*processProtocol)(Protocol *), bool isQueryProtocol);
bool unregisterActionProtocol(uint8_t mnemomic);

int toBeAThing();
int processReceivedData(uint8_t data[], int dataSize);
void sendAndRelease(uint8_t to[], ProtocolData *pData);

uint8_t processLoraDacAllocation(Protocol *protocol);

#endif