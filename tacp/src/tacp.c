#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "tacp.h"

#define MIN_PROTOCOL_DATA_SIZE 2 + 5

static InboundProtocolRegistration *inboundProtocolRegistrations = NULL;

static OutboundProtocolRegistration *outboundProtocolRegistrations = NULL;

void copyProtocolDescription(ProtocolDescription *dest, ProtocolDescription *src) {
	dest->mnemomic = src->mnemomic;
	dest->name = src->name;
	dest->attributes = src->attributes;
	dest->attributesSize = src->attributesSize;
}

void copyInboundProtocolRegistration(InboundProtocolRegistration *dest,
			InboundProtocolRegistration *src) {
	dest->description.mnemomic = src->description.mnemomic;
	dest->description.name = src->description.name;
	dest->description.attributes = src->description.attributes;
	dest->description.attributesSize = src->description.attributesSize;
	dest->processProtocol = src->processProtocol;
	dest->isQueryProtocol = src->isQueryProtocol;
}

void addAttributeToProtocol(Protocol *protocol, ProtocolAttribute *attribute) {
	attribute->previous = NULL;
	attribute->next = NULL;

	if (!protocol->attributes) {
		protocol->attributes = attribute;
	} else {
		ProtocolAttribute *lastAttribute = protocol->attributes;
		while (lastAttribute) {
			if (lastAttribute->next)
				lastAttribute = lastAttribute->next;
			else
				break;
		}

		lastAttribute->next = attribute;
		attribute->previous = lastAttribute;
	}
}

int addBytesAttribute(Protocol *protocol, uint8_t mnemonic, uint8_t bytes[], int size) {
	if (protocol->text)
		return TACP_ERROR_CHANGE_CLOSED;

	if (size > MAX_ATTRIBUTE_DATA_SIZE)
		return TACP_ERROR_ATTRIBUTE_DATA_TOO_LARGE;

	ProtocolAttribute *attribute = malloc(sizeof(ProtocolAttribute));
	attribute->mnemonic = mnemonic;
	attribute->dataType = TYPE_BYTES;
	attribute->value.bsValue = (uint8_t *)malloc((size + 1) * sizeof(uint8_t));

	if (!attribute->value.bsValue)
		return TACP_ERROR_OUT_OF_MEMEORY;

	*(attribute->value.bsValue) = (uint8_t)size;
	memcpy(attribute->value.bsValue + 1, bytes, size);

	addAttributeToProtocol(protocol, attribute);
	return 0;
}

int addStringAttribute(Protocol *protocol, uint8_t mnemonic, char string[]) {
	if(protocol->text)
		return TACP_ERROR_CHANGE_CLOSED;

	int length = strlen(string);
	if (length > MAX_ATTRIBUTE_DATA_SIZE)
		return TACP_ERROR_ATTRIBUTE_DATA_TOO_LARGE;

	ProtocolAttribute *attribute = malloc(sizeof(ProtocolAttribute));
	attribute->mnemonic = mnemonic;
	attribute->dataType = TYPE_STRING;
	attribute->value.sValue = malloc((strlen(string) + 1) * sizeof(char));

	if (!attribute->value.sValue)
		return TACP_ERROR_OUT_OF_MEMEORY;

	strcpy(attribute->value.sValue, string);

	addAttributeToProtocol(protocol, attribute);

	return 0;
}

void addByteAttribute(Protocol *protocol, uint8_t mnemonic, uint8_t bValue) {
	if(protocol->text)
		return TACP_ERROR_CHANGE_CLOSED;

	ProtocolAttribute *attribute = malloc(sizeof(ProtocolAttribute));

	attribute->mnemonic = mnemonic;
	attribute->dataType = TYPE_BYTE;
	attribute->value.bValue = bValue;

	addAttributeToProtocol(protocol, attribute);
}

void addIntAttribute(Protocol *protocol, uint8_t mnemonic, int iValue) {
	if(protocol->text)
		return TACP_ERROR_CHANGE_CLOSED;

	ProtocolAttribute *attribute = malloc(sizeof(ProtocolAttribute));

	attribute->mnemonic = mnemonic;
	attribute->dataType = TYPE_INT;
	attribute->value.iValue = iValue;

	addAttributeToProtocol(protocol, attribute);
}

