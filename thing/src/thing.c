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
		uint8_t (*assembleDomain)(Protocol *, void *), uint8_t (*processDomain)(void *)) {
	registerInboundProtocol(protocolDescription, assembleDomain, processDomain);
}
bool unregisterActionProtocol(uint8_t mnemomic) {
	unregisterInboundProtocol(mnemomic);
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
		pNIntrodction, padsIntroduction, 2, false);

	registerOutboundProtocol(pDIntroduction);
}

void registerLoraDacProtocols() {
	registerLoraDacIntroductionProtocol();
}

int introduce(char *thingId, int thingIdSize) {
	Protocol introduction;
	introduction.mnemonic = TACP_PROTOCOL_INTRODUCTION;
	introduction.attributesSize = 1;
	introduction.attributes = (ProtocolAttribute *)malloc(sizeof(ProtocolAttribute) * 1);

	if (!introduction.attributes)
		return TACP_ERROR_OUT_OF_MEMEORY;
	
	uint8_t aAddress[] ={0xef, 0xee, 0x1f};
	int result = createProtocolBytesAttribute(introduction.attributes,
		TACP_PROTOCOL_INTRODUCTION_ATTRIBUTE_ADDRESS, aAddress, 3);
	if (result != 0)
		return result;

	createProtocolText(&introduction, thingId);
	
	ProtocolData pData;
	translateProtocol(&introduction, &pData);
	releaseProtocolResources(&introduction);

	uint8_t aDacServiceAddress[] = {0xef, 0xef, 0x1f};
	send(aDacServiceAddress, pData.data, pData.dataSize);

	releaseProtocolData(&pData);

	return 0;
}

int toBeAThing() {
	ThingInfo thingInfo;
	loadThingInfo(&thingInfo);

	dacState = thingInfo.dacState;
	if (dacState == INITIAL) {
		uint8_t initialAddress[] = {0xef, 0xee, 0x1f};
		configureRadio(initialAddress);

		registerLoraDacProtocols();
		int result = introduce(thingInfo.thingId, thingInfo.thingIdSize);
		if (result != 0)
			return result;

		dacState = INTRODUCTING;

		return 0;
	} else {
		return -1;
	}
}

int processReceivedData(uint8_t data[], int dataSize) {

}