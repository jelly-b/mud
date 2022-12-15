#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "tacp.h"

#define MIN_PROTOCOL_DATA_SIZE 2 + 5

static uint8_t inboundProtocolsSize = 0;
static InboundProtocolRegistration *inboundProtocolRegistrations = NULL;

static uint8_t outboundProtocolsSize = 0;
static ProtocolDescription *outboundProtocolDescriptions = NULL;

void copyProtocolDescription(ProtocolDescription *dest, ProtocolDescription *src) {
	dest->mnemomic = src->mnemomic;
	dest->name = src->name;
	dest->attributes = src->attributes;
	dest->attributesSize = src->attributesSize;
}

void copyInboundProtocolRegistration(InboundProtocolRegistration *dest,
			InboundProtocolRegistration *src) {
	dest->description->mnemomic = src->description->mnemomic;
	dest->description->name = src->description->name;
	dest->description->attributes = src->description->attributes;
	dest->description->attributesSize = src->description->attributesSize;
	dest->assembleDomain = src->assembleDomain;
	dest->processDomain = src->processDomain;
}

int createProtocolBytesAttribute(ProtocolAttribute *attribute, uint8_t mnemonic,
			uint8_t bytes[], int size) {
	if(size > MAX_ATTRIBUTE_DATA_SIZE)
		return TACP_ERROR_ATTRIBUTE_DATA_TOO_LARGE;

	attribute->mnemonic = mnemonic;
	attribute->dataType = TYPE_BYTES;
	attribute->value.bsValue = (uint8_t *)malloc((size + 1) * sizeof(uint8_t));

	if (!attribute->value.bsValue)
		return TACP_ERROR_OUT_OF_MEMEORY;

	*(attribute->value.bsValue) = (uint8_t)size;
	memcpy(attribute->value.bsValue + 1, bytes, size);

	return 0;
}

int createProtocolStringAttribute(ProtocolAttribute *attribute, uint8_t mnemonic, char string[]) {
	int length = strlen(string);
	if(length > MAX_ATTRIBUTE_DATA_SIZE)
		return TACP_ERROR_ATTRIBUTE_DATA_TOO_LARGE;

	attribute->mnemonic = mnemonic;
	attribute->dataType = TYPE_STRING;
	attribute->value.sValue = malloc((strlen(string) + 1) * sizeof(char));

	if(!attribute->value.sValue)
		return TACP_ERROR_OUT_OF_MEMEORY;

	strcpy(attribute->value.sValue, string);

	return 0;
}

void createProtocolByteAttribute(ProtocolAttribute *attribute, uint8_t mnemonic, uint8_t bValue) {
	attribute->mnemonic = mnemonic;
	attribute->dataType = TYPE_BYTE;
	attribute->value.bValue = bValue;
}

void createProtocolIntAttribute(ProtocolAttribute *attribute, uint8_t mnemonic, int iValue) {
	attribute->mnemonic = mnemonic;
	attribute->dataType = TYPE_INT;
	attribute->value.iValue = iValue;
}

void createProtocolFloatAttribute(ProtocolAttribute *attribute, uint8_t mnemonic, float fValue) {
	attribute->mnemonic = mnemonic;
	attribute->dataType = TYPE_FLOAT;
	attribute->value.fValue = fValue;
}

int createProtocolText(Protocol *protocol, char *text) {
	protocol->text = (char *)malloc(strlen(text) + 1);
	if (!protocol->text)
		return TACP_ERROR_OUT_OF_MEMEORY;

	strcpy(protocol->text, text);
}