void addFloatAttribute(Protocol *protocol, uint8_t mnemonic, float fValue) {
	if(protocol->text)
		return TACP_ERROR_CHANGE_CLOSED;

	ProtocolAttribute *attribute = malloc(sizeof(ProtocolAttribute));

	attribute->mnemonic = mnemonic;
	attribute->dataType = TYPE_FLOAT;
	attribute->value.fValue = fValue;

	addAttributeToProtocol(protocol, attribute);
}

int setText(Protocol *protocol, char *text) {
	if(protocol->text)
		return TACP_ERROR_CHANGE_CLOSED;

	protocol->text = (char *)malloc(strlen(text) + 1);
	if (!protocol->text)
		return TACP_ERROR_OUT_OF_MEMEORY;

	strcpy(protocol->text, text);
	return 0;
}

ProtocolDescription createProtocolDescription(uint8_t mnemonic, ProtocolName name,
	ProtocolAttributeDescription attributes[], int attributeSize, bool acceptText) {
	ProtocolDescription pd = {
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
	for (int i = 0; i < pd.attributesSize; i++) {
		ProtocolAttributeDescription *pad = pd.attributes + i;
		pad->mnemonic = attributes[i].mnemonic;
		pad->name = attributes[i].name;
		pad->dataType = attributes[i].dataType;
	}

	return pd;
}

void registerInboundProtocol(ProtocolDescription protocolDescription,
		uint8_t (*processProtocol)(Protocol *), bool isQueryProtocol) {
	InboundProtocolRegistration *newestRegistration = malloc(sizeof(InboundProtocolRegistration));
	newestRegistration->description.mnemomic = protocolDescription.mnemomic;
	newestRegistration->description.name = protocolDescription.name;
	newestRegistration->description.attributes = protocolDescription.attributes;
	newestRegistration->description.attributesSize = protocolDescription.attributesSize;
	newestRegistration->processProtocol = processProtocol;
	newestRegistration->isQueryProtocol = isQueryProtocol;
	newestRegistration->next = NULL;
	
	if (inboundProtocolRegistrations) {
		InboundProtocolRegistration *last = inboundProtocolRegistrations;
		while (last->next) {
			last = last->next;
		}

		last->next = newestRegistration;
	} else {
		inboundProtocolRegistrations = newestRegistration;
	}
}

bool unregisterInboundProtocol(uint8_t mnemomic) {
	if (!inboundProtocolRegistrations)
		return false;

	InboundProtocolRegistration *current = inboundProtocolRegistrations;
	InboundProtocolRegistration *previous = NULL;
	while (current) {
		if (current->description.mnemomic != mnemomic) {
			previous = current;
			current = current->next;
			continue;
		}

		if (previous) {
			previous->next = current->next;
		} else {
			inboundProtocolRegistrations = current->next;
		}

		free(current);
		return true;
	}

	return false;
}

void registerOutboundProtocol(ProtocolDescription protocolDescription) {
	OutboundProtocolRegistration *newestRegistration = malloc(sizeof(OutboundProtocolRegistration));
	newestRegistration->description.mnemomic = protocolDescription.mnemomic;
	newestRegistration->description.name = protocolDescription.name;
	newestRegistration->description.attributes = protocolDescription.attributes;
	newestRegistration->description.attributesSize = protocolDescription.attributesSize;
	newestRegistration->next = NULL;

	if (outboundProtocolRegistrations) {
		OutboundProtocolRegistration *last = outboundProtocolRegistrations;
		while (last->next) {
			last = last->next;
		}

		last->next = newestRegistration;
	} else {
		outboundProtocolRegistrations = newestRegistration;
	}
}

bool unregisterOutboundProtocol(uint8_t mnemomic) {
	if (!outboundProtocolRegistrations)
		return false;

	OutboundProtocolRegistration *current = outboundProtocolRegistrations;
	OutboundProtocolRegistration *previous = NULL;
	while (current) {
		if (current->description.mnemomic != mnemomic) {
			previous = current;
			current = current->next;
			continue;
		}

		if (previous) {
			previous->next = current->next;
		} else {
			outboundProtocolRegistrations = current->next;
		}

		free(current);
		return true;
	}

	return false;
}

int escape(uint8_t data[], int size, ProtocolData *pData) {
	int position = 0;
	for (int i = 0; i < size; i++) {
		if (position > MAX_ATTRIBUTE_DATA_SIZE)
			return TACP_ERROR_ATTRIBUTE_DATA_TOO_LARGE;

		if (data[i] == FLAG_DOC_BEGINNING_END ||
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

	if (position == 1) {
		pData->data[1] = pData->data[0];
		pData->data[0] = FLAG_NOREPLACE;
		pData->dataSize = 2;
	} else if (position == 2) {
		pData->data[2] = pData->data[1];
		pData->data[1] = pData->data[0];
		pData->data[0] = FLAG_NOREPLACE;
		pData->dataSize = 3;
	} else {
		pData->dataSize = position;
	}

	return 0;
}

uint8_t getProtocolMnemonic(ProtocolData *pData) {
	return -1;
}

bool isValidProtocolData(ProtocolData *pData) {
	if (pData->dataSize < MIN_PROTOCOL_DATA_SIZE)
		return false;

	if (pData->data[0] != 0xff || pData->data[pData->dataSize - 1] != 0xff)
		return false;

	return true;
}

bool getInboundProtocolNameByMnemonic(uint8_t mnemonic, ProtocolName *name) {
	InboundProtocolRegistration *current = inboundProtocolRegistrations;
	while (current) {
		if (current->description.mnemomic == mnemonic) {
			*name = current->description.name;
			return true;
		}

		current = current->next;
	}

	return false;
}

bool isInboundProtocol(ProtocolData *pData, uint8_t mnemonic) {
	if (!isValidProtocolData(pData))
		return false;

	ProtocolName name;
	if (!getInboundProtocolNameByMnemonic(mnemonic, &name))
		return false;

	return pData->data[1] == name.namespace[0] &&
		pData->data[2] == name.namespace[1] &&
		pData->data[3] == name.localName;
}

ProtocolDescription *getOutboundProtocolDescriptionByMnemonic(uint8_t mnemonic) {
	OutboundProtocolRegistration *current = outboundProtocolRegistrations;
	while(current) {
		if(current->description.mnemomic == mnemonic) {
			return &current->description;
		}

		current = current->next;
	}

	return NULL;
}

ProtocolDescription *getInboundProtocolDescriptionByName(ProtocolName name) {
	InboundProtocolRegistration *current = inboundProtocolRegistrations;
	while (current) {
		if (current->description.name.namespace[0] == name.namespace[0] &&
					current->description.name.namespace[1] == name.namespace[1] &&
					current->description.name.localName == name.localName) {
			return &current->description;
		}

		current = current->next;
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
	for (int i = 0; i < description->attributesSize; i++) {
		if ((description->attributes + i)->mnemonic == mnemonic)
			return description->attributes + i;
	}

	return NULL;
}

int findAttributeValueEnd(ProtocolData *pData, int position, int *escapeNumber) {
	while (position <= (pData->dataSize - 1)) {
		uint8_t current = pData->data[position];
		if (current == FLAG_ESCAPE) {
			if ((position + 1) >= (pData->dataSize - 1))
				return -1;

			if (pData->data[position + 1] < 0xfa)
				return -1;

			(*escapeNumber)++;
			position += 2;
			continue;
		}

		if (pData->data[position] == FLAG_UNIT_SPLITTER ||
				pData->data[position] == FLAG_DOC_BEGINNING_END)
			return position;

		position++;
	}

	return -1;
}

int assembleProtocolAttributeValue(ProtocolData *pData, ProtocolAttribute *attribute,
			DataType dataType, int attributeValueStartPosition,
				int attributeValueEndPosition, int escapeNumber) {
	int attributeValueSize = (attributeValueEndPosition - attributeValueStartPosition - escapeNumber);
	if (dataType == TYPE_BYTE && attributeValueSize != 1)
		return TACP_ERROR_MALFORMED_PROTOCOL_DATA;

	if (dataType == TYPE_BYTE) {
		if (escapeNumber > 1)
			return TACP_ERROR_MALFORMED_PROTOCOL_DATA;

		uint8_t *value = malloc(sizeof(uint8_t));
		if (!value)
			return TACP_ERROR_OUT_OF_MEMEORY;

		if (escapeNumber == 0) {
			*value = pData->data[attributeValueStartPosition];
		} else {
			*value = pData->data[attributeValueStartPosition + 1];
		}
		attribute->value.bValue = *value;
	} else {
		int attributeDataSize = attributeValueEndPosition - attributeValueStartPosition;
		unsigned char *attributeValue = malloc(sizeof(char) * (attributeValueSize + 1));
		if (!attributeValue)
			return TACP_ERROR_OUT_OF_MEMEORY;

		bool escaped = false;
		int attributeValuePosition = 0;
		for (int i = 0; i < attributeDataSize; i++) {
			if (!escaped && pData->data[attributeValueStartPosition +  i] == FLAG_ESCAPE) {
				escaped = true;
				continue;
			}

			*(attributeValue + attributeValuePosition) = (char)pData->data[attributeValueStartPosition +  i];
			attributeValuePosition++;
			escaped =false;
		}
		*(attributeValue + attributeValueSize) = 0;

		if (dataType == TYPE_INT) {
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

int getAttributesSize(Protocol *protocol) {
	if (!protocol->attributes)
		return 0;

	int size = 0;
	ProtocolAttribute *current = protocol->attributes;
	while (current) {
		size++;
		current = current->next;
	}

	return size;
}

int doParseProtocol(ProtocolData *pData, Protocol *protocol) {
	protocol->attributes = NULL;
	protocol->text = NULL;

	if (!isValidProtocolData(pData))
		return TACP_ERROR_NOT_VALID_PROTOCOL;

	ProtocolName name = {
		{pData->data[1], pData->data[2]},
		pData->data[3]
	};

	ProtocolDescription *description = getInboundProtocolDescriptionByName(name);
	if (!description)
		return TACP_ERROR_UNKNOWN_PROTOCOL_NAME;

	uint8_t childrenSize = pData->data[5] & 0x7f;
	if (childrenSize > 0)
		return TACP_ERROR_FEATURE_EMBEDDED_PROTOCOL_NOT_IMPLEMENTED;

	protocol->mnemonic = description->mnemomic;
	protocol->attributes = NULL;
	protocol->text = NULL;

	uint8_t attributeSize = pData->data[4];
	bool hasText = ((pData->data[5] & 0x80) == 0x80);

	if (!description->acceptText && hasText)
		return TACP_ERROR_TEXT_NOT_ACCEPTED;

	if (attributeSize == 0 && !hasText) {
		if (pData->dataSize != 7)
			return TACP_ERROR_MALFORMED_PROTOCOL_DATA;

		return 0;
	}

	int position = 5;
	for (int i = 0; i < attributeSize; i++) {
		if (position++ >= (pData->dataSize - 1))
			return TACP_ERROR_MALFORMED_PROTOCOL_DATA;

		uint8_t attributeName = pData->data[position];
		ProtocolAttributeDescription *attributeDescription =
			getAttributeDescriptionByName(description, attributeName);

		if (!attributeDescription)
			return TACP_ERROR_UNKNOWN_ATTRIBUTE_NAME;

		ProtocolAttribute *attribute = malloc(sizeof(ProtocolAttribute));
		attribute->mnemonic = attributeDescription->mnemonic;
		attribute->dataType = attributeDescription->dataType;

		position++;
		int escapeNumber = 0;
		int attributeValueEndPosition = findAttributeValueEnd(pData, position, &escapeNumber);
		if (attributeValueEndPosition <= 0) {
			free(attribute);
			return TACP_ERROR_MALFORMED_PROTOCOL_DATA;
		}

		int result = assembleProtocolAttributeValue(pData, attribute,
			attributeDescription->dataType, position, attributeValueEndPosition, escapeNumber);
		if (result != 0) {
			free(attribute);
			return result;
		}

		position = attributeValueEndPosition;
		addAttributeToProtocol(protocol, attribute);
	}

	if (!hasText && (position != (pData->dataSize - 1))) {
		return TACP_ERROR_MALFORMED_PROTOCOL_DATA;
	}

	if (!hasText)
		return 0;

	position++;
	int textDataSize = pData->dataSize - position - 1;
	int escapeNumber = 0;
	bool escaped = false;
	for (int i = 0; i < textDataSize; i++) {
		uint8_t current = pData->data[position + i];
		if (!escaped && current == FLAG_ESCAPE) {
			if (position + i + 1 >= (pData->dataSize - 1) ||
					pData->data[position + i +1] < 0xfa)
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
	for (int i = 0; i < textDataSize; i++) {
		uint8_t current = pData->data[position + i];
		if (current == FLAG_ESCAPE) {
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

int8_t parseProtocol(ProtocolData *pData, Protocol *protocol) {
	int result = doParseProtocol(pData, protocol);
	if (result != 0)
		releaseProtocolResources(protocol);

	return result;
}

void releaseProtocolResources(Protocol *protocol) {
	if (protocol->text) {
		free(protocol->text);
		protocol->text = NULL;
	}

	ProtocolAttribute *last = protocol->attributes;
	while (last) {
		if (last->next) {
			last = last->next;
		} else {
			break;
		}
	}

	if (!last)
		return;

	ProtocolAttribute *previous = NULL;
	while (last) {
		previous = last->previous;

		if (last->dataType == TYPE_BYTES && last->value.bsValue != NULL) {
			free(last->value.bsValue);
		} else if(last->dataType == TYPE_STRING && last->value.sValue != NULL) {
			free(last->value.sValue);
		} else {
			// NOOP
		}

		free(last);
		previous->next = NULL;
		last = previous;
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
	if (getAttributesSize(protocol) > MAX_ATTRIBUTES_SIZE)
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

	int attributesSize = getAttributesSize(protocol);
	buff[4] = attributesSize;
	buff[5] = protocol->text ? 0x80 : 0x00;

	ProtocolData attributeData;
	if (attributesSize != 0) {
		attributeData.data = malloc(sizeof(uint8_t) * MAX_ATTRIBUTE_DATA_SIZE);
		if(!attributeData.data)
			return TACP_ERROR_OUT_OF_MEMEORY;
	}

	int position = 6;

	ProtocolAttribute *attribute = protocol->attributes;
	while (attribute) {
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

		if (position >= MAX_PROTOCOL_DATA_SIZE - 1) {
			free(attributeData.data);
			return TACP_ERROR_PROTOCOL_DATA_TOO_LARGE;
		}

		if (position + attributeData.dataSize >= MAX_PROTOCOL_DATA_SIZE - 1) {
			free(attributeData.data);
			return TACP_ERROR_PROTOCOL_DATA_TOO_LARGE;
		}

		memcpy(buff + position, attributeData.data, attributeData.dataSize);
		position += attributeData.dataSize;

		buff[position] = FLAG_UNIT_SPLITTER;
		position++;
		
		if (position >= MAX_PROTOCOL_DATA_SIZE - 1) {
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

int translateAndRelease(Protocol *protocol, ProtocolData *pData) {
	int result = translateProtocol(protocol, pData);
	releaseProtocolResources(protocol);

	return result;
}

ProtocolAttribute *getAttributeByMnemonic(Protocol *protocol, uint8_t mnemonic) {
	ProtocolAttribute *attribute = protocol->attributes;
	while(attribute) {
		if (attribute->mnemonic == mnemonic)
			return attribute;

		attribute = attribute->next;
	}

	return NULL;
}

bool getAttributeValueAsInt(Protocol *protocol, uint8_t mnemonic, int *value) {
	ProtocolAttribute *attribute = getAttributeByMnemonic(protocol, mnemonic);
	if (!attribute)
		return false;

	*value = attribute->value.iValue;
	return true;
}

bool getAttributeValueAsByte(Protocol *protocol, uint8_t mnemonic, uint8_t *value) {
	ProtocolAttribute *attribute = getAttributeByMnemonic(protocol,mnemonic);
	if(!attribute)
		return false;

	*value = attribute->value.bValue;
	return true;
}

char *getAttributeValueAsString(Protocol *protocol, uint8_t mnemonic) {
	ProtocolAttribute *attribute = getAttributeByMnemonic(protocol, mnemonic);
	if(!attribute)
		return NULL;

	return attribute->value.sValue;;
}

uint8_t *getAttributeValueAsBytes(Protocol *protocol, uint8_t mnemonic) {
	ProtocolAttribute *attribute = getAttributeByMnemonic(protocol, mnemonic);
	if(!attribute)
		return NULL;

	return attribute->value.bsValue;;
}

bool getAttributeValueAsFloat(Protocol *protocol, uint8_t mnemonic, float *value) {
	ProtocolAttribute *attribute = getAttributeByMnemonic(protocol,mnemonic);
	if(!attribute)
		return false;

	*value = attribute->value.fValue;
	return true;
}

char *getText(Protocol *protocol) {
	return protocol->text;
}

bool isLanAnswer(ProtocolData *pData) {
	return false;
}

void createLanResponse(TinyId requestId, ProtocolName protocolName,
		uint8_t lanResponse[SIZE_LAN_ANSWER]) {
}

void createLanError(TinyId requestId, ProtocolName protocolName,
		int8_t errorNumber, uint8_t lanError[SIZE_LAN_ANSWER]) {
}

bool isLanExecution(ProtocolData *pData) {
	return false;
}

int parseLanExecution(ProtocolData *pData, Protocol *action, TinyId tinyId) {
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
