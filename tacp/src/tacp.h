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

#define FLAG_DOC_BEGINNING_END 0xff
#define FLAG_UNIT_SPLITTER 0xfe
#define FLAG_ESCAPE 0xfd

#define SIZE_LAN_ANSWER SIZE_THINGS_TINY_ID + 3 + 1

typedef struct LanAnwser {
	TinyId requestId;
	ProtocolName protocolName;
	int8_t errorNumber;
} LanAnswer;

typedef struct InboundProtocolRegistration {
	ProtocolDescription *description;
	uint8_t (*assembleDomain)(Protocol *, void *);
	uint8_t (*processDomain)(void *);
} InboundProtocolRegistration;

void registerInboundProtocol(ProtocolDescription protocolDescription,
	uint8_t (*assembleDomain)(Protocol *, void *), uint8_t (*processDomain)(void *));
bool unregisterInboundProtocol(uint8_t mnemomic);

uint8_t getProtocolMnemonic(ProtocolData pData);
bool isProtocol(ProtocolData pData, uint8_t mnemonic);
int8_t parseProtocol(ProtocolData pData, Protocol *protocol);
void releaseProtocolResources(Protocol *protocol);
int translateProtocol(Protocol *protocol, uint8_t *data);

bool getAttributeValueAsInt(Protocol *protocol, uint8_t mnemonic, int *value);
char *getAttributeValueAsString(Protocol *protocol, uint8_t mnemonic);
bool getAttributeValueAsFloat(Protocol *protocol, uint8_t mnemonic, float *value);
char *getText(Protocol *protocol);

bool isLanAnswer(ProtocolData pData);
void createLanResponse(TinyId requestId, ProtocolName protocolName, uint8_t lanResponse[SIZE_LAN_ANSWER]);
void createLanError(TinyId requestId, ProtocolName protocolName, int8_t errorNumber, uint8_t lanError[SIZE_LAN_ANSWER]);
bool isLanExecution(ProtocolData pData);
int parseLanExecution(ProtocolData pData, Protocol *action, TinyId requestId);
int parseInboundProtocol(ProtocolData pData, Protocol *inbound);
int translateLanNotification(Protocol *event, uint8_t *data);

#endif