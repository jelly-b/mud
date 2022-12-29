#include <stdlib.h>

#include "tacp.h"
#include "thing.h"

static void (*reset)() = NULL;
static void (*schedule)(int, void (*)()) = NULL;
static void (*configureRadio)(uint8_t[]) = NULL;
static void (*changeRadioAddress)(uint8_t[]) = NULL;
static void (*loadThingInfo)(ThingInfo *) = NULL;
static void (*saveThingInfo)(ThingInfo *) = NULL;
static void (*configureProtocols)() = NULL;
static void (*send)(uint8_t[], uint8_t[], int) = NULL;

static DacState dacState = INITIAL;

void registerResetter(void (*_reset)()) {
	reset = _reset;
}

void registerTimer(void (*_schedule)(int delay,void (*doTask)())) {
	schedule = _schedule;
}

void registerRadioConfigurer(void (*_configureRadio)(uint8_t address[])) {
	configureRadio = _configureRadio;
}

void registerRadioAddressChanger(void (*_changeRadioAddress)(uint8_t address[])) {
	changeRadioAddress = _changeRadioAddress;
}

void registerThingInfoLoader(void (*_loadThingInfo)(ThingInfo *thingInfo)) {
	loadThingInfo = _loadThingInfo;
}

void registerThingInfoSaver(void (*_saveThingInfo)()) {
	saveThingInfo = _saveThingInfo;
}

void registerProtocolsConfigurer(void (*_configureProtocols)()) {
	configureProtocols = _configureProtocols;
}

void registerRadioSender(void (*_send)(uint8_t address[], uint8_t data[], int dataSize)) {
	send = _send;
}

void registerActionProtocol(ProtocolDescription protocolDescription,
			uint8_t (*processProtocol)(Protocol *), bool isQueryProtocol) {
	registerInboundProtocol(protocolDescription, processProtocol, isQueryProtocol);
}
bool unregisterActionProtocol(uint8_t mnemomic) {
	return unregisterInboundProtocol(mnemomic);
}

void unregisterThingHooks() {
	reset = NULL;
	schedule = NULL;
	configureRadio = NULL;
	changeRadioAddress = NULL;
	loadThingInfo = NULL;
	saveThingInfo = NULL;
	configureProtocols = NULL;
	send = NULL;
}

void registerLoraDacIntroductionProtocol() {
	ProtocolAttributeDescription padIntroductionAddress = {
		TACP_PROTOCOL_INTRODUCTION_ATTRIBUTE_ADDRESS, 0x02, TYPE_BYTES};

	ProtocolName pNIntrodction ={{0xf8, 0x05}, 0x00};
	ProtocolAttributeDescription padsIntroduction[] = {padIntroductionAddress};
	ProtocolDescription pDIntroduction = createProtocolDescription(TACP_PROTOCOL_INTRODUCTION,
		pNIntrodction, padsIntroduction, 1, true);

	registerOutboundProtocol(pDIntroduction);
}

void registerLoraDacAllocationProtocol() {
	ProtocolAttributeDescription padGatewayUplinkAddress ={
		TACP_PROTOCOL_ALLOCATION_ATTRIBUTE_GATEWAY_UPLINK_ADDRESS, 0x04, TYPE_BYTES};
	ProtocolAttributeDescription padGatewayDownlinkAddress = {
		TACP_PROTOCOL_INTRODUCTION_ATTRIBUTE_ADDRESS, 0x05, TYPE_BYTES};
	ProtocolAttributeDescription padAllocatedAddress ={
		TACP_PROTOCOL_ALLOCATION_ATTRIBUTE_ALLOCATED_ADDRESS, 0x06, TYPE_BYTES};

	ProtocolName pNAllocation ={{0xf8,0x05}, 0x03};
	ProtocolAttributeDescription padsAllocation[] ={padGatewayUplinkAddress,
			padGatewayDownlinkAddress, padAllocatedAddress};
	ProtocolDescription pDAllocation = createProtocolDescription(TACP_PROTOCOL_ALLOCATION,
		pNAllocation, padsAllocation, 3, false);

	registerInboundProtocol(pDAllocation, processLoraDacAllocation, false);
}

void registerLoraDacProtocols() {
	registerLoraDacIntroductionProtocol();
}

void sendAndRelease(uint8_t to[], ProtocolData *pData) {
	send(to, pData->data, pData->dataSize);
	releaseProtocolData(pData);
}

int introduce(char *thingId, int thingIdSize) {
	Protocol introduction = createEmptyProtocol();
	introduction.mnemonic = TACP_PROTOCOL_INTRODUCTION;

	uint8_t aAddress[] = {0xef, 0xee, 0x1f};
	if (addBytesAttribute(&introduction, TACP_PROTOCOL_INTRODUCTION_ATTRIBUTE_ADDRESS,
			aAddress, 3) != 0)
		return THING_ERROR_SET_PROTOCOL_ATTRIBUTE;

	if (setText(&introduction, thingId) != 0)
		return THING_ERROR_SET_PROTOCOL_TEXT;

	ProtocolData pData;
	if (translateAndRelease(&introduction, &pData) != 0)
		return THING_ERROR_PROTOCOL_TRANSLATION;

	uint8_t aDacServiceAddress[] = {0xef, 0xef, 0x1f};
	sendAndRelease(aDacServiceAddress, &pData);

	return 0;
}

bool checkHooks() {
	if (loadThingInfo == NULL ||
			saveThingInfo == NULL ||
			send == NULL ||
			configureRadio == NULL ||
			reset == NULL ||
			configureProtocols == NULL) {
		return false;
	}

	return true;
}

int toBeAThing() {
	if (!checkHooks())
		return THING_ERROR_LACK_OF_HOOKS;

	ThingInfo thingInfo;
	loadThingInfo(&thingInfo);

	dacState = thingInfo.dacState;
	if (dacState == INITIAL) {
		uint8_t initialAddress[] = {0xef, 0xee, 0x1f};
		configureRadio(initialAddress);

		registerLoraDacProtocols();
		int result = introduce(thingInfo.thingId, thingInfo.thingIdSize);
		if (result != 0)
			return THING_ERROR_DAC_INTRODUCE;

		dacState = INTRODUCTING;

		return 0;
	} else {
		return -1;
	}
}

int processReceivedData(uint8_t data[], int dataSize) {
	return -1;
}

uint8_t processLoraDacAllocation(Protocol *protocol) {
	return 1;
}

