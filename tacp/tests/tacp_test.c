#include "unity.h"

#include "tacp.h"

#define CHANGE_MODE_ACTION_ERROR_INVLID_MODE -1

enum Mnemonic {
	PROTOCOL_INTRODUCTION,
	PROTOCOL_INTRODUCTION_ATTRIBUTE_ADDRESS,
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
		flashProtocolAttributes, 1, false);
	registerInboundProtocol(pdFlash, NULL, false);

	ProtocolName introductionProtocolName = {
		{0xf8, 0x05},
		0x00
	};
	ProtocolAttributeDescription introductionProtocolAttributes[] = {
		{
			PROTOCOL_INTRODUCTION_ATTRIBUTE_ADDRESS,
			0x02,
			TYPE_BYTES
		}
	};
	ProtocolDescription pdIntroduction = createProtocolDescription(PROTOCOL_INTRODUCTION,
		introductionProtocolName, introductionProtocolAttributes, 1, true);
	registerInboundProtocol(pdIntroduction, NULL, false);
}

void unregisterInboundProtocols() {
	unregisterInboundProtocol(PROTOCOL_FLASH);
	unregisterInboundProtocol(PROTOCOL_INTRODUCTION);
}

void setUp() {
	registerInboundProtocols();
}

void tearDown() {
	unregisterInboundProtocols();
}

void testParseInboundProtocols(void) {
	// Flash
	uint8_t flashData[] = {
		0xff,
			0xf7, 0x01, 0x00, 0x01, 0x00,
				0x01, 0x35,
		0xff
	};

	ProtocolData flashPData = CREATE_PROTOCOL_DATA(flashData);
	TEST_ASSERT_TRUE(isInboundProtocol(&flashPData, PROTOCOL_FLASH));

	Protocol flashProtocol;
	TEST_ASSERT_EQUAL_INT(0, parseProtocol(&flashPData, &flashProtocol));
	TEST_ASSERT_EQUAL_INT(1, getAttributesSize(&flashProtocol));
	TEST_ASSERT_EQUAL_INT(PROTOCOL_FLASH_ATTRIBUTE_REPEAT, flashProtocol.attributes->mnemonic);

	int repeat;
	TEST_ASSERT_TRUE(getAttributeValueAsInt(&flashProtocol, PROTOCOL_FLASH_ATTRIBUTE_REPEAT, &repeat));
	TEST_ASSERT_EQUAL_INT(5, repeat);

	releaseProtocolResources(&flashProtocol);

	// Introduction
	uint8_t introductionData[] = {
		0xff,
			0xf8, 0x05, 0x00, 0x01, 0x80,
				0x02, 0xfb, 0xef, 0xee, 0x1f, 0xfe,
				0x53, 0x4c, 0x2d, 0x4c, 0x45, 0x30, 0x31, 0x2d, 0x43, 0x39, 0x38, 0x30, 0x41, 0x46, 0x45, 0x39,
		0xff
	};

	ProtocolData introductionPData = CREATE_PROTOCOL_DATA(introductionData);
	TEST_ASSERT_TRUE(isInboundProtocol(&introductionPData, PROTOCOL_INTRODUCTION));

	Protocol introductionProtocol;
	TEST_ASSERT_EQUAL_INT(0, parseProtocol(&introductionPData, &introductionProtocol));
	TEST_ASSERT_EQUAL_INT(1, getAttributesSize(&introductionProtocol));
	uint8_t *address = getAttributeValueAsBytes(&introductionProtocol, PROTOCOL_INTRODUCTION_ATTRIBUTE_ADDRESS);
	TEST_ASSERT_NOT_NULL(address);
	uint8_t expectedAddress[] = {0x03, 0xef, 0xee, 0x1f};
	TEST_ASSERT_EQUAL_UINT8_ARRAY(expectedAddress, address, 4);

	char *thingId = getText(&introductionProtocol);
	TEST_ASSERT_EQUAL_STRING("SL-LE01-C980AFE9", thingId);

	releaseProtocolResources(&introductionProtocol);
}

int main() {
	UNITY_BEGIN();
	
	RUN_TEST(testParseInboundProtocols);
	
	return UNITY_END();
}
