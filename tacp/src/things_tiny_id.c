#include <stdlib.h>
#include <stdio.h>

#include "debug.h"
#include "things_tiny_id.h"

static const uint32_t MAX_VALUE_PASSED_TIME_THIS_DAY = 86399999;

int createTinyIdModel(uint8_t lanId, MessageType messageType,
		uint32_t passedTimeThisDay, ThingsTinyIdModel *model) {
	if (lanId > 255)
		return TINY_ID_ERROR_LAN_ID_OVERFLOW;

	if (passedTimeThisDay >= MAX_VALUE_PASSED_TIME_THIS_DAY)
		return TINY_ID_ERROR_INVALID_PASSED_TIME;

	model->lanId = lanId;
	model->messageType = messageType;

	uint32_t remainedTime = passedTimeThisDay;

	uint32_t aHour = 3600000;
	model->hours = remainedTime / aHour;
	remainedTime = remainedTime % aHour;

	uint32_t aMinute = 60000;
	model->minutes = remainedTime / aMinute;
	remainedTime = remainedTime % aMinute;

	uint32_t aSecond = 1000;
	model->seconds = remainedTime / aSecond;
	remainedTime = remainedTime % aSecond;

	model->milliseconds = remainedTime;

	return 0;
}

int makeTinyId(uint8_t lanId, MessageType messageType,
			uint32_t passedTimeThisDay, TinyId tinyId) {
	struct ThingsTinyIdModel model;
	int retValue = createTinyIdModel(lanId, messageType, passedTimeThisDay, &model);
	if (retValue != 0)
		return retValue;

	return makeTinyIdByModel(&model, tinyId);
}

int makeTinyIdByModel(ThingsTinyIdModel *model, TinyId tinyId) {
	if (model->lanId < 0 || model->lanId > 255)
		return TINY_ID_ERROR_LAN_ID_OVERFLOW;

	if(model->hours < 0 || model->hours > 23)
		return TINY_ID_ERROR_INVALID_HOURS;

	if(model->minutes < 0 || model->minutes > 59)
		return TINY_ID_ERROR_INVALID_MINUTES;

	if(model->seconds < 0 || model->seconds > 59)
		return TINY_ID_ERROR_INVALID_SECONDS;

	if(model->milliseconds < 0 || model->milliseconds > 999)
		return TINY_ID_ERROR_INVALID_MILLISECONDS;

	tinyId[0] = model->lanId;
	
	tinyId[1] = ((model->messageType << 6) | model->hours);

	tinyId[2] = model->minutes;

	uint8_t leftest6BitsOfByte3 = model->seconds;
	uint8_t rightest2BitsOfBytes3 = model->milliseconds >> 8;
	tinyId[3] = (uint8_t)((leftest6BitsOfByte3 << 2) | rightest2BitsOfBytes3);

	tinyId[4] = (uint8_t)(model->milliseconds & 0xff);

	return 0;
}

bool isAnswerTinyIdOf(const TinyId answerId, const TinyId requestId) {
	if(answerId[0] != requestId[0] ||
			answerId[2] != requestId[2] ||
			answerId[3] != requestId[3] ||
			answerId[4] != requestId[4]) {
		return false;
	}

	int iType = (answerId[1] & 0xff) >> 6;
	if (iType != RESPONSE && iType != ERROR) {
		return false;
	}

	uint8_t requestIdHours = requestId[1] & 0X3f;
	uint8_t answerIdHours = requestId[1] & 0X3f;

	return requestIdHours == answerIdHours;
}

enum MessageType getMessageTypeFromTinyId(const TinyId tinyId) {
	int iType = (tinyId[1] & 0xff) >> 6;

	if (iType == REQUEST)
		return REQUEST;
	else if (iType == RESPONSE)
		return RESPONSE;
	else
		return ERROR;
}

bool isRequestTinyId(const TinyId tinyId) {
	return getMessageTypeFromTinyId(tinyId) == REQUEST;
}

bool isResponseTinyId(const TinyId tinyId) {
	return getMessageTypeFromTinyId(tinyId) == RESPONSE;
}

bool isErrorTinyId(const TinyId tinyId) {
	return getMessageTypeFromTinyId(tinyId) == ERROR;
}

int getTinyIdModel(const TinyId tinyId, ThingsTinyIdModel *model) {
	model->lanId = tinyId[0];
	model->messageType = getMessageTypeFromTinyId(tinyId);
	model->hours = tinyId[1] & 0X3f;
	model->minutes = tinyId[2];
	model->seconds = tinyId[3] >> 2;
	int rightest2BitsOfByte3 = tinyId[3] & 0x3;
	model->milliseconds = (rightest2BitsOfByte3 << 8) | tinyId[4];

	return 0;
}

uint8_t getLanIdFromTinyId(const TinyId tinyId) {
	return tinyId[0];
}

int makeAnswerTinyId(const TinyId requestId, MessageType messageType, TinyId answerId) {
	if (messageType == REQUEST)
		return TINY_ID_ERROR_NOT_ANSWER_MESSAGE_TYPE;

	answerId[0] = requestId[0];

	uint8_t hours = requestId[1] & 0x3f;
	answerId[1] = (messageType << 6) | hours;

	answerId[2] = requestId[2];
	answerId[3] = requestId[3];
	answerId[4] = requestId[4];

	return 0;
}

int makeResponseTinyId(const TinyId requestId, TinyId responseId) {
	return makeAnswerTinyId(requestId, RESPONSE, responseId);
}

int makeErrorTinyId(const TinyId requestId, TinyId errorId) {
	return makeAnswerTinyId(requestId, ERROR, errorId);
}
