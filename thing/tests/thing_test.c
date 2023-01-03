#include <string.h>

#include "unity.h"

#include "tacp.h"
#include "thing.h"

enum ProtocolsMnemonic {
	PROTOCOL_FLASH,
	PROTOCOL_FLASH_ATTRIBUTE_REPEAT
};

static const char *thingId = "SL-LE01-C980AFE9";
static ThingInfo thingInfoInStorage = {16, "SL-LE01-C980AFE9", INITIAL, NULL, NULL, NULL};
static int resetTimes = 0;
static DacState dacState = INITIAL;
static const uint8_t dacServiceAddress[] = {0xef, 0xef, 0x1f};
static const uint8_t dacClientAddress[] ={0xef, 0xee, 0x1f};
static const uint8_t configuredNodeAddress[] = {0x00, 0x01, 0x17};
static uint8_t nodeAddress[] = {0x00, 0x00, 0x00};

static bool flashProcessed = false;

void resetImpl() {}

void changeRadioAddressImpl(uint8_t address[]) {
	memcpy(nodeAddress, address, 3);
}

void configureRadioImpl(uint8_t address[]) {
	changeRadioAddressImpl(address);
}

uint8_t processFlash(Protocol *protocol) {
	uint8_t repeat;
	TEST_ASSERT_TRUE(getAttributeValueAsByte(protocol, PROTOCOL_FLASH_ATTRIBUTE_REPEAT, &repeat));
	TEST_ASSERT_EQUAL_UINT8(0x05, repeat);

	flashProcessed = true;
	return 0;
}

void configureProtocolsImpl() {
	ProtocolAttributeDescription padFlashRepeat = {
		PROTOCOL_FLASH_ATTRIBUTE_REPEAT, 0x01, TYPE_BYTE};

	ProtocolName pNFlash ={{0xf7, 0x01}, 0x00};
	ProtocolAttributeDescription padsFlash[] = {padFlashRepeat};
	ProtocolDescription pDIntroduction = createProtocolDescription(PROTOCOL_FLASH,
		pNFlash, padsFlash, 1, true);

	registerActionProtocol(pDIntroduction, processFlash, false);
}

void loadThingInfoImpl(ThingInfo *thingInfo) {
	thingInfo->thingId = thingInfoInStorage.thingId;
	thingInfo->thingIdSize = thingInfoInStorage.thingIdSize;
	thingInfo->dacState = thingInfoInStorage.dacState;
	thingInfo->address = thingInfoInStorage.address;
	thingInfo->gatewayUplinkAddress = thingInfoInStorage.gatewayUplinkAddress;
	thingInfo->gatewayDownlinkAddress = thingInfoInStorage.gatewayDownlinkAddress;
}

void saveThingInfoImpl(ThingInfo *thingInfo) {
	thingInfoInStorage.thingId = thingInfo->thingId;
	thingInfoInStorage.thingIdSize = thingInfo->thingIdSize;
	thingInfoInStorage.address = thingInfo->address;
	thingInfoInStorage.gatewayUplinkAddress = thingInfo->gatewayUplinkAddress;
	thingInfoInStorage.gatewayDownlinkAddress = thingInfo->gatewayDownlinkAddress;
	thingInfoInStorage.dacState = thingInfo->dacState;
}

