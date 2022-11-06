#include <stdlib.h>

#include "things_tiny_id.h"

int createTTIdModel(uint8_t lanId, enum MessageType messageType,
		uint32_t passedTimeThisDay, struct ThingsTinyIdModel *model) {
	if (lanId > 255)
		return TTID_ERROR_LAN_ID_OVERFLOW;

	if (passedTimeThisDay >= 86400000)
		return TTID_ERROR_INVALID_PASSED_TIME;

	model->lanId = lanId;
	model->messageType = messageType;

	uint32_t remainedTime = passedTimeThisDay;

	uint32_t aHour = 60 * 60 * 1000;
	model->hours = remainedTime / aHour;
	remainedTime = remainedTime % aHour;

	uint32_t aMinute = 60 * 1000;
	model->minutes = remainedTime / aMinute;
	remainedTime = remainedTime % aMinute;

	uint32_t aSecond = 1000;
	model->seconds = remainedTime / aSecond;
	remainedTime = remainedTime % aSecond;

	model->milliseconds = remainedTime;

	return 0;
}

int makeTTId(uint8_t lanId, enum MessageType messageType, uint32_t passedTimeThisDay,
			uint8_t tTId[SIZE_THINGS_TINY_ID]) {
	struct ThingsTinyIdModel model;
	int retValue = createTTIdModel(lanId, messageType, passedTimeThisDay, &model);
	if (retValue != 0)
		return retValue;

	return makeTTIdByModel(&model, tTId);
}

int makeTTIdByModel(struct ThingsTinyIdModel *model, uint8_t tTId[SIZE_THINGS_TINY_ID]) {
	if (model->lanId < 0 || model->lanId > 255)
		return TTID_ERROR_LAN_ID_OVERFLOW;

	if(model->hours < 0 || model->hours > 23)
		return TTID_ERROR_INVALID_HOURS;

	if(model->minutes < 0 || model->minutes > 59)
		return TTID_ERROR_INVALID_MINUTES;

	if(model->seconds < 0 || model->seconds > 59)
		return TTID_ERROR_INVALID_SECONDS;

	if(model->milliseconds < 0 || model->milliseconds > 999)
		return TTID_ERROR_INVALID_MILLISECONDS;

	tTId[0] = model->lanId;
	
	tTId[1] = ((model->messageType << 6) | model->hours);

	tTId[2] = model->minutes;

	uint8_t leftest6BitsOfByte3 = model->seconds;
	uint8_t rightest2BitsOfBytes3 = model->milliseconds >> 8;
	tTId[3] = (uint8_t)((leftest6BitsOfByte3 << 2) | rightest2BitsOfBytes3);

	tTId[4] = (uint8_t)(model->milliseconds & 0xff);

	return 0;
}

bool isAnswerTTIdOf(uint8_t answerId[SIZE_THINGS_TINY_ID], uint8_t requestId[SIZE_THINGS_TINY_ID]) {
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

enum MessageType getMessageType(uint8_t tTId[SIZE_THINGS_TINY_ID]) {
	int iType = (tTId[1] & 0xff) >> 6;

	if (iType == REQUEST)
		return REQUEST;
	else if (iType == RESPONSE)
		return RESPONSE;
	else
		return ERROR;
}

bool isResponseTTId(uint8_t tTId[SIZE_THINGS_TINY_ID]) {
	return false;
}

bool isErrorTTId(uint8_t tTId[SIZE_THINGS_TINY_ID]) {
	return false;
}

int getTTIdModel(uint8_t tTId[SIZE_THINGS_TINY_ID], struct ThingsTinyIdModel *model) {
	model->lanId = tTId[0];
	model->messageType = getMessageType(tTId);
	model->hours = tTId[1] & 0X3f;
	model->minutes = tTId[2];
	model->seconds = tTId[3] >> 2;
	int rightest2BitsOfByte3 = tTId[3] & 0x3;
	model->milliseconds = (rightest2BitsOfByte3 << 8) | tTId[4];

	return 0;
}

uint8_t getLanIdFromTTId(uint8_t tTId[SIZE_THINGS_TINY_ID]) {
	return -1;
}

int makeAnswerTTId(uint8_t requestId[SIZE_THINGS_TINY_ID], enum MessageType messageType,
		uint8_t answerId[SIZE_THINGS_TINY_ID]) {
	if (messageType == REQUEST)
		return TTID_ERROR_NOT_ANSWER_MESSAGE_TYPE;

	answerId[0] = requestId[0];

	uint8_t hours = requestId[1] & 0X3f;
	answerId[1] = (messageType << 6) | hours;

	answerId[2] = requestId[2];
	answerId[3] = requestId[3];
	answerId[4] = requestId[4];

	return 0;
}

int makeResponseTTId(uint8_t requestId[SIZE_THINGS_TINY_ID], uint8_t responseId[SIZE_THINGS_TINY_ID]) {
	return makeAnswerTTId(requestId, RESPONSE, responseId);
}

int makeErrorTTId(uint8_t requestId[SIZE_THINGS_TINY_ID], uint8_t errorId[SIZE_THINGS_TINY_ID]) {
	return makeAnswerTTId(requestId, ERROR, errorId);
}