ProtocolDescription createProtocolDescription(uint8_t mnemonic, ProtocolName name,
	ProtocolAttributeDescription attributes[], int attributeSize, bool acceptText) {
	ProtocolDescription pd ={
		mnemonic,
		{
			{name.namespace[0], name.namespace[1]},
			name.localName
		},
		NULL,
		attributeSize,
		acceptText
	};

	pd.attributes = (ProtocolAttributeDescription *)malloc(sizeof(ProtocolAttributeDescription) * pd.attributesSize);
	for(int i = 0; i < pd.attributesSize; i++) {
		ProtocolAttributeDescription *pad = pd.attributes + i;
		pad->mnemonic = attributes[i].mnemonic;
		pad->name = attributes[i].name;
		pad->dataType = attributes[i].dataType;
	}

	return pd;
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

void registerOutboundProtocol(ProtocolDescription protocolDescription) {
	ProtocolDescription *oldDescriptions = outboundProtocolDescriptions;

	outboundProtocolsSize++;
	outboundProtocolDescriptions = (ProtocolDescription *)malloc(sizeof(ProtocolDescription) * outboundProtocolsSize);

	if(oldDescriptions) {
		for(int i = 0; i < outboundProtocolsSize - 1; i++) {
			copyProtocolDescription(outboundProtocolDescriptions + i, oldDescriptions + i);
		}

		free(oldDescriptions);
	}

	ProtocolDescription *newestDescription = outboundProtocolDescriptions + (outboundProtocolsSize - 1);
	if (newestDescription) {
		newestDescription->mnemomic = protocolDescription.mnemomic;
		newestDescription->name = protocolDescription.name;
		newestDescription->attributes = protocolDescription.attributes;
		newestDescription->attributesSize = protocolDescription.attributesSize;
	}
}

bool unregisterOutboundProtocol(uint8_t mnemomic) {
	if(outboundProtocolsSize == 0)
		return false;

	if(outboundProtocolsSize == 1 && outboundProtocolDescriptions->mnemomic == mnemomic) {
		free(outboundProtocolDescriptions);
		outboundProtocolDescriptions = NULL;
		outboundProtocolsSize = 0;

		return true;
	}

	if(outboundProtocolsSize == 1 && outboundProtocolDescriptions->mnemomic != mnemomic)
		return false;

	ProtocolDescription *oldDescriptions = outboundProtocolDescriptions;
	outboundProtocolDescriptions = (ProtocolDescription *)malloc(sizeof(ProtocolDescription) * (outboundProtocolsSize - 1));

	bool removed = false;
	for(int i = 0; i < outboundProtocolsSize; i++) {
		ProtocolDescription *oldDescription = oldDescriptions + i;
		if(!removed && mnemomic == oldDescription->mnemomic) {
			free(oldDescription->attributes);
			removed = true;
			continue;
		}

		ProtocolDescription *newDescription = NULL;
		if(removed) {
			newDescription = outboundProtocolDescriptions + (i - 1);
		} else {
			newDescription = outboundProtocolDescriptions + i;
		}
		copyProtocolDescription(newDescription, oldDescription);
	}

	free(oldDescriptions);
	inboundProtocolsSize--;

	return removed;
}

int escape(uint8_t data[], int size, ProtocolData *pData) {
	int position = 0;
	for(int i = 0; i < size; i++) {
		if (position > MAX_ATTRIBUTE_DATA_SIZE)
			return TACP_ERROR_ATTRIBUTE_DATA_TOO_LARGE;

		if(data[i] == FLAG_DOC_BEGINNING_END ||
				data[i] == FLAG_UNIT_SPLITTER ||
				data[i] == FLAG_NOREPLACE ||
				data[i] == FLAG_ESCAPE ||
				data[i] == FLAG_BYTES_TYPE ||
				data[i] == FLAG_BYTE_TYPE) {
			pData->data[position] = FLAG_ESCAPE;
			position++;
		}

		pData->data[position] = data[i];
		position++;
	}

	if(position == 1) {
		pData->data[1] = pData->data[0];
		pData->data[0] = FLAG_NOREPLACE;
		pData->dataSize = 2;
	} else if(position == 2) {
		pData->data[2] = pData->data[1];
		pData->data[1] = pData->data[0];
		pData->data[0] = FLAG_NOREPLACE;
		pData->dataSize = 3;
	} else {
		pData->dataSize = position;
	}

	return 0;
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

bool getInboundProtocolNameByMnemonic(uint8_t mnemonic, ProtocolName *name) {
	for(int i = 0; i < inboundProtocolsSize; i++) {
		InboundProtocolRegistration *registration = inboundProtocolRegistrations + i;
		if (registration->description->mnemomic == mnemonic) {
			*name = registration->description->name;
			return true;
		}
	}

	return false;
}

bool isInboundProtocol(ProtocolData pData, uint8_t mnemonic) {
	if (!isValidProtocolData(pData))
		return false;

	ProtocolName name;
	if (!getInboundProtocolNameByMnemonic(mnemonic, &name))
		return false;

	return pData.data[1] == name.namespace[0] &&
		pData.data[2] == name.namespace[1] &&
		pData.data[3] == name.localName;
}

ProtocolDescription *getOutboundProtocolDescriptionByMnemonic(uint8_t mnemonic) {
	for(int i = 0; i < outboundProtocolsSize; i++) {
		ProtocolDescription *description = outboundProtocolDescriptions + i;
		if(description->mnemomic == mnemonic) {
			return description;
		}
	}

	return NULL;
}

ProtocolDescription *getInboundProtocolDescriptionByName(ProtocolName name) {
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

ProtocolAttributeDescription *getAttributeDescriptionByMnemonic(ProtocolDescription *description,
			uint8_t mnemonic) {
	for(int i = 0; i < description->attributesSize; i++) {
		if((description->attributes + i)->mnemonic == mnemonic)
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
		if (!value)
			return TACP_ERROR_OUT_OF_MEMEORY;

		if (escapeNumber == 0) {
			*value = pData.data[attributeValueStartPosition];
		} else {
			*value = pData.data[attributeValueStartPosition + 1];
		}
		attribute->value.bValue = *value;
	} else {
		int attributeDataSize = attributeValueEndPosition - attributeValueStartPosition;
		unsigned char *attributeValue = malloc(sizeof(char) * (attributeValueSize + 1));
		if (!attributeValue)
			return TACP_ERROR_OUT_OF_MEMEORY;

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
			attribute->value.iValue = atoi(attributeValue);
		} else if(dataType == TYPE_FLOAT) {
			attribute->value.fValue = atof(attributeValue);
		} else if (dataType == TYPE_BYTE) {
		} else if (dataType == TYPE_BYTES) {
		} else {
			attribute->value.sValue = attributeValue;
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

	ProtocolDescription *description = getInboundProtocolDescriptionByName(name);
	if (!description)
		return TACP_ERROR_UNKNOWN_PROTOCOL_NAME;

	uint8_t childrenSize = pData.data[5] & 0x7f;
	if (childrenSize > 0)
		return TACP_ERROR_FEATURE_EMBEDDED_PROTOCOL_NOT_IMPLEMENTED;

	protocol->mnemonic = description->mnemomic;
	protocol->attributes = NULL;
	protocol->text = NULL;

	uint8_t attributeSize = pData.data[4];
	bool hasText = ((pData.data[5] & 0x80) == 0x80);

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
	if (protocol->attributesSize != 0 || protocol->attributes) {
		for (int i = 0; i < protocol->attributesSize; i++) {
			if ((protocol->attributes + i)->dataType == TYPE_BYTES &&
					(protocol->attributes + i)->value.bsValue != NULL) {
				free((protocol->attributes + i)->value.bsValue);
			} else if ((protocol->attributes + i)->dataType == TYPE_STRING &&
					(protocol->attributes + i)->value.sValue != NULL) {
				free((protocol->attributes + i)->value.sValue);
			} else {
				// NOOP
			}
		}

		free(protocol->attributes);
	}

	protocol->attributes = NULL;
	protocol->attributesSize = 0;

	if(protocol->text) {
		free(protocol->text);
		protocol->text = NULL;
	}
}

void releaseProtocolData(ProtocolData *pData) {
	if (pData->dataSize != 0 && pData->data != NULL) {
		free(pData->data);
		pData->data = NULL;
		pData->dataSize = 0;
	}
}

int translateProtocol(Protocol *protocol, ProtocolData *pData) {
	if (protocol->attributesSize > MAX_ATTRIBUTES_SIZE)
		return TACP_ERROR_TOO_MANY_ATTRIBUTES;

	ProtocolDescription *description = getOutboundProtocolDescriptionByMnemonic(protocol->mnemonic);
	if (!description)
		return TACP_ERROR_UNKNOWN_PROTOCOL_MNEMONIC;

	if (!description->acceptText && protocol->text)
		return TACP_ERROR_TEXT_NOT_ACCEPTED;

	uint8_t buff[MAX_PROTOCOL_DATA_SIZE];
	buff[0] = 0xff;
	buff[1] = description->name.namespace[0];
	buff[2] = description->name.namespace[1];
	buff[3] = description->name.localName;
	buff[4] = protocol->attributesSize;
	buff[5] = protocol->text ? 0x80 : 0x00;

	ProtocolData attributeData;
	if(protocol->attributesSize != 0) {
		attributeData.data = malloc(sizeof(uint8_t) * MAX_ATTRIBUTE_DATA_SIZE);
		if(!attributeData.data)
			return TACP_ERROR_OUT_OF_MEMEORY;
	}

	int position = 6;
	for (int i = 0; i < protocol->attributesSize; i++) {
		ProtocolAttribute *attribute = protocol->attributes + i;
		ProtocolAttributeDescription *attributeDescription = getAttributeDescriptionByMnemonic(description, attribute->mnemonic);
		if (!attributeDescription) {
			free(attributeData.data);
			return TACP_ERROR_UNKNOWN_PROTOCOL_ATTRIBUTE_MNEMONIC;
		}

		if (position >= MAX_PROTOCOL_DATA_SIZE - 1) {
			free(attributeData.data);
			return TACP_ERROR_PROTOCOL_DATA_TOO_LARGE;
		}

		buff[position] = attributeDescription->name;
		position++;

		if(position >= MAX_PROTOCOL_DATA_SIZE - 1) {
			free(attributeData.data);
			return TACP_ERROR_PROTOCOL_DATA_TOO_LARGE;
		}

		int escapeResult = 0;
		if (attributeDescription->dataType == TYPE_BYTE) {
			uint8_t aData[1] = {attribute->value.bValue};
			escapeResult = escape(aData, 1, &attributeData);
		} else if (attributeDescription->dataType == TYPE_INT) {
			uint8_t aData[32];
			sprintf(aData, "%d", attribute->value.iValue);
			escapeResult = escape(aData, strlen(aData), &attributeData);
		} else if (attributeDescription->dataType == TYPE_FLOAT) {
			uint8_t aData[32];
			sprintf(aData, "%f", attribute->value.fValue);
			escapeResult = escape(aData, strlen(aData), &attributeData);
		} else if (attributeDescription->dataType == TYPE_BYTES) {
			int bsSize = attribute->value.bsValue[0];
			escapeResult = escape(attribute->value.bsValue + 1, bsSize, &attributeData);
		} else { // attributeDescription->dataType == TYPE_STRING
			escapeResult = escape(attribute->value.sValue, strlen(attribute->value.sValue), &attributeData);
		}

		if (escapeResult != 0) {
			free(attributeData.data);
			return escapeResult;
		} else if (attributeDescription->dataType == TYPE_BYTE) {
			buff[position] = FLAG_BYTE_TYPE;
			position++;
		} else if (attributeDescription->dataType == TYPE_BYTES) {
			buff[position] = FLAG_BYTES_TYPE;
			position++;
		} else {
			// NOOP
		}

		if(position >= MAX_PROTOCOL_DATA_SIZE - 1) {
			free(attributeData.data);
			return TACP_ERROR_PROTOCOL_DATA_TOO_LARGE;
		}

		if(position + attributeData.dataSize >= MAX_PROTOCOL_DATA_SIZE - 1) {
			free(attributeData.data);
			return TACP_ERROR_PROTOCOL_DATA_TOO_LARGE;
		}

		memcpy(buff + position, attributeData.data, attributeData.dataSize);
		position += attributeData.dataSize;

		buff[position] = FLAG_UNIT_SPLITTER;
		position++;
		
		if(position >= MAX_PROTOCOL_DATA_SIZE - 1) {
			free(attributeData.data);
			return TACP_ERROR_PROTOCOL_DATA_TOO_LARGE;
		}
	}

	if (attributeData.dataSize != 0 && attributeData.data != NULL)
		free(attributeData.data);

	if (protocol->text) {
		int textSize = strlen(protocol->text);
		if (position + textSize >= MAX_PROTOCOL_DATA_SIZE - 1)
			return TACP_ERROR_PROTOCOL_DATA_TOO_LARGE;

		memcpy(buff + position, protocol->text, textSize);
		position += textSize;
	}

	if (buff[position - 1] == FLAG_UNIT_SPLITTER) {
		buff[position - 1] = FLAG_DOC_BEGINNING_END;
	} else {
		if (position >= MAX_PROTOCOL_DATA_SIZE - 1)
			return TACP_ERROR_PROTOCOL_DATA_TOO_LARGE;

		buff[position] = FLAG_DOC_BEGINNING_END;
	}

	pData->data = NULL;
	pData->dataSize = 0;

	int dataSize = position + 1;
	pData->data = malloc(dataSize * sizeof(uint8_t));
	if (!pData->data)
		return TACP_ERROR_OUT_OF_MEMEORY;

	memcpy(pData->data, buff, dataSize);
	pData->dataSize = dataSize;

	return 0;
}

bool getAttributeValueAsInt(Protocol *protocol, uint8_t mnemonic, int *value) {
	for(int i = 0; i < protocol->attributesSize; i++) {
		if((protocol->attributes + i)->mnemonic == mnemonic) {
			*value = (protocol->attributes + i)->value.iValue;
			return true;
		}
	}

	return false;
}

char *getAttributeValueAsString(Protocol *protocol, uint8_t mnemonic) {
	for(int i = 0; i < protocol->attributesSize; i++) {
		if((protocol->attributes + i)->mnemonic == mnemonic) {
			ProtocolAttribute *attribute = protocol->attributes + i;
			return attribute->value.sValue;
		}
	}

	return NULL;
}

bool getAttributeValueAsFloat(Protocol *protocol, uint8_t mnemonic, float *value) {
	for(int i = 0; i < protocol->attributesSize; i++) {
		if((protocol->attributes + i)->mnemonic == mnemonic) {
			*value = (protocol->attributes + i)->value.fValue;
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
