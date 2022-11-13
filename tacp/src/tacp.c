#include "tacp.h"

#include <stdlib.h>

static int inboundProtocolsSize = 0;
static struct ProtocolRegistration *inboundProtocolRegistrations = NULL;

struct ProtocolDescription createProtocolDescription(uint8_t mnemonic,
		uint8_t name[2], struct ProtocolAttributeDescription attributes[]) {
	struct ProtocolDescription pd = {mnemonic, {name[0], name[1]}, NULL};
	pd.attributesSize = sizeof(attributes) / sizeof(struct ProtocolAttributeDescription);
	pd.attributes = (struct ProtocolAttributeDescription *)malloc(sizeof(struct ProtocolAttributeDescription) * pd.attributesSize);
	for(int i = 0; i < pd.attributesSize; i++) {
		struct ProtocolAttributeDescription *pad = pd.attributes + i;
		pad->mnemonic = attributes[i].mnemonic;
		pad->name = attributes[i].name;
		pad->dataType = attributes[i].dataType;
	}

	return pd;
}

void registerInboundProtocol(struct ProtocolDescription protocolDescription,
		void *(* assembleDomain)(struct Protocol *protocol),
			int (* processDomain)(void *action)) {
	struct ProtocolRegistration *olds = inboundProtocolRegistrations;

	inboundProtocolsSize++;
	inboundProtocolRegistrations = (struct ProtocolRegistration *)malloc(sizeof(struct ProtocolRegistration) * inboundProtocolsSize);

	if (olds) {
		for(int i = 0; i < inboundProtocolsSize - 1; i++) {
			*(inboundProtocolRegistrations + i) = *(olds + i);
		}

		free(olds);
	}

	struct ProtocolRegistration *newest = inboundProtocolRegistrations + (inboundProtocolsSize - 1);
	newest->name[0] = protocolDescription.name[0];
	newest->name[1] = protocolDescription.name[1];
	newest->description = protocolDescription;
	newest->assembleDomain = assembleDomain;
	newest->processDomain = processDomain;
}

bool unregisterInboundProtocol(uint8_t mnemomic) {
	inboundProtocolsSize--;
	bool removed = false;

	int i = 0;
	for(; i < inboundProtocolsSize - 1; i++) {
		struct ProtocolRegistration *current = inboundProtocolRegistrations + i;
		if (mnemomic == current->description.mnemomic) {
			free(current);
			removed = true;
			continue;
		}

		if (removed) {
			struct ProtocolRegistration *previous = inboundProtocolRegistrations + i -1;
			previous = current;
		}
	}

	return removed;
}

uint8_t getProtocolMnemonic(uint8_t data[]) {
	return -1;
}

bool isProtocol(uint8_t data[], uint8_t mnemonic) {
	return false;
}

bool isLanExecution(uint8_t data[]) {
	return false;
}

int parseProtocol(uint8_t data[], struct Protocol *protocol) {
	return -1;
}


int parseLanExecution(uint8_t data[], struct Protocol *action, TinyId tinyId) {
	return -1;
}

int translateProtocol(struct Protocol *protocol, uint8_t *data) {
	return -1;
}

int translateLanExecutionResponse(TinyId requestId, uint8_t *data) {
	return -1;
}

int translateLanExecutionError(TinyId requestId, int errorNumber, uint8_t *data) {
	return -1;
}

int translateLanNotify(struct Protocol *event, uint8_t *data) {
	return -1;
}

int processReceivedTacpData(uint8_t data[]) {
	return -1;
}

bool getAttributeValueAsBool(struct Protocol *protocol, uint8_t mnemonic, bool *value) {
	for (int i = 0; i < protocol->attributesSize; i++) {
		if ((protocol->attributes + i)->mnemonic == mnemonic) {
			value = (bool *)(protocol->attributes + i)->value;
			return true;
		}
	}

	return false;
}

bool getAttributeValueAsInt(struct Protocol *protocol, uint8_t mnemonic, int *value) {
	for(int i = 0; i < protocol->attributesSize; i++) {
		if((protocol->attributes + i)->mnemonic == mnemonic) {
			value = (int *)(protocol->attributes + i)->value;
			return true;
		}
	}

	return false;
}

char *getAttributeValueAsString(struct Protocol *protocol, uint8_t mnemonic) {
	for(int i = 0; i < protocol->attributesSize; i++) {
		if((protocol->attributes + i)->mnemonic == mnemonic) {
			struct ProtocolAttribute *attribute = protocol->attributes + i;
			return (char *)attribute->value;
		}
	}

	return NULL;
}

bool getAttributeValueAsFloat(struct Protocol *protocol, uint8_t mnemonic, float *value) {
	for(int i = 0; i < protocol->attributesSize; i++) {
		if((protocol->attributes + i)->mnemonic == mnemonic) {
			value = (float *)(protocol->attributes + i)->value;
			return true;
		}
	}

	return false;
}

char *getText(struct Protocol *protocol) {
	return NULL;
}
