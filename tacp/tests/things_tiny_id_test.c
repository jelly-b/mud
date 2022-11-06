#include "unity.h"

#include "things_tiny_id.h"

void setUp() {}

void tearDown() {}

void test_things_tiny_id(void) {
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

	uint8_t requestId[SIZE_THINGS_TINY_ID] = {0};
	if (makeTTId(lanId, REQUEST, passedTimeThisDay, requestId) != 0)
		TEST_FAIL_MESSAGE("Failed to create things tiny ID.");

	uint8_t responseId[SIZE_THINGS_TINY_ID] = {0};
	if (makeResponseTTId(requestId, responseId) != 0)
		TEST_FAIL_MESSAGE("Failed to make response things tiny ID.");

	struct ThingsTinyIdModel responseModel;
	if (getTTIdModel(responseId, &responseModel) != 0)
		TEST_FAIL_MESSAGE("Failed to get things tiny ID model.");

	TEST_ASSERT_EQUAL_UINT8(lanId, responseModel.lanId);
	TEST_ASSERT_EQUAL_UINT(RESPONSE, responseModel.messageType);
	TEST_ASSERT_EQUAL_UINT8(hours, responseModel.hours);
	TEST_ASSERT_EQUAL_UINT8(minutes, responseModel.minutes);
	TEST_ASSERT_EQUAL_UINT8(seconds, responseModel.seconds);
	TEST_ASSERT_EQUAL_UINT16(milliseconds, responseModel.milliseconds);

	uint8_t errorId[SIZE_THINGS_TINY_ID] = {0};
	if(makeErrorTTId(requestId, errorId) != 0)
		TEST_FAIL_MESSAGE("Failed to make error things tiny ID.");

	struct ThingsTinyIdModel errorModel;
	if(getTTIdModel(errorId, &errorModel) != 0)
		TEST_FAIL_MESSAGE("Failed to get things tiny ID model.");

	TEST_ASSERT_EQUAL_UINT(ERROR, errorModel.messageType);
}

int main() {
	UNITY_BEGIN();
	
	RUN_TEST(test_things_tiny_id);
	
	return UNITY_END();
}