void sendToGatewayMock1(uint8_t address[], uint8_t data[], int dataSize) {
	TEST_ASSERT_EQUAL_UINT8_ARRAY(dacServiceAddress, address, 3);
	TEST_ASSERT_EQUAL_UINT8_ARRAY(dacClientAddress, nodeAddress, 3);

	if (dacState == INITIAL) {
		uint8_t dacServiceAddress[] = {0xef, 0xef, 0x1f};
		TEST_ASSERT_EQUAL_UINT8_ARRAY(dacServiceAddress, address, 3);

		TEST_ASSERT_EQUAL_INT(29, dataSize);
		uint8_t dIntroduction[] = {
			0xff,
				0xf8, 0x05, 0x00, 0x01, 0x80,
					0x02, 0xfb, 0xef, 0xee, 0x1f, 0xfe,
					0x53, 0x4c, 0x2d, 0x4c, 0x45, 0x30, 0x31, 0x2d, 0x43, 0x39, 0x38, 0x30, 0x41, 0x46, 0x45, 0x39,
			0xff
		};
		TEST_ASSERT_EQUAL_UINT8_ARRAY(dIntroduction, data, dataSize);

		ProtocolData pDataIntroduction = {data, dataSize};
		TEST_ASSERT_TRUE(isInboundProtocol(&pDataIntroduction, TACP_PROTOCOL_INTRODUCTION));
		
		Protocol protocol = createEmptyProtocol();
		TEST_ASSERT_EQUAL_INT(0, parseProtocol(&pDataIntroduction, &protocol));
		
		uint8_t *actualAddress = getAttributeValueAsBytes(&protocol, TACP_PROTOCOL_INTRODUCTION_ATTRIBUTE_ADDRESS);
		TEST_ASSERT_NOT_NULL(actualAddress);
		uint8_t expectedAddress[] = {0x03, 0xef, 0xee, 0x1f};
		TEST_ASSERT_EQUAL_UINT8_ARRAY(expectedAddress, actualAddress, 4);

		char *text = getText(&protocol);
		TEST_ASSERT_EQUAL_CHAR_ARRAY(thingId, text, strlen(thingId));

		Protocol pAllocation = createEmptyProtocolByMenmonic(TACP_PROTOCOL_ALLOCATION);

		uint8_t gatewayUplinkAddress[] = {0x00, 0xef, 0x17};
		addBytesAttribute(&pAllocation,
			TACP_PROTOCOL_ALLOCATION_ATTRIBUTE_GATEWAY_UPLINK_ADDRESS, gatewayUplinkAddress, 3);

		uint8_t gatewayDownlinkAddress[] = {0x00, 0x00, 0x17};
		addBytesAttribute(&pAllocation,
			TACP_PROTOCOL_ALLOCATION_ATTRIBUTE_GATEWAY_DOWNLINK_ADDRESS, gatewayDownlinkAddress, 3);

		uint8_t allocatedAddress[] = {0x00, 0x01, 0x17};
		addBytesAttribute(&pAllocation,
			TACP_PROTOCOL_ALLOCATION_ATTRIBUTE_ALLOCATED_ADDRESS, allocatedAddress, 3);

		ProtocolData pDataAllocation;
		TEST_ASSERT_EQUAL_INT(0, translateAndRelease(&pAllocation, &pDataAllocation));

		dacState = ALLOCATING;

		TEST_ASSERT_EQUAL(0, processReceivedData(pDataAllocation.data, pDataAllocation.dataSize));
		releaseProtocolData(&pDataAllocation);
	} else if (dacState == ALLOCATING) {
		ProtocolData pDataAllocated = {data, dataSize};
		TEST_ASSERT_TRUE(isInboundProtocol(&pDataAllocated, TACP_PROTOCOL_ALLOCATED));

		Protocol pAllocated = createEmptyProtocol();
		TEST_ASSERT_EQUAL_INT(0, parseProtocol(&pDataAllocated, &pAllocated));

		char *text = getText(&pAllocated);
		TEST_ASSERT_EQUAL_CHAR_ARRAY(thingId, text, strlen(thingId));
	}
}

