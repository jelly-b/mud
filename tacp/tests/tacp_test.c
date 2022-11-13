#include "unity.h"

#include "tacp.h"

#define CHANGE_MODE_ACTION_ERROR_INVLID_MODE -1

enum ActionProcessingStage {
	NONE,
	ASSEMBLED,
	FAILED_TO_ASSEMBLE,
	PROCESSED
};

enum ActionProcessingStage actionProcessingStage;

enum Mnemonic {
	PROTOCOL_INTRODUCTION,
	PROTOCOL_FLASH,
	PROTOCOL_FLASH_ATTRIBUTE_REPEAT,
	PROTOCOL_CHANGE_MODE,
	PROTOCOL_CHANGE_MODE_ATTRIBUTE_MODE
};

struct Introduction {
	char *thingId;
	int address;
	int frequencyBand;

};

struct Flash {
	int repeat;
};

struct ChangeMode {
	char mode;
};

int assembleFlash(struct Protocol *protocol, void *action) {
	TEST_ASSERT_EQUAL_UINT(NONE, actionProcessingStage);

	int repeat;
	TEST_ASSERT_TRUE(getAttributeValueAsInt(protocol, PROTOCOL_FLASH_ATTRIBUTE_REPEAT, &repeat));
	TEST_ASSERT_EQUAL_INT(5, repeat);

	struct Flash *flash = malloc(sizeof(struct Flash));
	flash->repeat = repeat;

	action = flash;

	actionProcessingStage = ASSEMBLED;
	return 0;
}

int processFlash(void *domain) {
	TEST_ASSERT_EQUAL_UINT(ASSEMBLED, actionProcessingStage);

	struct Flash *flash = (struct Flash *)domain;
	TEST_ASSERT_EQUAL_INT(5, flash->repeat);

	actionProcessingStage = PROCESSED;
	return 0;
}

int assembleChangeMode(struct Protocol *protocol, void *domain) {
	TEST_ASSERT_EQUAL_UINT(NONE, actionProcessingStage);

	char *mode;
	TEST_ASSERT_TRUE(getAttributeValueAsString(protocol, PROTOCOL_CHANGE_MODE_ATTRIBUTE_MODE, mode));
	if (!strcmp("A", mode) && !strcmp("W", mode)) {
		actionProcessingStage = FAILED_TO_ASSEMBLE;
		return CHANGE_MODE_ACTION_ERROR_INVLID_MODE;
	}
	
	return 0;
}

int processChangeMode(void *domain) {
	TEST_FAIL_MESSAGE("The program shouldn't run to here.");
}

void setUp() {
	actionProcessingStage = NONE;

	uint8_t flashProtocolName[2] = {0xf7, 0x01};
	struct ProtocolAttributeDescription flashProtocolAttributes[] = {
		{
			PROTOCOL_FLASH_ATTRIBUTE_REPEAT,
			0x01,
			TYPE_INT
		}
	};
	struct ProtocolDescription pdFlash =
		createProtocolDescription(PROTOCOL_FLASH, flashProtocolName, flashProtocolAttributes);

	registerInboundProtocol(pdFlash, assembleFlash, processFlash);

	uint8_t changeModeProtocolName[2] = {0xf7, 0x00};
	struct ProtocolAttributeDescription changeModeProtocolAttributes[] ={
		{
			PROTOCOL_CHANGE_MODE_ATTRIBUTE_MODE,
			0x01,
			TYPE_STRING
		}
	};
	struct ProtocolDescription pdChangeMode =
		createProtocolDescription(PROTOCOL_CHANGE_MODE, changeModeProtocolName,
			changeModeProtocolAttributes);

	registerInboundProtocol(pdChangeMode, assembleChangeMode, processChangeMode);
}

void tearDown() {
	actionProcessingStage = NONE;
}

void test_dac_protocols(void) {

}

void test_registered_action_protocols(void) {
	// Flash
	uint8_t flashData[] ={
		0xff,
			0xf7, 0x01, 0x00, 0x01, 0x00,
				0x01, 0x05,
		0xff
	};

	TEST_ASSERT_EQUAL_INT(0, processReceivedTacpData(flashData));
	TEST_ASSERT_EQUAL_UINT(PROCESSED, actionProcessingStage);

	actionProcessingStage = NONE;
	// ChangeMode
	uint8_t changeModeData[] = {
		0xff,
			0xf8, 0x03, 0x05, 0x00, 0x01,
				0xf7, 0x00, 0x00, 0x01, 0x00,
					0x01, 0x42,
		0xff
	};

	TEST_ASSERT_EQUAL_INT(CHANGE_MODE_ACTION_ERROR_INVLID_MODE, processReceivedTacpData(changeModeData));
	TEST_ASSERT_EQUAL_UINT(FAILED_TO_ASSEMBLE, actionProcessingStage);
}

int main() {
	UNITY_BEGIN();
	
	RUN_TEST(test_dac_protocols);
	RUN_TEST(test_registered_action_protocols);
	
	return UNITY_END();
}
