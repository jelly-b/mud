#include "unity.h"

#include "things_tiny_id.h"

void setUp() {}

void tearDown() {}

void testThingsTinyId(void) {
	uint8_t lanId = 24;
	uint8_t hours = 11;
	uint8_t minutes = 23;
	uint8_t seconds = 52;
	uint16_t milliseconds = 997;

	uint32_t passedTimeThisDay =
		11 * (60 * 60 * 1000) +
		23 * (60 * 1000) +
		52 * 1000 +
		997;

	TinyId requestId = {0};
	if (makeTinyId(lanId, REQUEST, passedTimeThisDay, requestId) != 0)
		TEST_FAIL_MESSAGE("Failed to create things tiny ID.");

	TinyId responseId = {0};
	TEST_ASSERT_EQUAL_INT_MESSAGE(0, makeResponseTinyId(requestId, responseId),
		"Failed to make response things tiny ID.");

	struct ThingsTinyIdModel responseModel;
	TEST_ASSERT_EQUAL_INT_MESSAGE(0, getTinyIdModel(responseId, &responseModel),
		"Failed to get things tiny ID model.");

	TEST_ASSERT_EQUAL_UINT8(lanId, responseModel.lanId);
	TEST_ASSERT_EQUAL_UINT(RESPONSE, responseModel.messageType);
	TEST_ASSERT_EQUAL_UINT8(hours, responseModel.hours);
	TEST_ASSERT_EQUAL_UINT8(minutes, responseModel.minutes);
	TEST_ASSERT_EQUAL_UINT8(seconds, responseModel.seconds);
	TEST_ASSERT_EQUAL_UINT16(milliseconds, responseModel.milliseconds);

	TinyId errorId = {0};
	if(makeErrorTinyId(requestId, errorId) != 0)
		TEST_FAIL_MESSAGE("Failed to make error things tiny ID.");

	TEST_ASSERT_EQUAL_UINT8(lanId, getLanIdFromTinyId(errorId));
	TEST_ASSERT_EQUAL_UINT(ERROR, getMessageTypeFromTinyId(errorId));

	struct ThingsTinyIdModel errorModel;
	TEST_ASSERT_EQUAL_INT_MESSAGE(0, getTinyIdModel(errorId, &errorModel),
		"Failed to get things tiny ID model.");

	TEST_ASSERT_EQUAL_UINT(ERROR, errorModel.messageType);
}

int main() {
	UNITY_BEGIN();
	
	RUN_TEST(testThingsTinyId);
	
	return UNITY_END();
}
