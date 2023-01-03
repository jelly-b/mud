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

static uint8_t messages[MAX_PROTOCOL_DATA_SIZE * 2];
static int messagesLength = 0;

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

	ProtocolName pNIntrodction = {{0xf8, 0x05}, 0x00};
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
	ProtocolName pNNotConfigured = {{0xf8, 0x05}, 0x0a};

	ProtocolDescription pDNotConfigured = createProtocolDescription(TACP_PROTOCOL_NOT_CONFIGURED,
		pNNotConfigured, NULL, 0, true);

	registerInboundProtocol(pDNotConfigured, NULL, false);
}

void registerLoraDacConfiguredProtocol() {
	ProtocolName pNConfigured = {{0xf8, 0x05}, 0x08};

	ProtocolDescription pDConfigured = createProtocolDescription(TACP_PROTOCOL_CONFIGURED,
		pNConfigured, NULL, 0,true);

	registerInboundProtocol(pDConfigured, NULL, false);
}

void registerLoraDacProtocols() {
	registerLoraDacIntroductionProtocol();
	registerLoraDacAllocationProtocol();
	registerLoraDacAllocatedProtocol();
	registerLoraDacIsConfiguredProtocol();
	registerLoraDacNotConfiguredProtocol();
	registerLoraDacConfiguredProtocol();
}

void unregisterLoraDacProtocols() {
	unregisterInboundProtocol(TACP_PROTOCOL_CONFIGURED);
	unregisterInboundProtocol(TACP_PROTOCOL_NOT_CONFIGURED);
	unregisterOutboundProtocol(TACP_PROTOCOL_IS_CONFIGURED);
	unregisterOutboundProtocol(TACP_PROTOCOL_ALLOCATED);
	unregisterInboundProtocol(TACP_PROTOCOL_ALLOCATION);
	unregisterOutboundProtocol(TACP_PROTOCOL_INTRODUCTION);
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

int doDac() {
	uint8_t initialAddress[] = {0xef, 0xee, 0x1f};
	configureRadio(initialAddress);

	registerLoraDacProtocols();

	if (thingInfo.dacState == INITIAL) {
		int result = introduce(thingInfo.thingId);
		if(result != 0) {
			thingInfo.dacState = INITIAL;
			return THING_ERROR_DAC_INTRODUCE;
		}
	} else if (thingInfo.dacState == ALLOCATED) {
		int result = isConfigured(thingInfo.thingId);
		if(result != 0) {
			return THING_ERROR_DAC_IS_CONFIGURED;
		}
	} else {
		return THING_ERROR_INVALID_DAC_STATE;
	}

	return 0;
}

int toBeAThing() {
	if (!checkHooks())
		return THING_ERROR_LACK_OF_HOOKS;

	loadThingInfo(&thingInfo);

	if (thingInfo.dacState == CONFIGURED) {
		configureRadio(thingInfo.address);
		configureProtocols();

		return 0;
	} else {
		return doDac();
	}

	return 0;
}

void cleanMessages() {
	messagesLength = 0;
}

void addMessage(uint8_t data[], int dataSize) {
	memcpy(messages + messagesLength, data, dataSize);
	messagesLength += dataSize;
}

int findProtocolStartPosition() {
	for (int i = 0; i < messagesLength - 1; i++) {
		if (messages[i] == FLAG_DOC_BEGINNING_END) {
			if (messages[i + 1] == FLAG_DOC_BEGINNING_END)
				continue;

			return i;
		}
	}

	return -1;
}

int findProtocolEndPosition(int startPosition) {
	for(int i = startPosition + 1; i < messagesLength; i++) {
		if(messages[i] == FLAG_DOC_BEGINNING_END) {
			return i;
		}
	}

	return -1;
}

int processProtocol(int protocolStartPosition, int protocolEndPosition) {
	ProtocolData pData = {messages + protocolStartPosition, (protocolEndPosition - protocolStartPosition + 1)};
	if (isLanExecution(&pData)) {
		
	} else {
		Protocol protocol;
		int parseResult = parseProtocol(&pData,&protocol);
		if (parseResult != 0) {
			releaseProtocolResources(&protocol);
			return parseResult;
		}

		ProtocolName name;
		if(!getInboundProtocolNameByMnemonic(protocol.mnemonic, &name))
			return TACP_ERROR_UNKNOWN_PROTOCOL_MNEMONIC;

		InboundProtocolRegistration *registration = getInboundProtocolRegistrationByName(name);
		if (!registration)
			return TACP_ERROR_UNKNOWN_PROTOCOL_NAME;

		if (!registration->processProtocol) {
			return TACP_ERROR_NO_REGISTRATED_PROCESSOR;
		}

		registration->processProtocol(&protocol);
		return 0;
	}

	messagesLength -= (protocolEndPosition - protocolStartPosition);
	if (messagesLength > 0)
		memcpy(messages, messages[protocolEndPosition], messagesLength);
}

int processMessage() {
	int protocolStartPosition = findProtocolStartPosition();
	if (protocolStartPosition == -1)
		return;

	int protocolEndPosition = findProtocolEndPosition(protocolStartPosition);
	if (protocolEndPosition == -1)
		return;

	return processProtocol(protocolStartPosition, protocolEndPosition);
}

int processAsAThing(uint8_t data[], int dataSize) {
	if (dataSize > MAX_PROTOCOL_DATA_SIZE * 2)
		return TACP_ERROR_PROTOCOL_DATA_TOO_LARGE;

	if (messagesLength + dataSize > MAX_PROTOCOL_DATA_SIZE * 2) {
		cleanMessages();
	}
	addMessage(data, dataSize);

	return processMessage();
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

void processNotConfigured() {
	thingInfo.gatewayUplinkAddress = NULL;
	thingInfo.gatewayDownlinkAddress = NULL;
	thingInfo.address = NULL;
	thingInfo.dacState = INITIAL;

	saveThingInfo(&thingInfo);

	reset();
}

void processConfigured() {
	thingInfo.dacState = CONFIGURED;

	saveThingInfo(&thingInfo);

	if (configureRadio) {
		unregisterLoraDacProtocols();
		configureRadio(thingInfo.address);
		configureProtocols();
	} else {
		reset();
	}
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
		} else if (isInboundProtocol(&pData, TACP_PROTOCOL_CONFIGURED)) {
			processConfigured();
		} else {
			return TACP_ERROR_INVALID_DAC_STATE;
		}
	} else {
		return TACP_ERROR_INVALID_DAC_STATE;
	}

	return 0;
}

int processReceivedData(uint8_t data[], int dataSize) {
	if (thingInfo.dacState == CONFIGURED) {
		return processAsAThing(data, dataSize);
	} else {
		return processDac(data, dataSize);
	}
}