void sendToGatewayMock2(uint8_t address[], uint8_t data[], int dataSize) {
	TEST_ASSERT_EQUAL_UINT8_ARRAY(dacServiceAddress, address, 3);
	TEST_ASSERT_EQUAL_UINT8_ARRAY(dacClientAddress, nodeAddress, 3);

	ProtocolData pDataIsConfigured = {data, dataSize};
	TEST_ASSERT_TRUE(isInboundProtocol(&pDataIsConfigured, TACP_PROTOCOL_IS_CONFIGURED));

	Protocol pIsConfigured = createEmptyProtocol();
	TEST_ASSERT_EQUAL_INT(0, parseProtocol(&pDataIsConfigured, &pIsConfigured));

	uint8_t *nodeAddress = getAttributeValueAsBytes(&pIsConfigured, TACP_PROTOCOL_IS_CONFIGURED_ATTRIBUTE_ADDRESS);
	uint8_t expectedNodeAddress[] = {0x03, 0xef, 0xee, 0x1f};
	TEST_ASSERT_EQUAL_UINT8_ARRAY(expectedNodeAddress, nodeAddress, 4);

	char *nodeThingId = getText(&pIsConfigured);
	TEST_ASSERT_EQUAL_CHAR_ARRAY(thingId, nodeThingId, strlen(thingId));

	Protocol pNotConfigured = createEmptyProtocolByMenmonic(TACP_PROTOCOL_NOT_CONFIGURED);
	ProtocolData pDataNotConfigured;
	TEST_ASSERT_EQUAL_INT(0,translateAndRelease(&pNotConfigured, &pDataNotConfigured));

	dacState = INITIAL;

	TEST_ASSERT_EQUAL(0, processReceivedData(pDataNotConfigured.data, pDataNotConfigured.dataSize));
	releaseProtocolData(&pDataNotConfigured);
}

void sendToGatewayMock3(uint8_t address[], uint8_t data[], int dataSize) {
	TEST_ASSERT_EQUAL_UINT8_ARRAY(dacServiceAddress, address, 3);
	TEST_ASSERT_EQUAL_UINT8_ARRAY(dacClientAddress, nodeAddress, 3);

	sendToGatewayMock1(address, data, dataSize);
	if(dacState == ALLOCATING) {
		Protocol pConfigured = createEmptyProtocolByMenmonic(TACP_PROTOCOL_CONFIGURED);

		ProtocolData pDataConfigured;
		TEST_ASSERT_EQUAL_INT(0, translateAndRelease(&pConfigured, &pDataConfigured));

		dacState = INITIAL;

		TEST_ASSERT_EQUAL(0, processReceivedData(pDataConfigured.data, pDataConfigured.dataSize));
		releaseProtocolData(&pDataConfigured);
	}
}

void registerThingHooks() {
	registerResetter(resetImpl);
	registerRadioConfigurer(configureRadioImpl);
	registerRadioAddressChanger(changeRadioAddressImpl);
	registerThingInfoLoader(loadThingInfoImpl);
	registerThingInfoSaver(saveThingInfoImpl);
	registerProtocolsConfigurer(configureProtocolsImpl);
}

void registerInboundIntroductionProtocol() {
	ProtocolAttributeDescription padIntroductionAddress = {
		TACP_PROTOCOL_INTRODUCTION_ATTRIBUTE_ADDRESS, 0x02, TYPE_BYTES};
	
	ProtocolName pNIntrodction ={{0xf8, 0x05}, 0x00};
	ProtocolAttributeDescription padsIntroduction[] = {padIntroductionAddress};
	ProtocolDescription pDIntroduction = createProtocolDescription(TACP_PROTOCOL_INTRODUCTION,
		pNIntrodction, padsIntroduction, 1, true);

	registerInboundProtocol(pDIntroduction, NULL, false);
}

void registerOutboundAllocationProtocol() {
	ProtocolAttributeDescription padGatewayUplinkAddress = {
		TACP_PROTOCOL_ALLOCATION_ATTRIBUTE_GATEWAY_UPLINK_ADDRESS, 0x04, TYPE_BYTES};
	ProtocolAttributeDescription padGatewayDownlinkAddress = {
		TACP_PROTOCOL_ALLOCATION_ATTRIBUTE_GATEWAY_DOWNLINK_ADDRESS, 0x05, TYPE_BYTES};
	ProtocolAttributeDescription padAllocatedAddress = {
		TACP_PROTOCOL_ALLOCATION_ATTRIBUTE_ALLOCATED_ADDRESS, 0x06, TYPE_BYTES};

	ProtocolName pNAllocation = {{0xf8, 0x05}, 0x03};
	ProtocolAttributeDescription padsAllocation[] = {padGatewayUplinkAddress,
		padGatewayDownlinkAddress, padAllocatedAddress};
	ProtocolDescription pDAllocation = createProtocolDescription(TACP_PROTOCOL_ALLOCATION,
		pNAllocation, padsAllocation, 3, false);

	registerOutboundProtocol(pDAllocation);
}

