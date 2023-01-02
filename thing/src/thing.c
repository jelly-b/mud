#include <stdlib.h>
#include <string.h>

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

static ThingInfo thingInfo;

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
		TACP_PROTOCOL_ALLOCATION_ATTRIBUTE_GATEWAY_DOWNLINK_ADDRESS, 0x05, TYPE_BYTES};
	ProtocolAttributeDescription padAllocatedAddress ={
		TACP_PROTOCOL_ALLOCATION_ATTRIBUTE_ALLOCATED_ADDRESS, 0x06, TYPE_BYTES};

	ProtocolName pNAllocation ={{0xf8, 0x05}, 0x03};
	ProtocolAttributeDescription padsAllocation[] ={padGatewayUplinkAddress,
			padGatewayDownlinkAddress, padAllocatedAddress};
	ProtocolDescription pDAllocation = createProtocolDescription(TACP_PROTOCOL_ALLOCATION,
		pNAllocation, padsAllocation, 3, false);

	registerInboundProtocol(pDAllocation, NULL, false);
}

void registerLoraDacAllocatedProtocol() {
	ProtocolName pNAllocated = {{0xf8, 0x05}, 0x07};
	ProtocolDescription pDAllocated = createProtocolDescription(TACP_PROTOCOL_ALLOCATED,
		pNAllocated, NULL, 0, true);

	registerOutboundProtocol(pDAllocated);
}

void registerLoraDacIsConfiguredProtocol() {
	ProtocolName pNIsConfigured = {{0xf8, 0x05}, 0x09};

	ProtocolAttributeDescription padAddress = {
		TACP_PROTOCOL_IS_CONFIGURED_ATTRIBUTE_ADDRESS, 0x02, TYPE_BYTES};
	ProtocolAttributeDescription padsIsConfigured[] = {padAddress};

	ProtocolDescription pDIsConfigured = createProtocolDescription(TACP_PROTOCOL_IS_CONFIGURED,
		pNIsConfigured, padsIsConfigured, 1, true);

	registerOutboundProtocol(pDIsConfigured);
}

void registerLoraDacNotConfiguredProtocol() {
	ProtocolName pNNotConfigured ={{0xf8,0x05}, 0x0a};

	ProtocolDescription pDIsConfigured = createProtocolDescription(TACP_PROTOCOL_NOT_CONFIGURED,
		pNNotConfigured, NULL, 0, true);

	registerInboundProtocol(pDIsConfigured, NULL, false);
}

void registerLoraDacProtocols() {
	registerLoraDacIntroductionProtocol();
	registerLoraDacAllocationProtocol();
	registerLoraDacAllocatedProtocol();
	registerLoraDacIsConfiguredProtocol();
	registerLoraDacNotConfiguredProtocol();
}

void sendAndRelease(uint8_t to[], ProtocolData *pData) {
	send(to, pData->data, pData->dataSize);
	releaseProtocolData(pData);
}

int introduce(char *thingId) {
	Protocol pIntroduction = createEmptyProtocolByMenmonic(TACP_PROTOCOL_INTRODUCTION);

	uint8_t address[] = {0xef, 0xee, 0x1f};
	if (addBytesAttribute(&pIntroduction, TACP_PROTOCOL_INTRODUCTION_ATTRIBUTE_ADDRESS,
			address, 3) != 0)
		return THING_ERROR_SET_PROTOCOL_ATTRIBUTE;

	if (setText(&pIntroduction, thingId) != 0)
		return THING_ERROR_SET_PROTOCOL_TEXT;

	ProtocolData pData;
	if (translateAndRelease(&pIntroduction, &pData) != 0)
		return THING_ERROR_PROTOCOL_TRANSLATION;

	thingInfo.dacState = INTRODUCTING;

	uint8_t dacServiceAddress[] = {0xef, 0xef, 0x1f};
	sendAndRelease(dacServiceAddress, &pData);

	return 0;
}

int isConfigured(char *thingId) {
	Protocol pIsConfigured = createEmptyProtocolByMenmonic(TACP_PROTOCOL_IS_CONFIGURED);

	uint8_t address[] ={0xef, 0xee, 0x1f};
	if(addBytesAttribute(&pIsConfigured, TACP_PROTOCOL_IS_CONFIGURED_ATTRIBUTE_ADDRESS,
				address, 3) != 0)
		return THING_ERROR_SET_PROTOCOL_ATTRIBUTE;

	if(setText(&pIsConfigured, thingId) != 0)
		return THING_ERROR_SET_PROTOCOL_TEXT;

	ProtocolData pData;
	if(translateAndRelease(&pIsConfigured, &pData) != 0)
		return THING_ERROR_PROTOCOL_TRANSLATION;

	uint8_t dacServiceAddress[] = {0xef, 0xef, 0x1f};
	sendAndRelease(dacServiceAddress, &pData);

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

	loadThingInfo(&thingInfo);

	if (thingInfo.dacState != CONFIGURED) {
		uint8_t initialAddress[] ={0xef, 0xee, 0x1f};
		configureRadio(initialAddress);

		registerLoraDacProtocols();
	}

	if (thingInfo.dacState == INITIAL) {
		int result = introduce(thingInfo.thingId);
		if (result != 0) {
			thingInfo.dacState = INITIAL;
			return THING_ERROR_DAC_INTRODUCE;
		}

		return 0;
	} else if (thingInfo.dacState == ALLOCATED) {
		int result = isConfigured(thingInfo.thingId);
		if(result != 0) {
			return THING_ERROR_DAC_IS_CONFIGURED;
		}

		return 0;
	}
}

