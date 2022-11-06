#ifndef THINGS_TINY_ID_H
#define THINGS_TINY_ID_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#define SIZE_THINGS_TINY_ID 5

#define TTID_ERROR_LAN_ID_OVERFLOW -1
#define TTID_ERROR_INVALID_PASSED_TIME -2
#define TTID_ERROR_INVALID_HOURS -3
#define TTID_ERROR_INVALID_MINUTES -4
#define TTID_ERROR_INVALID_SECONDS -5
#define TTID_ERROR_INVALID_MILLISECONDS -6
#define TTID_ERROR_NOT_ANSWER_MESSAGE_TYPE -7

enum MessageType {
	REQUEST = 0,
	RESPONSE = 1,
	ERROR = 2
};

struct ThingsTinyIdModel {
	uint8_t lanId;
	enum MessageType messageType;
	uint8_t hours;
	uint8_t minutes;
	uint8_t seconds;
	uint16_t milliseconds;
};

int createTTIdModel(uint8_t lanId, enum MessageType messageType,
	uint32_t passedTimeThisDay, struct ThingsTinyIdModel *model);
int makeTTIdByModel(struct ThingsTinyIdModel *model, uint8_t tTId[SIZE_THINGS_TINY_ID]);
int makeTTId(uint8_t lanId, enum MessageType messageType, uint32_t passedTimeThisDay,
	uint8_t tTId[SIZE_THINGS_TINY_ID]);
bool isAnswerTTIdOf(uint8_t answerId[SIZE_THINGS_TINY_ID], uint8_t requestId[SIZE_THINGS_TINY_ID]);
enum MessageType getMessageType(uint8_t tTId[SIZE_THINGS_TINY_ID]);
bool isRequestTTId(uint8_t tTId[SIZE_THINGS_TINY_ID]);
bool isResponseTTId(uint8_t tTId[SIZE_THINGS_TINY_ID]);
bool isErrorTTId(uint8_t tTId[SIZE_THINGS_TINY_ID]);
int getTTIdModel(uint8_t tTId[SIZE_THINGS_TINY_ID], struct ThingsTinyIdModel *model);
uint8_t getLanIdFromTTId(uint8_t tTId[SIZE_THINGS_TINY_ID]);
int makeAnswerTTId(uint8_t requestId[SIZE_THINGS_TINY_ID], enum MessageType messgaeType,
	uint8_t answerId[SIZE_THINGS_TINY_ID]);
int makeResponseTTId(uint8_t requestId[SIZE_THINGS_TINY_ID], uint8_t responseId[SIZE_THINGS_TINY_ID]);
int makeErrorTTId(uint8_t requestId[SIZE_THINGS_TINY_ID], uint8_t errorId[SIZE_THINGS_TINY_ID]);

#endif
