#include "unity.h"

#include "thing.h"

static ThingInfo thingInfoInStorage = {16, "SL-LE01-C980AFE9", INITIAL, NULL, NULL, NULL};
static int resetTimes = 0;
static DacState dacState = INITIAL;

void reset() {
	TEST_ASSERT_EQUAL_INT(0, 1 - 1);
}

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
}

void configureProtocols() {}

void sendToGatewayMock1(uint8_t address[], uint8_t data[], int dataSize) {
	if (dacState == INITIAL) {
		uint8_t dacServiceAddress[] = {0xef, 0xef, 0x1f};
		TEST_ASSERT_EQUAL_UINT8_ARRAY(dacServiceAddress, address, 3);

		TEST_ASSERT_EQUAL_INT(29, dataSize);
		uint8_t introductionData[] = {
			0xff,
				0xf8, 0x05, 0x00, 0x01, 0x80,
					0x02, 0xfb, 0xef, 0xee, 0x1f, 0xfe,
					0x53, 0x4c, 0x2d, 0x4c, 0x45, 0x30, 0x31, 0x2d, 0x43, 0x39, 0x38, 0x30, 0x41, 0x46, 0x45, 0x39,
			0xff
		};
		TEST_ASSERT_EQUAL_UINT8_ARRAY(introductionData, data, dataSize);
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

void setUp() {
	registerThingHooks();

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

	toBeAThing();

	loadThingInfo(&thingInfo);
	TEST_ASSERT_EQUAL_INT(ALLOCATED, thingInfo.dacState);
	TEST_ASSERT_NOT_NULL(thingInfo.address);
	TEST_ASSERT_NOT_NULL(thingInfo.gatewayUplinkAddress);
	TEST_ASSERT_NOT_NULL(thingInfo.gatewayDownlinkAddress);
}

void testLoraDacNotConfigured() {
	/*ThingInfo thingInfo;
	loadThingInfo(&thingInfo);
	TEST_ASSERT_EQUAL_INT(ALLOCATED, thingInfo.dacState);

	toBeAThing();*/
}

void testLoraDacConfigured() {
	/*ThingInfo thingInfo;
	loadThingInfo(&thingInfo);
	TEST_ASSERT_EQUAL_INT(ALLOCATED, thingInfo.dacState);

	toBeAThing();*/
}

void testProcessFlashAction() {
	/*ThingInfo thingInfo;
	loadThingInfo(&thingInfo);
	TEST_ASSERT_EQUAL_INT(CONFIGURED, thingInfo.dacState);

	toBeAThing();*/
}

int main() {
	UNITY_BEGIN();
	
	RUN_TEST(testLoraDacAllocated);
	RUN_TEST(testLoraDacNotConfigured);
	RUN_TEST(testLoraDacNotConfigured);
	RUN_TEST(testProcessFlashAction);
	
	return UNITY_END();
}
