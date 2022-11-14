#include "unity.h"

#include "protocols.h"

#define CHANGE_MODE_ACTION_ERROR_INVLID_MODE -1

enum Mnemonic {
	PROTOCOL_INTRODUCTION,
	PROTOCOL_FLASH,
	PROTOCOL_FLASH_ATTRIBUTE_REPEAT,
};

struct Flash {
	int repeat;
};

void registerInboundProtocols() {
	ProtocolName flashProtocolName = {
		{0xf7, 0x01},
		0x00
	};
	ProtocolAttributeDescription flashProtocolAttributes[] = {
		{
			PROTOCOL_FLASH_ATTRIBUTE_REPEAT,
			0x01,
			TYPE_INT
		}
	};
	ProtocolDescription pdFlash = createProtocolDescription(PROTOCOL_FLASH, flashProtocolName,
			flashProtocolAttributes, 1);
	registerInboundProtocol(pdFlash);
}

void unregisterInboundProtocols() {
	unregisterInboundProtocol(PROTOCOL_FLASH);
}

void setUp() {
	registerInboundProtocols();
}

void tearDown() {
	unregisterInboundProtocols();
}

void test_parse_action_protocols(void) {
	// Flash
	uint8_t flashData[] = {
		0xff,
			0xf7, 0x01, 0x00, 0x01, 0x00,
				0x01, 0x05, 0xfe,
		0xff
	};

	ProtocolData pData = CREATE_PROTOCOL_DATA(flashData);
	TEST_ASSERT_TRUE(isProtocol(pData, PROTOCOL_FLASH));

	struct Protocol protocol;
	TEST_ASSERT_EQUAL_INT(0, parseProtocol(pData, &protocol));
	TEST_ASSERT_EQUAL_INT(1, protocol.attributesSize);
	TEST_ASSERT_EQUAL_INT(PROTOCOL_FLASH_ATTRIBUTE_REPEAT, protocol.attributes->mnemonic);

	int repeat;
	TEST_ASSERT_EQUAL_INT(0, getAttributeValueAsInt(&protocol, PROTOCOL_FLASH_ATTRIBUTE_REPEAT, &repeat));
	TEST_ASSERT_EQUAL_INT(5, repeat);

	freeProtocolAttributeValues(&protocol);
}

int main() {
	UNITY_BEGIN();
	
	RUN_TEST(test_parse_action_protocols);
	
	return UNITY_END();
}
