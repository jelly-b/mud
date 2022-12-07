#include <stdlib.h>
#include <string.h>

#include "tacp.h"

#define MIN_PROTOCOL_DATA_SIZE 2 + 5

static int inboundProtocolsSize = 0;
static InboundProtocolRegistration *inboundProtocolRegistrations = NULL;

void copyInboundProtocolRegistration(InboundProtocolRegistration *dest,
			InboundProtocolRegistration *src) {
	dest->description->mnemomic = src->description->mnemomic;
	dest->description->name = src->description->name;
	dest->description->attributes = src->description->attributes;
	dest->description->attributesSize = src->description->attributesSize;
	dest->assembleDomain = src->assembleDomain;
	dest->processDomain = src->processDomain;
}

void registerInboundProtocol(ProtocolDescription protocolDescription,
		uint8_t (*assembleDomain)(Protocol *, void *), uint8_t (*processDomain)(void *)) {
	/*InboundProtocolRegistration *oldRegistrations = inboundProtocolRegistrations;

	inboundProtocolsSize++;
	inboundProtocolRegistrations = (InboundProtocolRegistration *)malloc(sizeof(InboundProtocolRegistration) * inboundProtocolsSize);

	if(oldRegistrations) {
		for(int i = 0; i < inboundProtocolsSize - 1; i++) {
			copyInboundProtocolRegistration(inboundProtocolRegistrations + i, oldRegistrations + i);
		}

		free(oldRegistrations);
	}*/

/*	InboundProtocolRegistration *newestRegistration = inboundProtocolRegistrations + (inboundProtocolsSize - 1);
	newestRegistration->description->mnemomic = protocolDescription.mnemomic;
	newestRegistration->description->name = protocolDescription.name;
	newestRegistration->description->attributes = protocolDescription.attributes;
	newestRegistration->description->attributesSize = protocolDescription.attributesSize;
	newestRegistration->assembleDomain = assembleDomain;
	newestRegistration->processDomain = processDomain;*/
}