void registerInboundAllocatedProtocol() {
	ProtocolName pNAllocated ={{0xf8, 0x05}, 0x07};
	ProtocolDescription pDAllocated = createProtocolDescription(TACP_PROTOCOL_ALLOCATED,
		pNAllocated, NULL, 0, true);

	registerInboundProtocol(pDAllocated, NULL, false);
}

void registerInboundIsConfiguredProtocol() {
	ProtocolName pNIsConfigured = {{0xf8, 0x05}, 0x09};

	ProtocolAttributeDescription padAddress = {
		TACP_PROTOCOL_IS_CONFIGURED_ATTRIBUTE_ADDRESS, 0x02, TYPE_BYTES};
	ProtocolAttributeDescription padsIsConfigured[] = {padAddress};

	ProtocolDescription pDAllocated = createProtocolDescription(TACP_PROTOCOL_IS_CONFIGURED,
		pNIsConfigured, padsIsConfigured, 1, true);

	registerInboundProtocol(pDAllocated, NULL, false);
}

void registerOutboundNotConfiguredProtocol() {
	ProtocolName pNNotConfigured = {{0xf8, 0x05}, 0x0a};
	ProtocolDescription pDNotConfigured = createProtocolDescription(TACP_PROTOCOL_NOT_CONFIGURED,
		pNNotConfigured, NULL, 0, false);

	registerOutboundProtocol(pDNotConfigured);
}

void registerOutboundConfiguredProtocol() {
	ProtocolName pNConfigured = {{0xf8,0x05}, 0x08};
	ProtocolDescription pDConfigured = createProtocolDescription(TACP_PROTOCOL_CONFIGURED,
		pNConfigured, NULL, 0, false);

	registerOutboundProtocol(pDConfigured);
}

void registerOutboundFlashProtocol() {
	ProtocolAttributeDescription padFlashRepeat = {
		PROTOCOL_FLASH_ATTRIBUTE_REPEAT, 0x01, TYPE_BYTE};

	ProtocolName pNFlash = {{0xf7, 0x01}, 0x00};
	ProtocolAttributeDescription padsFlash[] = {padFlashRepeat};
	ProtocolDescription pDIntroduction = createProtocolDescription(PROTOCOL_FLASH,
		pNFlash, padsFlash, 1, false);

	registerOutboundProtocol(pDIntroduction);
}

void setUp() {
	registerThingHooks();

	registerInboundIntroductionProtocol();
	registerOutboundAllocationProtocol();
	registerInboundAllocatedProtocol();
	registerInboundIsConfiguredProtocol();
	registerOutboundNotConfiguredProtocol();
	registerOutboundConfiguredProtocol();
	registerOutboundFlashProtocol();

	ThingInfo thingInfo;
	loadThingInfoImpl(&thingInfo);

	if(thingInfo.dacState == INITIAL && resetTimes == 0) {
		registerRadioSender(sendToGatewayMock1);
	} else if(thingInfo.dacState == ALLOCATED) {
		registerRadioSender(sendToGatewayMock2);
	} else if(thingInfo.dacState == INITIAL && resetTimes == 2) {
		registerRadioSender(sendToGatewayMock3);
	} else {
		// NOOP
	}

	flashProcessed = false;
}

void tearDown() {
	resetImpl();
	resetTimes++;
}