int processAsAThing(uint8_t data[], int dataSize) {
	return -1;
}

int allocated(uint8_t *gatewayUplinkAddress, uint8_t *gatewayDownlinkAddress,
			uint8_t *allocatedAddress) {
	thingInfo.gatewayUplinkAddress = malloc(sizeof(uint8_t) * gatewayUplinkAddress[0]);
	thingInfo.gatewayDownlinkAddress = malloc(sizeof(uint8_t) * gatewayDownlinkAddress[0]);
	thingInfo.address = malloc(sizeof(uint8_t) * allocatedAddress[0]);

	memcpy(thingInfo.gatewayUplinkAddress, gatewayUplinkAddress + 1, gatewayUplinkAddress[0]);
	memcpy(thingInfo.gatewayDownlinkAddress, gatewayDownlinkAddress + 1, gatewayDownlinkAddress[0]);
	memcpy(thingInfo.address, allocatedAddress + 1, allocatedAddress[0]);

	thingInfo.dacState = ALLOCATED;

	saveThingInfo(&thingInfo);

	Protocol pAllocated = createEmptyProtocolByMenmonic(TACP_PROTOCOL_ALLOCATED);
	setText(&pAllocated, thingInfo.thingId);

	ProtocolData pDataAllocated;
	int translateResult = translateAndRelease(&pAllocated, &pDataAllocated);
	if(translateResult != 0) {
		releaseProtocolData(&pDataAllocated);
		return translateResult;
	}

	uint8_t aDacServiceAddress[] = {0xef, 0xef, 0x1f};
	sendAndRelease(aDacServiceAddress, &pDataAllocated);

	return 0;
}

int processAllocation(Protocol *pAllocation) {
	uint8_t *gatewayUplinkAddress = getAttributeValueAsBytes(pAllocation, TACP_PROTOCOL_ALLOCATION_ATTRIBUTE_GATEWAY_UPLINK_ADDRESS);
	uint8_t *gatewayDownlinkAddress = getAttributeValueAsBytes(pAllocation, TACP_PROTOCOL_ALLOCATION_ATTRIBUTE_GATEWAY_DOWNLINK_ADDRESS);
	uint8_t *allocatedAddress = getAttributeValueAsBytes(pAllocation, TACP_PROTOCOL_ALLOCATION_ATTRIBUTE_ALLOCATED_ADDRESS);

	if (!gatewayUplinkAddress || !gatewayDownlinkAddress || !allocatedAddress)
		return TACP_ERROR_LACK_OF_ALLOCATION_PARAMETERS;

	return allocated(gatewayUplinkAddress, gatewayDownlinkAddress, allocatedAddress);
}

int processNotConfigured() {
	thingInfo.gatewayUplinkAddress = NULL;
	thingInfo.gatewayDownlinkAddress = NULL;
	thingInfo.address = NULL;
	thingInfo.dacState = INITIAL;

	saveThingInfo(&thingInfo);

	reset();
}

int processDac(uint8_t data[], int dataSize) {
	ProtocolData pData = {data, dataSize};

	if (thingInfo.dacState == INTRODUCTING) {
		if (!isInboundProtocol(&pData, TACP_PROTOCOL_ALLOCATION)) {
			return TACP_ERROR_INVALID_DAC_STATE;
		}

		Protocol pAllocation = createEmptyProtocol();

		int parseResult = parseProtocol(&pData, &pAllocation);
		if (parseResult != 0)
			return parseResult;

		processAllocation(&pAllocation);
		releaseProtocolResources(&pAllocation);
	} else if (thingInfo.dacState == ALLOCATED) {
		if (isInboundProtocol(&pData, TACP_PROTOCOL_NOT_CONFIGURED)) {
			Protocol pNotConfigured = createEmptyProtocol();

			int parseResult = parseProtocol(&pData, &pNotConfigured);
			if(parseResult != 0)
				return parseResult;

			releaseProtocolResources(&pNotConfigured);
			processNotConfigured();
		} else if (isInboundProtocol(&pData,TACP_PROTOCOL_CONFIGURED)) {

		} else {
			return TACP_ERROR_INVALID_DAC_STATE;
		}
	} else {
		return TACP_ERROR_INVALID_DAC_STATE;
	}
}

int processReceivedData(uint8_t data[], int dataSize) {
	if (thingInfo.dacState == CONFIGURED) {
		return processAsAThing(data, dataSize);
	} else {
		return processDac(data, dataSize);
	}
}
