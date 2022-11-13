#ifndef TACP_TACP_H
#define TACP_TACP_H

#include <stdint.h>
#include <stdbool.h>

#include "things_tiny_id.h"

enum DataType {
	TYPE_BOOL,
	TYPE_INT,
	TYPE_FLOAT,
	TYPE_STRING
};

struct ProtocolAttributeDescription {
	uint8_t mnemonic;
	uint8_t name;
	enum DataType dataType;
};

struct ProtocolDescription {
	uint8_t mnemomic;
	uint8_t name[2];
	uint8_t attributesSize;
	struct ProtocolAttributeDescription *attributes;
};

struct ProtocolRegistration {
	uint8_t name[2];
	struct ProtocolDescription description;
	void *(*assembleDomain)(struct Protocol *protocol);
	int (*processDomain)(void *action);
};

struct ProtocolAttribute {
	uint8_t mnemonic;
	uint8_t size;
	void *value;
};

struct Protocol {
	uint8_t attributesSize;
	struct ProtocolAttribute *attributes;
	char *text;
};

struct Allocation {
	int gatewayAddress;
	int gatewayChannel;
	int allocatedAddress;
	int allocatedFrequencyBand;
};

int (*assembleDomain)(struct Protocol *, void *);
int (*processDomain)(void *);

struct ProtocolDescription createProtocolDescription(uint8_t mnemonic,
	uint8_t name[2], struct ProtocolAttributeDescription attributes[]);
void registerInboundProtocol(struct ProtocolDescription protocolDescription,
	int (* assembleDomain)(struct Protocol *, void *),
	int (* processDomain)(void *));
bool unregisterInboundProtocol(uint8_t mnemomic);

uint8_t getProtocolMnemonic(uint8_t data[]);
bool isProtocol(uint8_t data[], uint8_t mnemonic);
bool isLanExecution(uint8_t data[]);
int parseProtocol(uint8_t data[], struct Protocol *protocol);
int parseLanExecution(uint8_t data[], struct Protocol *action, TinyId tinyId);
int translateProtocol(struct Protocol *protocol, uint8_t *data);
int translateLanExecutionResponse(TinyId requestId, uint8_t *data);
int translateLanExecutionError(TinyId requestId, int errorNumber, uint8_t *data);
int translateLanNotify(struct Protocol *event, uint8_t *data);

int processReceivedTacpData(uint8_t data[]);
bool getAttributeValueAsBool(struct Protocol *protocol, uint8_t mnemonic, bool *value);
bool getAttributeValueAsInt(struct Protocol *protocol, uint8_t mnemonic, int *value);
char *getAttributeValueAsString(struct Protocol *protocol, uint8_t mnemonic);
bool getAttributeValueAsFloat(struct Protocol *protocol, uint8_t mnemonic, float *value);
char *getText(struct Protocol *protocol);

#endif