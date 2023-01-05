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

static uint8_t messages[MAX_SIZE_PROTOCOL_DATA * 2];
static int messagesLength = 0;

static const uint8_t dacServiceAddress[] = {0xef, 0xef, 0x1f};
static const uint8_t dacClientAddress[] = {0xef, 0xee, 0x1f};

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

void registerActionProtocol(ProtocolDescription description,
			uint8_t (*processProtocol)(Protocol *), bool isQueryProtocol) {
	registerInboundProtocol(description, processProtocol, isQueryProtocol);
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
	Protocol introduction = createEmptyProtocolByMenmonic(TACP_PROTOCOL_INTRODUCTION);

	if (addBytesAttribute(&introduction, TACP_PROTOCOL_INTRODUCTION_ATTRIBUTE_ADDRESS,
			dacClientAddress, 3) != 0)
		return THING_ERROR_SET_PROTOCOL_ATTRIBUTE;

	if (setText(&introduction, thingId) != 0)
		return THING_ERROR_SET_PROTOCOL_TEXT;

	ProtocolData pData = {NULL, 0};
	if (translateAndRelease(&introduction, &pData) != 0)
		return THING_ERROR_PROTOCOL_TRANSLATION;

	thingInfo.dacState = INTRODUCTING;

	sendAndRelease(dacServiceAddress, &pData);

	return 0;
}

int isConfigured(char *thingId) {
	Protocol isConfigured = createEmptyProtocolByMenmonic(TACP_PROTOCOL_IS_CONFIGURED);

	if(addBytesAttribute(&isConfigured, TACP_PROTOCOL_IS_CONFIGURED_ATTRIBUTE_ADDRESS,
				dacClientAddress, 3) != 0)
		return THING_ERROR_SET_PROTOCOL_ATTRIBUTE;

	if(setText(&isConfigured, thingId) != 0)
		return THING_ERROR_SET_PROTOCOL_TEXT;

	ProtocolData pData = {NULL, 0};
	if(translateAndRelease(&isConfigured, &pData) != 0)
		return THING_ERROR_PROTOCOL_TRANSLATION;
	
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
	configureRadio(dacClientAddress);

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

InboundProtocolRegistration *getInboiundProtoclRegistrationByMnemonic(uint8_t mnemonic) {
	ProtocolName name;
	if(!getInboundProtocolNameByMnemonic(mnemonic, &name))
		return TACP_ERROR_UNKNOWN_PROTOCOL_MNEMONIC;

	return getInboundProtocolRegistrationByName(name);
}

int processProtocol(int protocolStartPosition, int protocolEndPosition) {
	ProtocolData pData = {messages + protocolStartPosition, (protocolEndPosition - protocolStartPosition + 1)};
	if (isLanExecution(&pData)) {
		TinyId requestId;
		Protocol action = createEmptyProtocol();
		int result = parseLanExecution(&pData, requestId, &action);
		if(result != 0) {
			releaseProtocol(&action);
			return TACP_ERROR_FAILED_TO_PARSE_PROTOCOL;
		}

		InboundProtocolRegistration *registration = getInboiundProtoclRegistrationByMnemonic(action.mnemonic);
		if(!registration)
			return TACP_ERROR_UNKNOWN_PROTOCOL_NAME;

		if(!registration->processProtocol) {
			return TACP_ERROR_NO_REGISTRATED_PROCESSOR;
		}

		uint8_t errorNumber = registration->processProtocol(&action);
		if (registration->isQueryProtocol)
			return 0;

		ProtocolData pData = {NULL, 0};
		if (errorNumber == 0) {
			LanAnswer answer = createLanResonse(requestId);
			if (translateLanAnswer(&answer, &pData) != 0) {
				releaseProtocolData(&pData);
				return TACP_ERROR_FAILED_TO_TRANSLATE_ANSWER;
			}
		} else {
			LanAnswer answer = createLanError(requestId, errorNumber);

			if(translateLanAnswer(&answer, &pData) != 0) {
				releaseProtocolData(&pData);
				return TACP_ERROR_FAILED_TO_TRANSLATE_ANSWER;
			}
		}

		sendAndRelease(thingInfo.gatewayUplinkAddress, &pData);
		return 0;
	} else {
		Protocol protocol;
		int result = parseProtocol(&pData, &protocol);
		if (result != 0) {
			releaseProtocol(&protocol);
			return result;
		}

		InboundProtocolRegistration *registration = getInboiundProtoclRegistrationByMnemonic(protocol.mnemonic);
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
	if (protocolStartPosition == -1) {
		cleanMessages();
		return TACP_ERROR_ABANDON_MALFORMED_DATA;
	}

	int protocolEndPosition = findProtocolEndPosition(protocolStartPosition);
	if (protocolEndPosition == -1)
		return TACP_ERROR_WAITING_DATA;

	int result = processProtocol(protocolStartPosition, protocolEndPosition);
	if(protocolEndPosition == messagesLength - 1) {
		cleanMessages();
	} else {
		messagesLength = messagesLength - (protocolEndPosition + 1);
		memcpy(messages, messages + protocolEndPosition + 1, messagesLength);
	}

	return result;
}

int processAsAThing(uint8_t data[], int dataSize) {
	if (dataSize > MAX_SIZE_PROTOCOL_DATA * 2)
		return TACP_ERROR_PROTOCOL_DATA_TOO_LARGE;

	if (messagesLength + dataSize > MAX_SIZE_PROTOCOL_DATA * 2) {
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

	Protocol allocated = createEmptyProtocolByMenmonic(TACP_PROTOCOL_ALLOCATED);
	setText(&allocated, thingInfo.thingId);

	ProtocolData pData = {NULL, 0};
	int result = translateAndRelease(&allocated, &pData);
	if(result != 0) {
		releaseProtocolData(&pData);
		return TACP_ERROR_FAILED_TO_TRANSLATE_PROTOCOL;
	}

	sendAndRelease(dacServiceAddress, &pData);

	return 0;
}

int processAllocation(Protocol *allocation) {
	uint8_t *gatewayUplinkAddress = getAttributeValueAsBytes(allocation, TACP_PROTOCOL_ALLOCATION_ATTRIBUTE_GATEWAY_UPLINK_ADDRESS);
	uint8_t *gatewayDownlinkAddress = getAttributeValueAsBytes(allocation, TACP_PROTOCOL_ALLOCATION_ATTRIBUTE_GATEWAY_DOWNLINK_ADDRESS);
	uint8_t *allocatedAddress = getAttributeValueAsBytes(allocation, TACP_PROTOCOL_ALLOCATION_ATTRIBUTE_ALLOCATED_ADDRESS);

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

		Protocol allocation = createEmptyProtocol();

		int result = parseProtocol(&pData, &allocation);
		if (result != 0) {
			releaseProtocolData(&pData);
			return TACP_ERROR_FAILED_TO_PARSE_PROTOCOL;
		}

		processAllocation(&allocation);
		releaseProtocol(&allocation);
	} else if (thingInfo.dacState == ALLOCATED) {
		if (isInboundProtocol(&pData, TACP_PROTOCOL_NOT_CONFIGURED)) {
			Protocol notConfigured = createEmptyProtocol();

			int result = parseProtocol(&pData, &notConfigured);
			if(result != 0) {
				releaseProtocolData(&pData);
				return TACP_ERROR_FAILED_TO_PARSE_PROTOCOL;

			}

			releaseProtocol(&notConfigured);
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
