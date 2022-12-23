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
#define TACP_ERROR_UNKNOWN_PROTOCOL_MNEMONIC -9
#define TACP_ERROR_TOO_MANY_ATTRIBUTES -10
#define TACP_ERROR_UNKNOWN_PROTOCOL_ATTRIBUTE_MNEMONIC -11
#define TACP_ERROR_PROTOCOL_DATA_TOO_LARGE -12
#define TACP_ERROR_CHANGE_CLOSED -13

#define FLAG_DOC_BEGINNING_END 0xff
#define FLAG_UNIT_SPLITTER 0xfe
#define FLAG_ESCAPE 0xfd
#define FLAG_NOREPLACE 0xfc
#define FLAG_BYTES_TYPE 0xfb
#define FLAG_BYTE_TYPE 0xfa

#define SIZE_LAN_ANSWER SIZE_THINGS_TINY_ID + sizeof(uint8_t)
#define MAX_PROTOCOL_DATA_SIZE 128
#define MAX_ATTRIBUTE_DATA_SIZE 32
#define MAX_ATTRIBUTES_SIZE 16

typedef struct LanAnwser {
	TinyId requestId;
	int8_t errorNumber;
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
	TACP_PROTOCOL_INTRODUCTION = 200,
	TACP_PROTOCOL_INTRODUCTION_ATTRIBUTE_ADDRESS
};

void addIntAttribute(Protocol *protocol, uint8_t mnemonic, int iValue);
void addFloatAttribute(Protocol *protocol, uint8_t mnemonic, float fValue);
void addByteAttribute(Protocol *protocol, uint8_t mnemonic, uint8_t bValue);
int addBytesAttribute(Protocol *protocol, uint8_t mnemonic, uint8_t bytes[], int size);
int addStringAttribute(Protocol *protocol, uint8_t mnemonic, char string[]);
int setText(Protocol *protocol, char *text);

void registerInboundProtocol(ProtocolDescription protocolDescription,
		uint8_t (*processProtocol)(Protocol *), bool isQueryProtocol);
bool unregisterInboundProtocol(uint8_t mnemomic);
void registerOutboundProtocol(ProtocolDescription protocolDescription);
bool unregisterOutboundProtocol(uint8_t mnemomic);

uint8_t getProtocolMnemonic(ProtocolData *pData);
bool isInboundProtocol(ProtocolData *pData, uint8_t mnemonic);
int8_t parseProtocol(ProtocolData *pData, Protocol *protocol);
void releaseProtocolResources(Protocol *protocol);
void releaseProtocolData(ProtocolData *pData);
int translateProtocol(Protocol *protocol, ProtocolData *pData);
int translateAndRelease(Protocol *protocol, ProtocolData *pData);

void sendAndRelease(uint8_t to[], ProtocolData *pData);

int getAttributesSize(Protocol *protocol);
bool getAttributeValueAsByte(Protocol *protocol, uint8_t mnemonic, uint8_t *value);
uint8_t *getAttributeValueAsBytes(Protocol *protocol, uint8_t mnemonic);
bool getAttributeValueAsInt(Protocol *protocol, uint8_t mnemonic, int *value);
char *getAttributeValueAsString(Protocol *protocol, uint8_t mnemonic);
bool getAttributeValueAsFloat(Protocol *protocol, uint8_t mnemonic, float *value);
char *getText(Protocol *protocol);

bool isLanAnswer(ProtocolData *pData);
void createLanResponse(TinyId requestId, ProtocolName protocolName, uint8_t lanResponse[SIZE_LAN_ANSWER]);
void createLanError(TinyId requestId, ProtocolName protocolName, int8_t errorNumber, uint8_t lanError[SIZE_LAN_ANSWER]);
bool isLanExecution(ProtocolData *pData);
int parseLanExecution(ProtocolData *pData, Protocol *action, TinyId requestId);
int parseInboundProtocol(ProtocolData *pData, Protocol *protocol);
int translateLanNotification(Protocol *event, uint8_t *data);

#endif