void testLoraDacAllocated() {
	ThingInfo thingInfo;
	loadThingInfoImpl(&thingInfo);

	TEST_ASSERT_EQUAL_INT(INITIAL, thingInfo.dacState);
	TEST_ASSERT_NULL(thingInfo.address);
	TEST_ASSERT_NULL(thingInfo.gatewayUplinkAddress);
	TEST_ASSERT_NULL(thingInfo.gatewayDownlinkAddress);

	TEST_ASSERT_EQUAL(0, toBeAThing());

	loadThingInfoImpl(&thingInfo);
	TEST_ASSERT_EQUAL_INT(ALLOCATED, thingInfo.dacState);

	uint8_t expectedGatewayUplinkAddress[] = {0x00, 0xef, 0x17};
	TEST_ASSERT_EQUAL_UINT8_ARRAY(expectedGatewayUplinkAddress, thingInfo.gatewayUplinkAddress, 3);
	uint8_t expectedGatewayDownlinkAddress[] = {0x00, 0x00, 0x17};
	TEST_ASSERT_EQUAL_UINT8_ARRAY(expectedGatewayDownlinkAddress, thingInfo.gatewayDownlinkAddress, 3);
	uint8_t expectedAddress[] = {0x00, 0x01, 0x17};
	TEST_ASSERT_EQUAL_UINT8_ARRAY(expectedAddress, thingInfo.address, 3);

	TEST_ASSERT_EQUAL_UINT8_ARRAY(dacClientAddress, nodeAddress, 3);
}

void testLoraDacNotConfigured() {
	ThingInfo thingInfo;
	loadThingInfoImpl(&thingInfo);
	TEST_ASSERT_EQUAL_INT(ALLOCATED, thingInfo.dacState);

	TEST_ASSERT_EQUAL(0, toBeAThing());

	loadThingInfoImpl(&thingInfo);

	TEST_ASSERT_EQUAL_INT(INITIAL, thingInfo.dacState);
	TEST_ASSERT_NULL(thingInfo.address);
	TEST_ASSERT_NULL(thingInfo.gatewayUplinkAddress);
	TEST_ASSERT_NULL(thingInfo.gatewayDownlinkAddress);

	TEST_ASSERT_EQUAL_UINT8_ARRAY(dacClientAddress, nodeAddress, 3);
}

void testLoraDacConfigured() {
	ThingInfo thingInfo;
	loadThingInfoImpl(&thingInfo);
	TEST_ASSERT_EQUAL_INT(INITIAL, thingInfo.dacState);

	TEST_ASSERT_EQUAL(0, toBeAThing());

	loadThingInfoImpl(&thingInfo);
	TEST_ASSERT_EQUAL_INT(CONFIGURED, thingInfo.dacState);

	TEST_ASSERT_EQUAL_UINT8_ARRAY(configuredNodeAddress, nodeAddress, 3);
}

void testProcessFlashAction() {
	ThingInfo thingInfo;
	loadThingInfoImpl(&thingInfo);
	TEST_ASSERT_EQUAL_INT(CONFIGURED, thingInfo.dacState);

	TEST_ASSERT_EQUAL(0, toBeAThing());

	TEST_ASSERT_EQUAL_UINT8_ARRAY(configuredNodeAddress, nodeAddress,3);

	Protocol pFlash = createEmptyProtocolByMenmonic(PROTOCOL_FLASH);
	TEST_ASSERT_EQUAL(0, addByteAttribute(&pFlash, PROTOCOL_FLASH_ATTRIBUTE_REPEAT, 0x05));

	ProtocolData pDataFlash;
	TEST_ASSERT_EQUAL(0, translateAndRelease(&pFlash, &pDataFlash));
	TEST_ASSERT_EQUAL(0, processReceivedData(pDataFlash.data, pDataFlash.dataSize));

	TEST_ASSERT_TRUE(flashProcessed);
}

int main() {
	UNITY_BEGIN();
	
	RUN_TEST(testLoraDacAllocated);
	RUN_TEST(testLoraDacNotConfigured);
	RUN_TEST(testLoraDacConfigured);
	RUN_TEST(testProcessFlashAction);
	
	return UNITY_END();
}
