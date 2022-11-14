#include <stdlib.h>

#include "protocols.h"

#define MIN_PROTOCOL_DATA_SIZE 2 + 5

static int inboundProtocolsSize = 0;
static ProtocolRegistration *inboundProtocolRegistrations = NULL;

struct ProtocolDescription createProtocolDescription(uint8_t mnemonic, ProtocolName name,
			ProtocolAttributeDescription attributes[], int attributeSize) {
	ProtocolDescription pd = {
		mnemonic,
		{
			{name.namespace[0], name.namespace[1]},
			name.localName
		},
		NULL
	};

	pd.attributesSize = attributeSize;
	pd.attributes = (ProtocolAttributeDescription *)malloc(sizeof(struct ProtocolAttributeDescription) * pd.attributesSize);
	for(int i = 0; i < pd.attributesSize; i++) {
		struct ProtocolAttributeDescription *pad = pd.attributes + i;
		pad->mnemonic = attributes[i].mnemonic;
		pad->name = attributes[i].name;
		pad->dataType = attributes[i].dataType;
	}

	return pd;
}

void copyProtocolRegistration(ProtocolRegistration *dest, ProtocolRegistration *src) {
	dest->name.namespace[0] = src->name.namespace[0];
	dest->name.namespace[1] = src->name.namespace[1];
	dest->name.localName = src->name.localName;

	dest->description.mnemomic = src->description.mnemomic;
	dest->description.name = src->description.name;
	dest->description.attributes = src->description.attributes;
	dest->description.attributesSize = src->description.attributesSize;
}

void registerInboundProtocol(ProtocolDescription protocolDescription) {
	ProtocolRegistration *oldRegistrations = inboundProtocolRegistrations;

	inboundProtocolsSize++;
	inboundProtocolRegistrations = (ProtocolRegistration *)malloc(sizeof(ProtocolRegistration) * inboundProtocolsSize);

	if(oldRegistrations) {
		for(int i = 0; i < inboundProtocolsSize - 1; i++) {
			copyProtocolRegistration(inboundProtocolRegistrations + i, oldRegistrations + i);
		}

		free(oldRegistrations);
	}

	ProtocolRegistration *newestRegistration = inboundProtocolRegistrations + (inboundProtocolsSize - 1);
	newestRegistration->name = protocolDescription.name;
	newestRegistration->description = protocolDescription;
}

bool unregisterInboundProtocol(uint8_t mnemomic) {
	if(inboundProtocolsSize == 0)
		return false;

	ProtocolRegistration *oldRegistrations = inboundProtocolRegistrations;
	inboundProtocolRegistrations = (ProtocolRegistration *)malloc(sizeof(ProtocolRegistration) * (inboundProtocolsSize - 1));

	bool removed = false;
	for(int i = 0; i < inboundProtocolsSize; i++) {
		struct ProtocolRegistration *oldRegistration = oldRegistrations + i;
		if(mnemomic == oldRegistration->description.mnemomic) {
			free(oldRegistration->description.attributes);
			removed = true;
			continue;
		}

		ProtocolRegistration *newRegistration = NULL;
		if(removed) {
			newRegistration = inboundProtocolRegistrations + (i - 1);
		} else {
			newRegistration = inboundProtocolRegistrations + i;
		}
		copyProtocolRegistration(newRegistration, oldRegistration);
	}

	free(oldRegistrations);
	inboundProtocolsSize--;

	return removed;
}

uint8_t getProtocolMnemonic(ProtocolData pData) {
	return -1;
}

bool isValidProtocolData(ProtocolData pData) {
	if(pData.dataSize < MIN_PROTOCOL_DATA_SIZE)
		return false;

	if(pData.data[0] != 0xff || pData.data[pData.dataSize - 1] != 0xff)
		return false;

	return true;
}

bool getProtocolNameByMnemonic(uint8_t mnemonic, ProtocolName *name) {
	for(int i = 0; i < inboundProtocolsSize; i++) {
		ProtocolRegistration *registration = inboundProtocolRegistrations + i;
		if (registration->description.mnemomic == mnemonic) {
			*name = registration->name;
			return true;
		}
	}

	return false;
}

bool isProtocol(ProtocolData pData, uint8_t mnemonic) {
	if (!isValidProtocolData(pData))
		return false;

	ProtocolName name;
	if (!getProtocolNameByMnemonic(mnemonic, &name))
		return false;

	return pData.data[1] == name.namespace[0] &&
		pData.data[2] == name.namespace[1] &&
		pData.data[3] == name.localName;
}

bool getProtocolDescriptionByName(ProtocolName name, ProtocolDescription *description) {
	for(int i = 0; i < inboundProtocolsSize; i++) {
		ProtocolRegistration *registration = inboundProtocolRegistrations + i;
		if (registration->name.namespace[0] == name.namespace[0] &&
				registration->name.namespace[1] == name.namespace[1] &&
				registration->name.localName == name.localName) {
			description->mnemomic = registration->description.mnemomic;
			description->name = registration->description.name;
			description->attributes = registration->description.attributes;
			description->attributesSize = registration->description.attributesSize;

			return true;
		}
	}

	return false;
}

int parseProtocol(ProtocolData pData, Protocol *protocol) {
	if (!isValidProtocolData(pData))
		return TACP_NOT_VALID_PROTOCOL;

	ProtocolName name = {
		{pData.data[1], pData.data[2]},
		pData.data[3]
	};

	ProtocolDescription description;
	if (!getProtocolDescriptionByName(name, &description))
		return TACP_UNKNOWN_PROTOCOL_NAME;



	return -1;
}

void freeProtocolAttributeValues(Protocol *protocol) {

}

int translateProtocol(Protocol *protocol, uint8_t *data) {
	return -1;
}

bool getAttributeValueAsBool(Protocol *protocol, uint8_t mnemonic, bool *value) {
	for(int i = 0; i < protocol->attributesSize; i++) {
		if((protocol->attributes + i)->mnemonic == mnemonic) {
			value = (bool *)(protocol->attributes + i)->value;
			return true;
		}
	}

	return false;
}

bool getAttributeValueAsInt(Protocol *protocol, uint8_t mnemonic, int *value) {
	for(int i = 0; i < protocol->attributesSize; i++) {
		if((protocol->attributes + i)->mnemonic == mnemonic) {
			value = (int *)(protocol->attributes + i)->value;
			return true;
		}
	}

	return false;
}

char *getAttributeValueAsString(Protocol *protocol, uint8_t mnemonic) {
	for(int i = 0; i < protocol->attributesSize; i++) {
		if((protocol->attributes + i)->mnemonic == mnemonic) {
			struct ProtocolAttribute *attribute = protocol->attributes + i;
			return (char *)attribute->value;
		}
	}

	return NULL;
}

bool getAttributeValueAsFloat(Protocol *protocol, uint8_t mnemonic, float *value) {
	for(int i = 0; i < protocol->attributesSize; i++) {
		if((protocol->attributes + i)->mnemonic == mnemonic) {
			value = (float *)(protocol->attributes + i)->value;
			return true;
		}
	}

	return false;
}

char *getText(Protocol *protocol) {
	return NULL;
}
