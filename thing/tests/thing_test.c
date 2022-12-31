#include "unity.h"

#include "tacp.h"
#include "thing.h"

static const char *thingId = "SL-LE01-C980AFE9";
static ThingInfo thingInfoInStorage = {16, "SL-LE01-C980AFE9", INITIAL, NULL, NULL, NULL};
static int resetTimes = 0;
static DacState dacState = INITIAL;

void reset() {}

void configureRadio(uint8_t address[]) {
}

void changeRadioAddress(uint8_t address[]) {}

void loadThingInfo(ThingInfo *thingInfo) {
	thingInfo->thingId = thingInfoInStorage.thingId;
	thingInfo->thingIdSize = thingInfoInStorage.thingIdSize;
	thingInfo->dacState = thingInfoInStorage.dacState;
	thingInfo->address = thingInfoInStorage.address;
	thingInfo->gatewayUplinkAddress = thingInfoInStorage.gatewayUplinkAddress;
	thingInfo->gatewayDownlinkAddress = thingInfoInStorage.gatewayDownlinkAddress;
}

void saveThingInfo(ThingInfo *thingInfo) {
	thingInfoInStorage.thingId = thingInfo->thingId;
	thingInfoInStorage.thingIdSize = thingInfo->thingIdSize;
	thingInfoInStorage.address = thingInfo->address;
	thingInfoInStorage.gatewayUplinkAddress = thingInfo->gatewayUplinkAddress;
	thingInfoInStorage.gatewayDownlinkAddress = thingInfo->gatewayDownlinkAddress;
	thingInfoInStorage.dacState = thingInfo->dacState;
}

void configureProtocols() {}

void sendToGatewayMock1(uint8_t address[], uint8_t data[], int dataSize) {
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

		dacState = ALLOCATED;
	}
}

void sendToGatewayMock2(uint8_t address[], uint8_t data[], int dataSize) {}

void sendToGatewayMock3(uint8_t address[], uint8_t data[], int dataSize) {}

void sendToGatewayMock4(uint8_t address[],uint8_t data[],int dataSize) {}

void registerThingHooks() {
	registerResetter(reset);
	registerRadioConfigurer(configureRadio);
	registerRadioAddressChanger(changeRadioAddress);
	registerThingInfoLoader(loadThingInfo);
	registerThingInfoSaver(saveThingInfo);
	registerProtocolsConfigurer(configureProtocols);
}

void registerInboundIntroductionProtocol() {
	ProtocolAttributeDescription padIntroductionAddress = {
		TACP_PROTOCOL_INTRODUCTION_ATTRIBUTE_ADDRESS, 0x02, TYPE_BYTES};
	
	ProtocolName pNIntrodction ={{0xf8, 0x05}, 0x00};
	ProtocolAttributeDescription padsIntroduction[] ={padIntroductionAddress};
	ProtocolDescription pDIntroduction = createProtocolDescription(TACP_PROTOCOL_INTRODUCTION,
		pNIntrodction, padsIntroduction, 1, true);

	registerInboundProtocol(pDIntroduction, NULL, false);
}

void registerOutboundAllocationProtocol() {
	ProtocolAttributeDescription padGatewayUplinkAddress ={
		TACP_PROTOCOL_ALLOCATION_ATTRIBUTE_GATEWAY_UPLINK_ADDRESS, 0x04, TYPE_BYTES};
	ProtocolAttributeDescription padGatewayDownlinkAddress ={
		TACP_PROTOCOL_ALLOCATION_ATTRIBUTE_GATEWAY_DOWNLINK_ADDRESS, 0x05, TYPE_BYTES};
	ProtocolAttributeDescription padAllocatedAddress ={
		TACP_PROTOCOL_ALLOCATION_ATTRIBUTE_ALLOCATED_ADDRESS, 0x06, TYPE_BYTES};

	ProtocolName pNAllocation = {{0xf8,0x05}, 0x03};
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

void setUp() {
	registerThingHooks();

	registerInboundIntroductionProtocol();
	registerOutboundAllocationProtocol();
	registerInboundAllocatedProtocol();

	ThingInfo thingInfo;
	loadThingInfo(&thingInfo);

	if(thingInfo.dacState == INITIAL) {
		registerRadioSender(sendToGatewayMock1);
	} else if(thingInfo.dacState == ALLOCATED && resetTimes == 1) {
		registerRadioSender(sendToGatewayMock2);
	} else if(thingInfo.dacState == ALLOCATED && resetTimes == 2) {
		registerRadioSender(sendToGatewayMock3);
	} else {
		registerRadioSender(sendToGatewayMock4);
	}
}

void tearDown() {
	reset();
	resetTimes++;
}

void testLoraDacAllocated() {
	ThingInfo thingInfo;
	loadThingInfo(&thingInfo);

	TEST_ASSERT_EQUAL_INT(INITIAL, thingInfo.dacState);
	TEST_ASSERT_NULL(thingInfo.address);
	TEST_ASSERT_NULL(thingInfo.gatewayUplinkAddress);
	TEST_ASSERT_NULL(thingInfo.gatewayDownlinkAddress);

	TEST_ASSERT_EQUAL(0, toBeAThing());

	loadThingInfo(&thingInfo);
	TEST_ASSERT_EQUAL_INT(ALLOCATED, thingInfo.dacState);

	uint8_t expectedGatewayUplinkAddress[] = {0x00, 0xef, 0x17};
	TEST_ASSERT_EQUAL_UINT8_ARRAY(expectedGatewayUplinkAddress, thingInfo.gatewayUplinkAddress, 3);
	uint8_t expectedGatewayDownlinkAddress[] = {0x00, 0x00, 0x17};
	TEST_ASSERT_EQUAL_UINT8_ARRAY(expectedGatewayDownlinkAddress, thingInfo.gatewayDownlinkAddress, 3);
	uint8_t expectedAddress[] = {0x00, 0x01, 0x17};
	TEST_ASSERT_EQUAL_UINT8_ARRAY(expectedAddress, thingInfo.address, 3);
}

void testLoraDacNotConfigured() {
	/*ThingInfo thingInfo;
	loadThingInfo(&thingInfo);
	TEST_ASSERT_EQUAL_INT(ALLOCATED, thingInfo.dacState);

	TEST_ASSERT_EQUAL(0, toBeAThing());*/
}

void testLoraDacConfigured() {
	/*ThingInfo thingInfo;
	loadThingInfo(&thingInfo);
	TEST_ASSERT_EQUAL_INT(ALLOCATED, thingInfo.dacState);

	TEST_ASSERT_EQUAL(0, toBeAThing());*/
}

void testProcessFlashAction() {
	/*ThingInfo thingInfo;
	loadThingInfo(&thingInfo);
	TEST_ASSERT_EQUAL_INT(CONFIGURED, thingInfo.dacState);

	TEST_ASSERT_EQUAL(0, toBeAThing());*/
}

int main() {
	UNITY_BEGIN();
	
	RUN_TEST(testLoraDacAllocated);
	RUN_TEST(testLoraDacNotConfigured);
	RUN_TEST(testLoraDacConfigured);
	RUN_TEST(testProcessFlashAction);
	
	return UNITY_END();
}
