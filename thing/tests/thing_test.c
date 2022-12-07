#include "unity.h"

#include "thing.h"

static ThingInfo thingInfoInStorage = {16, "SL-LE01-EA41B9F8", INITIAL, 0, NULL};

void setUp() {}

void tearDown() {}

void reset() {
	TEST_ASSERT_EQUAL_INT(0, 1 - 1);
}

void configureRadio(uint8_t address[]) {
}

void changeRadioAddress(uint8_t address[]) {}

void loadThingInfo(ThingInfo *thingInfo) {}

void saveThingInfo(ThingInfo *thingInfo) {}

void configureProtocols() {}

void sendToGatewayMock1(uint8_t address[], uint8_t data[], int dataSize) {}

void sendToGatewayMock2(uint8_t address[], uint8_t data[], int dataSize) {}

void sendToGatewayMock3(uint8_t address[], uint8_t data[], int dataSize) {}

void registerThingHooks() {
	registerResetter(reset);
	registerRadioConfigurer(configureRadio);
	registerRadioAddressChanger(changeRadioAddress);
	registerThingInfoLoader(loadThingInfo);
	registerThingInfoSaver(saveThingInfo);
	registerProtocolsConfigurer(configureProtocols);
}

void testToBeAThing() {
	registerThingHooks();

	ThingInfo thingInfo;
	loadThingInfo(&thingInfo);

	if(thingInfo.dacState == INITIAL) {
		registerRadioSender(sendToGatewayMock1);
	} else if (thingInfo.dacState == ALLOCATED) {
		registerRadioSender(sendToGatewayMock2);
	} else {
		registerRadioSender(sendToGatewayMock3);
	}

	toBeAThing();
}

int main() {
	UNITY_BEGIN();
	
	RUN_TEST(testToBeAThing);
	
	return UNITY_END();
}