bool unregisterInboundProtocol(uint8_t mnemomic) {
	if(inboundProtocolsSize == 0)
		return false;

	if (inboundProtocolsSize == 1 && inboundProtocolRegistrations->description->mnemomic == mnemomic) {
		free(inboundProtocolRegistrations);
		inboundProtocolRegistrations = NULL;
		inboundProtocolsSize = 0;

		return true;
	}

	if(inboundProtocolsSize == 1 && inboundProtocolRegistrations->description->mnemomic != mnemomic)
		return false;

	InboundProtocolRegistration *oldRegistrations = inboundProtocolRegistrations;
	inboundProtocolRegistrations = (InboundProtocolRegistration *)malloc(sizeof(InboundProtocolRegistration) * (inboundProtocolsSize - 1));

	bool removed = false;
	for(int i = 0; i < inboundProtocolsSize; i++) {
		InboundProtocolRegistration *oldRegistration = oldRegistrations + i;
		if(!removed && mnemomic == oldRegistration->description->mnemomic) {
			// free(oldRegistration->description->attributes);
			// free(oldRegistration->description);
			removed = true;
			continue;
		}

		/*InboundProtocolRegistration *newRegistration = NULL;
		if(removed) {
			newRegistration = inboundProtocolRegistrations + (i - 1);
		} else {
			newRegistration = inboundProtocolRegistrations + i;
		}
		copyInboundProtocolRegistration(newRegistration, oldRegistration);*/
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
	/*for(int i = 0; i < inboundProtocolsSize; i++) {
		InboundProtocolRegistration *registration = inboundProtocolRegistrations + i;
		if (registration->description->mnemomic == mnemonic) {
			*name = registration->description->name;
			return true;
		}
	}*/

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

ProtocolDescription *getProtocolDescriptionByName(ProtocolName name) {
	for(int i = 0; i < inboundProtocolsSize; i++) {
		InboundProtocolRegistration *registration = inboundProtocolRegistrations + i;
		if (registration->description->name.namespace[0] == name.namespace[0] &&
			registration->description->name.namespace[1] == name.namespace[1] &&
				registration->description->name.localName == name.localName) {
			return registration->description;
		}
	}

	return NULL;
}

ProtocolAttributeDescription *getAttributeDescriptionByName(ProtocolDescription *description,
		uint8_t attributeName) {
	for (int i = 0; i < description->attributesSize; i++) {
		if ((description->attributes + i)->name == attributeName)
			return description->attributes + i;
	}

	return NULL;
}

int findAttributeValueEnd(ProtocolData pData, int position, int *escapeNumber) {
	while (position <= (pData.dataSize - 1)) {
		uint8_t current = pData.data[position];
		if (current == FLAG_ESCAPE) {
			if ((position + 1) >= (pData.dataSize - 1))
				return -1;

			if(pData.data[position + 1] < 0xfa)
				return -1;

			(*escapeNumber)++;
			position += 2;
			continue;
		}

		if (pData.data[position] == FLAG_UNIT_SPLITTER ||
				pData.data[position] == FLAG_DOC_BEGINNING_END)
			return position;

		position++;
	}

	return -1;
}

int assembleProtocolAttributeValue(ProtocolData pData, ProtocolAttribute *attribute,
			DataType dataType, int attributeValueStartPosition,
				int attributeValueEndPosition, int escapeNumber) {
	int attributeValueSize = (attributeValueEndPosition - attributeValueStartPosition - escapeNumber);
	if(dataType == TYPE_BYTE && attributeValueSize != 1)
		return TACP_ERROR_MALFORMED_PROTOCOL_DATA;

	if(dataType == TYPE_BYTE) {
		if (escapeNumber > 1)
			return TACP_ERROR_MALFORMED_PROTOCOL_DATA;

		uint8_t *value = malloc(sizeof(uint8_t));
		if (escapeNumber == 0) {
			*value = pData.data[attributeValueStartPosition];
		} else {
			*value = pData.data[attributeValueStartPosition + 1];
		}
		attribute->value = value;
	} else {
		int attributeDataSize = attributeValueEndPosition - attributeValueStartPosition;
		unsigned char *attributeValue = malloc(sizeof(char) * (attributeValueSize + 1));
		bool escaped = false;
		int attributeValuePosition = 0;
		for(int i = 0; i < attributeDataSize; i++) {
			if(!escaped && pData.data[attributeValueStartPosition +  i] == FLAG_ESCAPE) {
				escaped = true;
				continue;
			}

			*(attributeValue + attributeValuePosition) = (char)pData.data[attributeValueStartPosition +  i];
			attributeValuePosition++;
			escaped =false;
		}
		*(attributeValue + attributeValueSize) = 0;

		if(dataType == TYPE_INT) {
			int *value = malloc(sizeof(int));
			*value = atoi(attributeValue);
			attribute->value = value;
		} else if(dataType == TYPE_FLOAT) {
			float *value = malloc(sizeof(float));
			*value = atof(attributeValue);
			attribute->value = value;
		} else {
			attribute->value = attributeValue;
		}

		if (dataType != TYPE_STRING)
			free(attributeValue);
	}

	return 0;
}

int doParseProtocol(ProtocolData pData, Protocol *protocol) {
	protocol->attributes = NULL;
	protocol->attributesSize = 0;
	protocol->text = NULL;

	if (!isValidProtocolData(pData))
		return TACP_ERROR_NOT_VALID_PROTOCOL;

	ProtocolName name = {
		{pData.data[1], pData.data[2]},
		pData.data[3]
	};

	ProtocolDescription *description = getProtocolDescriptionByName(name);
	if (!description)
		return TACP_ERROR_UNKNOWN_PROTOCOL_NAME;

	uint8_t childrenSize = pData.data[5] & 0x7f;
	if (childrenSize > 0)
		return TACP_ERROR_FEATURE_EMBEDDED_PROTOCOL_NOT_IMPLEMENTED;

	protocol->mnemonic = description->mnemomic;
	protocol->attributes = NULL;
	protocol->text = NULL;

	uint8_t attributeSize = pData.data[4];
	bool hasText = (pData.data[5] & 0x80) == 0x80;

	if (!description->acceptText && hasText)
		return TACP_ERROR_TEXT_NOT_ACCEPTED;

	if (attributeSize == 0 && !hasText) {
		if (pData.dataSize != 7)
			return TACP_ERROR_MALFORMED_PROTOCOL_DATA;

		return 0;
	}

	protocol->attributes = malloc(sizeof(ProtocolAttribute) * attributeSize);
	protocol->attributesSize = 0;

	int position = 5;
	for (int i = 0; i < attributeSize; i++) {
		if (position++ >= (pData.dataSize - 1))
			return TACP_ERROR_MALFORMED_PROTOCOL_DATA;

		uint8_t attributeName = pData.data[position];
		ProtocolAttributeDescription *attributeDescription =
			getAttributeDescriptionByName(description, attributeName);

		if (!attributeDescription)
			return TACP_ERROR_UNKNOWN_ATTRIBUTE_NAME;

		ProtocolAttribute *attribute = protocol->attributes + i;
		attribute->mnemonic = attributeDescription->mnemonic;
		attribute->value = NULL;

		position++;
		int escapeNumber = 0;
		int attributeValueEndPosition = findAttributeValueEnd(pData, position, &escapeNumber);
		if(attributeValueEndPosition <= 0)
			return TACP_ERROR_MALFORMED_PROTOCOL_DATA;

		int result = assembleProtocolAttributeValue(pData, attribute,
			attributeDescription->dataType, position, attributeValueEndPosition, escapeNumber);
		if (result != 0)
			return result;

		position = attributeValueEndPosition;
		protocol->attributesSize++;
	}

	if (!hasText && (position != (pData.dataSize - 1))) {
		return TACP_ERROR_MALFORMED_PROTOCOL_DATA;
	}

	if (!hasText)
		return 0;

	position++;
	int textDataSize = pData.dataSize - position - 1;
	int escapeNumber = 0;
	bool escaped = false;
	for (int i = 0; i < textDataSize; i++) {
		uint8_t current = pData.data[position + i];
		if (!escaped && current == FLAG_ESCAPE) {
			if (position + i + 1 >= (pData.dataSize - 1) ||
					pData.data[position + i +1] < 0xfa)
				return TACP_ERROR_MALFORMED_PROTOCOL_DATA;

			escaped = true;
			escapeNumber++;
		} else if (escaped) {
			escaped = false;
			continue;
		} else {
			// NOOP
		}
	}

	unsigned char *text = (unsigned char *)malloc(sizeof(char) * (textDataSize - escapeNumber + 1));
	int textPosition = 0;
	for(int i = 0; i < textDataSize; i++) {
		uint8_t current = pData.data[position + i];
		if(current == FLAG_ESCAPE) {
			continue;
		} else {
			*(text + textPosition) = current;
			textPosition++;
		}
	}
	*(text + textPosition) = 0;

	protocol->text = text;

	return 0;
}

int8_t parseProtocol(ProtocolData pData, Protocol *protocol) {
	int result = doParseProtocol(pData, protocol);
	if (result != 0)
		releaseProtocolResources(protocol);

	return result;
}

void releaseProtocolResources(Protocol *protocol) {
	for (int i = 0; i < protocol->attributesSize; i++) {
		ProtocolAttribute *attribute = protocol->attributes + i;
		if (attribute->value != NULL) {
			void *value = attribute->value;
			free(value);
		}
	}
	if (protocol->attributesSize != 0 || protocol->attributes)
		free(protocol->attributes);

	protocol->attributes = NULL;
	protocol->attributesSize = 0;

	if(protocol->text) {
		free(protocol->text);
		protocol->text = NULL;
	}
}

int translateProtocol(Protocol *protocol, uint8_t *data) {
	return -1;
}

bool getAttributeValueAsInt(Protocol *protocol, uint8_t mnemonic, int *value) {
	for(int i = 0; i < protocol->attributesSize; i++) {
		if((protocol->attributes + i)->mnemonic == mnemonic) {
			int *intValue = (int *)(protocol->attributes + i)->value;
			*value = *intValue;
			return true;
		}
	}

	return false;
}

char *getAttributeValueAsString(Protocol *protocol, uint8_t mnemonic) {
	for(int i = 0; i < protocol->attributesSize; i++) {
		if((protocol->attributes + i)->mnemonic == mnemonic) {
			ProtocolAttribute *attribute = protocol->attributes + i;
			return (char *)attribute->value;
		}
	}

	return NULL;
}

bool getAttributeValueAsFloat(Protocol *protocol, uint8_t mnemonic, float *value) {
	for(int i = 0; i < protocol->attributesSize; i++) {
		if((protocol->attributes + i)->mnemonic == mnemonic) {
			float *floatValue = (float *)(protocol->attributes + i);
			*value = *floatValue;
			return true;
		}
	}

	return false;
}

char *getText(Protocol *protocol) {
	return protocol->text;
}

bool isLanAnswer(ProtocolData pData) {
	return false;
}

void createLanResponse(TinyId requestId, ProtocolName protocolName,
		uint8_t lanResponse[SIZE_LAN_ANSWER]) {
}

void createLanError(TinyId requestId, ProtocolName protocolName,
		int8_t errorNumber, uint8_t lanError[SIZE_LAN_ANSWER]) {
}

bool isLanExecution(ProtocolData pData) {
	return false;
}

int parseLanExecution(ProtocolData pData, Protocol *action, TinyId tinyId) {
	return -1;
}

int translateLanExecutionResponse(TinyId requestId, uint8_t *data) {
	return -1;
}

int translateLanExecutionError(TinyId requestId, int8_t errorNumber, uint8_t *data) {
	return -1;
}

int translateLanNotify(struct Protocol *event, uint8_t *data) {
	return -1;
}
