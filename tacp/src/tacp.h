#ifndef MUD_TACP_H
#define MUD_TACP_H

#include "things_tiny_id.h"
#include "protocols.h"

#define TACP_ERROR_NOT_VALID_PROTOCOL -1
#define TACP_ERROR_UNKNOWN_PROTOCOL_NAME -2
#define TACP_ERROR_FEATURE_EMBEDDED_PROTOCOL_NOT_IMPLEMENTED -3
#define TACP_ERROR_MALFORMED_PROTOCOL_DATA -4
#define TACP_ERROR_UNKNOWN_ATTRIBUTE_NAME -5
#define TACP_ERROR_TEXT_NOT_ACCEPTED -6
#define TACP_ERROR_OUT_OF_MEMEORY -7
#define TACP_ERROR_ATTRIBUTE_DATA_TOO_LARGE -8
#define TACP_ERROR_TEXT_DATA_TOO_LARGE -9
#define TACP_ERROR_UNKNOWN_PROTOCOL_MNEMONIC -10
#define TACP_ERROR_TOO_MANY_ATTRIBUTES -11
#define TACP_ERROR_UNKNOWN_PROTOCOL_ATTRIBUTE_MNEMONIC -12
#define TACP_ERROR_PROTOCOL_DATA_TOO_LARGE -13
#define TACP_ERROR_CHANGE_CLOSED -14
#define TACP_ERROR_INVALID_DAC_STATE -15
#define TACP_ERROR_LACK_OF_ALLOCATION_PARAMETERS -16
#define TACP_ERROR_NO_REGISTRATED_PROCESSOR -16
#define TACP_ERROR_FAILED_TO_ESCAPE -17
#define TACP_ERROR_FAILED_TO_UNESCAPE -18
#define TACP_ERROR_FAILED_TO_TRANSLATE_PROTOCOL -19
#define TACP_ERROR_FAILED_TO_PARSE_PROTOCOL -20
#define TACP_ERROR_ABANDON_MALFORMED_DATA -21
#define TACP_ERROR_WAITING_DATA -22
#define TACP_ERROR_FAILED_TO_MAKE_ANSWER -23
#define TACP_ERROR_FAILED_TO_TRANSLATE_ANSWER -24
#define TACP_ERROR_UNKNOWN_ANSWER_TINY_ID_TYPE -25

#define FLAG_DOC_BEGINNING_END 0xff
#define FLAG_UNIT_SPLITTER 0xfe
#define FLAG_ESCAPE 0xfd
#define FLAG_NOREPLACE 0xfc
#define FLAG_BYTES_TYPE 0xfb
#define FLAG_BYTE_TYPE 0xfa

#define MAX_SIZE_PROTOCOL_DATA 128
#define MAX_SIZE_ATTRIBUTE_DATA 16
#define MAX_SIZE_TEXT_DATA 32
#define MAX_SIZE_ATTRIBUTES 16

typedef struct LanAnwser {
	TinyId traceId;
	uint8_t errorNumber;
} LanAnswer;

typedef struct InboundProtocolRegistration {
	ProtocolDescription description;
	uint8_t (*processProtocol)(Protocol *);
	bool isQueryProtocol;
	struct InboundProtocolRegistration *next;
} InboundProtocolRegistration;

typedef struct OutboundProtocolRegistration {
	ProtocolDescription description;
	struct OutboundProtocolRegistration *next;
} OutboundProtocolRegistration;

enum TacpProtocolsMnemonic {
	TACP_PROTOCOL_INTRODUCTION = 100,
	TACP_PROTOCOL_INTRODUCTION_ATTRIBUTE_ADDRESS,
	TACP_PROTOCOL_ALLOCATION,
	TACP_PROTOCOL_ALLOCATION_ATTRIBUTE_GATEWAY_UPLINK_ADDRESS,
	TACP_PROTOCOL_ALLOCATION_ATTRIBUTE_GATEWAY_DOWNLINK_ADDRESS,
	TACP_PROTOCOL_ALLOCATION_ATTRIBUTE_ALLOCATED_ADDRESS,
	TACP_PROTOCOL_ALLOCATED,
	TACP_PROTOCOL_IS_CONFIGURED,
	TACP_PROTOCOL_IS_CONFIGURED_ATTRIBUTE_ADDRESS,
	TACP_PROTOCOL_NOT_CONFIGURED,
	TACP_PROTOCOL_CONFIGURED
};

ProtocolDescription createProtocolDescription(uint8_t mnemonic, ProtocolName name,
	ProtocolAttributeDescription attributes[], int attributeSize, bool acceptText);
Protocol createEmptyProtocol();
Protocol createEmptyProtocolByMenmonic(uint8_t menmonic);
int addIntAttribute(Protocol *protocol, uint8_t mnemonic, int iValue);
int addFloatAttribute(Protocol *protocol, uint8_t mnemonic, float fValue);
int addByteAttribute(Protocol *protocol, uint8_t mnemonic, uint8_t bValue);
int addBytesAttribute(Protocol *protocol, uint8_t mnemonic, uint8_t bytes[], int size);
int addStringAttribute(Protocol *protocol, uint8_t mnemonic, char string[]);
int setText(Protocol *protocol, char *text);

void registerInboundProtocol(ProtocolDescription description,
	uint8_t (*processProtocol)(Protocol *), bool isQueryProtocol);
bool unregisterInboundProtocol(uint8_t mnemomic);
void registerOutboundProtocol(ProtocolDescription description);
bool unregisterOutboundProtocol(uint8_t mnemomic);

bool isInboundProtocol(ProtocolData *pData, uint8_t mnemonic);
int parseProtocol(ProtocolData *pData, Protocol *protocol);
void releaseProtocol(Protocol *protocol);
void releaseProtocolData(ProtocolData *pData);
int translateProtocol(Protocol *protocol, ProtocolData *pData);
int translateAndRelease(Protocol *protocol, ProtocolData *pData);
int translateLanExecution(TinyId requestId, Protocol *action, ProtocolData *pData);

void sendAndRelease(uint8_t to[], ProtocolData *pData);

int getAttributesSize(Protocol *protocol);
bool getAttributeValueAsByte(Protocol *protocol, uint8_t mnemonic, uint8_t *value);
uint8_t *getAttributeValueAsBytes(Protocol *protocol, uint8_t mnemonic);
bool getAttributeValueAsInt(Protocol *protocol, uint8_t mnemonic, int *value);
char *getAttributeValueAsString(Protocol *protocol, uint8_t mnemonic);
bool getAttributeValueAsFloat(Protocol *protocol, uint8_t mnemonic, float *value);
char *getText(Protocol *protocol);

bool isLanAnswer(ProtocolData *pData);
LanAnswer createLanResonse(TinyId requestId);
LanAnswer createLanError(TinyId requestId, uint8_t errorNumber);
int parseLanAnswer(ProtocolData *pData, LanAnswer *lanAnswer);
int translateLanAnswer(LanAnswer *answer, ProtocolData *pData);

bool isLanExecution(ProtocolData *pData);
int parseLanExecution(ProtocolData *pData, TinyId requestId, Protocol *action);
int parseProtocol(ProtocolData *pData, Protocol *protocol);
int translateLanNotification(Protocol *event, uint8_t *data);
InboundProtocolRegistration *getInboundProtocolRegistrationByName(ProtocolName name);
bool getInboundProtocolNameByMnemonic(uint8_t mnemonic, ProtocolName *name);

#endif