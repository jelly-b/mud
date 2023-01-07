#ifndef MUD_THINGS_TINY_ID_H
#define MUD_THINGS_TINY_ID_H

#include <stdint.h>
#include <stdbool.h>

#define SIZE_THINGS_TINY_ID 5

#define TINY_ID_ERROR_LAN_ID_OVERFLOW -1
#define TINY_ID_ERROR_INVALID_PASSED_TIME -2
#define TINY_ID_ERROR_INVALID_HOURS -3
#define TINY_ID_ERROR_INVALID_MINUTES -4
#define TINY_ID_ERROR_INVALID_SECONDS -5
#define TINY_ID_ERROR_INVALID_MILLISECONDS -6
#define TINY_ID_ERROR_NOT_ANSWER_MESSAGE_TYPE -7

typedef uint8_t TinyId[SIZE_THINGS_TINY_ID];

typedef enum MessageType {
	REQUEST = 0,
	RESPONSE = 1,
	ERROR = 2
} MessageType;

typedef struct ThingsTinyIdModel {
	uint8_t lanId;
	enum MessageType messageType;
	uint8_t hours;
	uint8_t minutes;
	uint8_t seconds;
	uint16_t milliseconds;
} ThingsTinyIdModel;

int createTinyIdModel(uint8_t lanId, MessageType messageType,
	uint32_t passedTimeThisDay, ThingsTinyIdModel *model);
int makeTinyId(uint8_t lanId, MessageType messageType,
	uint32_t passedTimeThisDay, TinyId tinyId);
int makeTinyIdByModel(ThingsTinyIdModel *model, TinyId tinyId);
bool isAnswerTinyIdOf(const TinyId answerId, const TinyId requestId);
enum MessageType getMessageTypeFromTinyId(const TinyId tinyId);
bool isRequestTinyId(const TinyId tinyId);
bool isResponseTinyId(const TinyId tinyId);
bool isErrorTinyId(const TinyId tinyId);
int getTinyIdModel(const TinyId tinyId, ThingsTinyIdModel *model);
uint8_t getLanIdFromTinyId(const TinyId tinyId);
int makeAnswerTinyId(const TinyId requestId, MessageType messageType, TinyId answerId);
int makeResponseTinyId(const TinyId requestId, TinyId responseId);
int makeErrorTinyId(const TinyId requestId, TinyId errorId);

#endif
