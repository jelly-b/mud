#ifndef TACP_PROTOCOLS_H
#define TACP_PROTOCOLS_H

#define TACP_NOT_VALID_PROTOCOL -1
#define TACP_UNKNOWN_PROTOCOL_NAME -2

#define DATA_SIZE(data) sizeof(data) / sizeof(uint8_t)
#define CREATE_PROTOCOL_DATA(data) {data, DATA_SIZE(data)}

#include <stdint.h>
#include <stdbool.h>

typedef enum DataType {
	TYPE_BOOL,
	TYPE_INT,
	TYPE_FLOAT,
	TYPE_STRING
} DataType;

typedef struct ProtocolAttributeDescription {
	uint8_t mnemonic;
	uint8_t name;
	DataType dataType;
} ProtocolAttributeDescription;

typedef struct ProtocolName {
	uint8_t namespace[2];
	uint8_t localName;
} ProtocolName;

typedef struct ProtocolDescription {
	uint8_t mnemomic;
	ProtocolName name;
	uint8_t attributesSize;
	ProtocolAttributeDescription *attributes;
} ProtocolDescription;

typedef struct ProtocolRegistration {
	ProtocolName name;
	ProtocolDescription description;
} ProtocolRegistration;

typedef struct ProtocolAttribute {
	uint8_t mnemonic;
	void *value;
} ProtocolAttribute;

typedef struct Protocol {
	uint8_t mnemonic;
	ProtocolAttribute *attributes;
	uint8_t attributesSize;
	char *text;
} Protocol;

typedef struct ProtocolData {
	uint8_t *data;
	int dataSize;
} ProtocolData;

ProtocolDescription createProtocolDescription(uint8_t mnemonic, ProtocolName name,
	ProtocolAttributeDescription attributes[], int attributeSize);
void registerInboundProtocol(ProtocolDescription protocolDescription);
bool unregisterInboundProtocol(uint8_t mnemomic);

uint8_t getProtocolMnemonic(ProtocolData pData);
bool isProtocol(ProtocolData pData, uint8_t mnemonic);
int parseProtocol(ProtocolData pData, Protocol *protocol);
void freeProtocolAttributeValues(Protocol *protocol);
int translateProtocol(Protocol *protocol, uint8_t *data);

bool getAttributeValueAsBool(Protocol *protocol, uint8_t mnemonic, bool *value);
bool getAttributeValueAsInt(Protocol *protocol, uint8_t mnemonic, int *value);
char *getAttributeValueAsString(Protocol *protocol, uint8_t mnemonic);
bool getAttributeValueAsFloat(Protocol *protocol, uint8_t mnemonic, float *value);
char *getText(Protocol *protocol);

#endif