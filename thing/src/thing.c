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

void registerResetter(void (*_reset)()) {
	reset = _reset;
}

void registerTimer(void (*_schedule)(int, void (*)())) {
	schedule = _schedule;
}

void registerRadioConfigurer(void (*_configureRadio)(uint8_t[])) {
	configureRadio = _configureRadio;
}

void registerRadioAddressChanger(void (*_changeRadioAddress)(uint8_t[])) {
	changeRadioAddress = _changeRadioAddress;
}

void registerThingInfoLoader(void (*_loadThingInfo)(ThingInfo *)) {
	loadThingInfo = _loadThingInfo;
}

void registerThingInfoSaver(void (*_saveThingInfo)(ThingInfo *)) {
	saveThingInfo = _saveThingInfo;
}

void registerProtocolsConfigurer(void (*_configureProtocols)()) {
	configureProtocols = _configureProtocols;
}

void registerRadioSender(void (*_send)(uint8_t[], uint8_t[], int)) {
	send = _send;
}

void registerActionProtocol(ProtocolDescription protocolDescription,
		uint8_t (*assembleDomain)(Protocol *, void *), uint8_t (*processDomain)(void *)) {
	registerInboundProtocol(protocolDescription, assembleDomain, processDomain);
}
bool unregisterActionProtocol(uint8_t mnemomic) {
	unregisterInboundProtocol(mnemomic);
}

void registerDacProtocols() {}

void introduceMe(char *thingId, int thingIdSize) {
	Protocol introduction;
	introduction.mnemonic = PROTOCOL_INTRODUCTION;
	introduction.attributesSize = 2;
	introduction.attributes = malloc(sizeof(ProtocolAttribute) * 2);
	
	introduction.attributes->mnemonic = PROTOCOL_INTRODUCTION_ATTRIBUTE_ADDRESS;
	int *address = malloc(sizeof(int));
	*address = 0xff * 0xff + 0xfd;
	introduction.attributes->value = address;

	introduction.attributes->mnemonic = PROTOCOL_INTRODUCTION_ATTRIBUTE_CHANNEL;
	int *channel = malloc(sizeof(int));
	*channel = 0x31;
	introduction.attributes->value = address;
}

int toBeAThing() {
	ThingInfo thingInfo;
	loadThingInfo(&thingInfo);

	if (thingInfo.dacState == INITIAL) {
		configureRadio(NULL);

		registerDacProtocols();
		introduceMe();
	}
}

int processReceivedData(uint8_t data[], int dataSize) {

}
