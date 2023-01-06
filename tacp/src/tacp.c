#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "tacp.h"

#define MIN_SIZE_PROTOCOL_DATA 2 + 5
#define MIN_SIZE_LAN_EXECUTION_DATA MIN_SIZE_PROTOCOL_DATA + (SIZE_THINGS_TINY_ID + 2) + 5
#define MIN_SIZE_LAN_RESPONSE_DATA 2 + 5 + 1 + 1 + SIZE_THINGS_TINY_ID
#define MIN_SIZE_LAN_ERROR_DATA 2 + 5 + 1 + 1 + SIZE_THINGS_TINY_ID + 1 + 2 + 1

static InboundProtocolRegistration *inboundProtocolRegistrations = NULL;
static OutboundProtocolRegistration *outboundProtocolRegistrations = NULL;

static const uint8_t LAN_EXECUTION_PREFIX_BYTES[] ={
	0xff, 0xf8, 0x03, 0x05, 0x01, 0x01
};
static const int SIZE_LAN_EXECUTION_PREFIX_BYTES = sizeof(LAN_EXECUTION_PREFIX_BYTES);

static const uint8_t LAN_ERROR_PREFIX_BYTES[] = {
	0xff, 0xf8, 0x00, 0x07, 0x02, 0x00
};
static const uint8_t LAN_RESPONSE_PREFIX_BYTES[] ={
	0xff, 0xf8, 0x00, 0x07, 0x01, 0x00
};
static const int SIZE_LAN_ANSWER_PREFIX_BYTES = sizeof(LAN_ERROR_PREFIX_BYTES);

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

Protocol createEmptyProtocol() {
	return createEmptyProtocolByMenmonic(0xe0);
}

Protocol createEmptyProtocolByMenmonic(uint8_t menmonic) {
	Protocol pEmpty = {menmonic, NULL, NULL};
	return pEmpty;
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

	if (size > MAX_SIZE_ATTRIBUTE_DATA)
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
	if (length > MAX_SIZE_ATTRIBUTE_DATA)
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

int addByteAttribute(Protocol *protocol, uint8_t mnemonic, uint8_t bValue) {
	if(protocol->text)
		return TACP_ERROR_CHANGE_CLOSED;

	ProtocolAttribute *attribute = malloc(sizeof(ProtocolAttribute));

	attribute->mnemonic = mnemonic;
	attribute->dataType = TYPE_BYTE;
	attribute->value.bValue = bValue;

	addAttributeToProtocol(protocol, attribute);
	return 0;
}

int addIntAttribute(Protocol *protocol, uint8_t mnemonic, int iValue) {
	if(protocol->text)
		return TACP_ERROR_CHANGE_CLOSED;

	ProtocolAttribute *attribute = malloc(sizeof(ProtocolAttribute));

	attribute->mnemonic = mnemonic;
	attribute->dataType = TYPE_INT;
	attribute->value.iValue = iValue;

	addAttributeToProtocol(protocol, attribute);
	return 0;
}

int addFloatAttribute(Protocol *protocol, uint8_t mnemonic, float fValue) {
	if(protocol->text)
		return TACP_ERROR_CHANGE_CLOSED;

	ProtocolAttribute *attribute = malloc(sizeof(ProtocolAttribute));

	attribute->mnemonic = mnemonic;
	attribute->dataType = TYPE_FLOAT;
	attribute->value.fValue = fValue;

	addAttributeToProtocol(protocol, attribute);
	return 0;
}

int setText(Protocol *protocol, char *text) {
	if(protocol->text)
		return TACP_ERROR_CHANGE_CLOSED;

	if (strlen(text) > MAX_SIZE_TEXT_DATA)
		return TACP_ERROR_TEXT_DATA_TOO_LARGE;

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

	if (pd.attributesSize != 0) {
		pd.attributes = (ProtocolAttributeDescription *)malloc(sizeof(ProtocolAttributeDescription) * pd.attributesSize);
		for(int i = 0; i < pd.attributesSize; i++) {
			ProtocolAttributeDescription *pad = pd.attributes + i;
			pad->mnemonic = attributes[i].mnemonic;
			pad->name = attributes[i].name;
			pad->dataType = attributes[i].dataType;
		}
	} else {
		pd.attributes = NULL;
	}

	return pd;
}

void registerInboundProtocol(ProtocolDescription description,
		uint8_t (*processProtocol)(Protocol *), bool isQueryProtocol) {
	InboundProtocolRegistration *newestRegistration = malloc(sizeof(InboundProtocolRegistration));
	newestRegistration->description.mnemomic = description.mnemomic;
	newestRegistration->description.name = description.name;

	newestRegistration->description.attributes = malloc(sizeof(ProtocolAttributeDescription) * description.attributesSize);
	for(int i = 0; i < description.attributesSize; i++) {
		ProtocolAttributeDescription *pad = (newestRegistration->description.attributes) + i;
		pad->mnemonic = ((description.attributes) + i)->mnemonic;
		pad->name = ((description.attributes) + i)->name;
		pad->dataType = ((description.attributes) + i)->dataType;
	}
	newestRegistration->description.attributesSize = description.attributesSize;

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

void registerOutboundProtocol(ProtocolDescription description) {
	OutboundProtocolRegistration *newestRegistration = malloc(sizeof(OutboundProtocolRegistration));
	newestRegistration->description.mnemomic = description.mnemomic;
	newestRegistration->description.name = description.name;

	newestRegistration->description.attributes = malloc(sizeof(ProtocolAttributeDescription) * description.attributesSize);
	for (int i = 0; i < description.attributesSize; i++) {
		ProtocolAttributeDescription *pad = (newestRegistration->description.attributes) + i;
		pad->mnemonic = ((description.attributes) + i)->mnemonic;
		pad->name = ((description.attributes) + i)->name;
		pad->dataType = ((description.attributes) + i)->dataType;
	}
	newestRegistration->description.attributesSize = description.attributesSize;
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
	if (size > MAX_SIZE_ATTRIBUTE_DATA)
		return TACP_ERROR_ATTRIBUTE_DATA_TOO_LARGE;

	int position = 0;
	uint8_t buff[MAX_SIZE_ATTRIBUTE_DATA];
	for (int i = 0; i < size; i++) {
		if (position > MAX_SIZE_ATTRIBUTE_DATA)
			return TACP_ERROR_ATTRIBUTE_DATA_TOO_LARGE;

		if (data[i] == FLAG_DOC_BEGINNING_END ||
				data[i] == FLAG_UNIT_SPLITTER ||
				data[i] == FLAG_NOREPLACE ||
				data[i] == FLAG_ESCAPE ||
				data[i] == FLAG_BYTES_TYPE ||
				data[i] == FLAG_BYTE_TYPE) {
			buff[position] = FLAG_ESCAPE;
			position++;
		}

		buff[position] = data[i];
		position++;
	}

	if (position == 1) {
		pData->data = malloc(sizeof(uint8_t) * 2);
		if (!pData->data)
			return TACP_ERROR_OUT_OF_MEMEORY;

		pData->data[1] = buff[0];
		pData->data[0] = FLAG_NOREPLACE;
		pData->dataSize = 2;
	} else if (position == 2) {
		pData->data = malloc(sizeof(uint8_t) * 3);
		if(!pData->data)
			return TACP_ERROR_OUT_OF_MEMEORY;

		pData->data[2] = pData->data[1];
		pData->data[1] = pData->data[0];
		pData->data[0] = FLAG_NOREPLACE;
		pData->dataSize = 3;
	} else {
		pData->data = malloc(sizeof(uint8_t) * position);
		if(!pData->data)
			return TACP_ERROR_OUT_OF_MEMEORY;

		memcpy(pData->data, buff, position);
		pData->dataSize = position;
	}

	return 0;
}

bool isEscapedByte(uint8_t b) {
	return b >= 0xfa && b <= 0xff;
}

int unescape(uint8_t data[], int size, ProtocolData *pData) {
	if (size > MAX_SIZE_TEXT_DATA)
		return TACP_ERROR_ATTRIBUTE_DATA_TOO_LARGE;

	int position = 0;
	uint8_t buff[MAX_SIZE_TEXT_DATA];
	for (int i = 0; i < size; i++) {
		if (data[i] == FLAG_ESCAPE && i < (size - 1) && isEscapedByte(data[i + 1])) {
			continue;
		}

		buff[position] = data[i];
		position++;
	}

	if (position == 1) {
		pData->data = malloc(sizeof(uint8_t) * 1);
		if (!pData->data)
			TACP_ERROR_OUT_OF_MEMEORY;

		pData->data[0] = buff[0];
		pData->dataSize = 1;
	} else if (buff[0] == FLAG_BYTES_TYPE) {
		pData->data = malloc(sizeof(uint8_t) * (position - 1));
		if(!pData->data)
			TACP_ERROR_OUT_OF_MEMEORY;

		memcpy(pData->data, buff + 1, position - 1);
		pData->dataSize = position - 1;
	} else if ((position == 2 && buff[0] == FLAG_BYTE_TYPE) ||
				(position == 2 && buff[0] == FLAG_NOREPLACE)) {
		pData->data = malloc(sizeof(uint8_t) * 1);
		if(!pData->data)
			TACP_ERROR_OUT_OF_MEMEORY;

		pData->data[0] = buff[1];
		pData->dataSize = 1;
	} else if (position == 3 && buff[0] == FLAG_NOREPLACE) {
		pData->data = malloc(sizeof(uint8_t) * 2);
		if(!pData->data)
			TACP_ERROR_OUT_OF_MEMEORY;

		pData->data[0] = buff[1];
		pData->data[1] = buff[2];
		pData->dataSize = 2;
	} else {
		pData->data = malloc(sizeof(uint8_t) * position);
		if(!pData->data)
			TACP_ERROR_OUT_OF_MEMEORY;

		memcpy(pData->data, buff, position);
		pData->dataSize = position;
	}

	return 0;
}

bool isValidProtocolData(ProtocolData *pData) {
	if (pData->dataSize < MIN_SIZE_PROTOCOL_DATA)
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
	InboundProtocolRegistration *registration = getInboundProtocolRegistrationByName(name);
	if (registration)
		return &registration->description;
}

InboundProtocolRegistration *getInboundProtocolRegistrationByName(ProtocolName name) {
	InboundProtocolRegistration *current = inboundProtocolRegistrations;
	while(current) {
		if(current->description.name.namespace[0] == name.namespace[0] &&
			current->description.name.namespace[1] == name.namespace[1] &&
			current->description.name.localName == name.localName) {

			return current;
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
	if (dataType == TYPE_BYTE) {
		if (escapeNumber > 1)
			return TACP_ERROR_MALFORMED_PROTOCOL_DATA;

		uint8_t *value = malloc(sizeof(uint8_t));
		if (!value)
			return TACP_ERROR_OUT_OF_MEMEORY;

		if (escapeNumber == 0) {
			*value = pData->data[attributeValueStartPosition + 1];
		} else {
			*value = pData->data[attributeValueStartPosition + 2];
		}
		attribute->value.bValue = *value;
	} else {
		ProtocolData unescapedValue;
		int attributeDataSize = attributeValueEndPosition - attributeValueStartPosition;
		int unescapeResult = unescape(pData->data + attributeValueStartPosition,
			attributeDataSize, &unescapedValue);
		if (unescapeResult != 0) {
			releaseProtocolData(&unescapedValue);
			return unescapeResult;
		}

		if (dataType == TYPE_BYTES) {
			uint8_t *attributeValue = malloc(sizeof(uint8_t) * (unescapedValue.dataSize + 1));
			if(!attributeValue) {
				releaseProtocolData(&unescapedValue);
				return TACP_ERROR_OUT_OF_MEMEORY;
			}

			memcpy(attributeValue + 1, unescapedValue.data, unescapedValue.dataSize);
			attributeValue[0] = unescapedValue.dataSize;

			releaseProtocolData(&unescapedValue);

			attribute->value.bsValue = attributeValue;
		} else {
			char *attributeValue = malloc(sizeof(char) * (unescapedValue.dataSize + 1));
			if(!attributeValue) {
				releaseProtocolData(&unescapedValue);
				return TACP_ERROR_OUT_OF_MEMEORY;
			}

			memcpy(attributeValue, unescapedValue.data, unescapedValue.dataSize);
			attributeValue[unescapedValue.dataSize] = '\0';

			releaseProtocolData(&unescapedValue);

			if(dataType == TYPE_INT) {
				attribute->value.iValue = atoi(attributeValue);
			} else if(dataType == TYPE_FLOAT) {
				attribute->value.fValue = atof(attributeValue);
			} else {
				attribute->value.sValue = attributeValue;
			}

			if(dataType != TYPE_STRING)
				free(attributeValue);
		}
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
	ProtocolData textData;
	int unescapeResult = unescape(pData->data + position, textDataSize, &textData);
	if (unescapeResult != 0) {
		releaseProtocolData(&textData);
		return unescapeResult;
	}

	char *text = malloc(sizeof(char) * (textDataSize + 1));
	if(!text)
		return TACP_ERROR_OUT_OF_MEMEORY;

	memcpy(text, textData.data, textDataSize);
	*(text + textDataSize) = 0;
	releaseProtocolData(&textData);

	protocol->text = text;

	return 0;
}

int parseProtocol(ProtocolData *pData, Protocol *protocol) {
	int result = doParseProtocol(pData, protocol);
	if (result != 0)
		releaseProtocol(protocol);

	return result;
}

void releaseProtocol(Protocol *protocol) {
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

		if (previous)
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
	if (getAttributesSize(protocol) > MAX_SIZE_ATTRIBUTES)
		return TACP_ERROR_TOO_MANY_ATTRIBUTES;

	ProtocolDescription *description = getOutboundProtocolDescriptionByMnemonic(protocol->mnemonic);
	if (!description)
		return TACP_ERROR_UNKNOWN_PROTOCOL_MNEMONIC;

	if (!description->acceptText && protocol->text)
		return TACP_ERROR_TEXT_NOT_ACCEPTED;

	uint8_t buff[MAX_SIZE_PROTOCOL_DATA];
	buff[0] = 0xff;
	buff[1] = description->name.namespace[0];
	buff[2] = description->name.namespace[1];
	buff[3] = description->name.localName;

	int attributesSize = getAttributesSize(protocol);
	buff[4] = attributesSize;
	buff[5] = protocol->text ? 0x80 : 0x00;

	int position = 6;

	ProtocolAttribute *attribute = protocol->attributes;
	while (attribute) {
		ProtocolAttributeDescription *attributeDescription = getAttributeDescriptionByMnemonic(description, attribute->mnemonic);
		if (!attributeDescription)
			return TACP_ERROR_UNKNOWN_PROTOCOL_ATTRIBUTE_MNEMONIC;

		if (position >= MAX_SIZE_PROTOCOL_DATA - 1)
			return TACP_ERROR_PROTOCOL_DATA_TOO_LARGE;

		buff[position] = attributeDescription->name;
		position++;

		if(position >= MAX_SIZE_PROTOCOL_DATA - 1)
			return TACP_ERROR_PROTOCOL_DATA_TOO_LARGE;

		ProtocolData attributeData;
		int escapeResult = 0;
		if (attributeDescription->dataType == TYPE_BYTE) {
			uint8_t b = attribute->value.bValue;
			if (isEscapedByte(b)) {
				attributeData.data = malloc(sizeof(uint8_t) * 2);
				attributeData.data[0] = FLAG_ESCAPE;
				attributeData.data[1] = b;
				attributeData.dataSize = 2;
			} else {
				attributeData.data = malloc(sizeof(uint8_t) * 1);
				attributeData.data[0] = b;
				attributeData.dataSize = 1;
			}
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
			releaseProtocolData(&attributeData);
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

		if (position >= MAX_SIZE_PROTOCOL_DATA - 1) {
			releaseProtocolData(&attributeData);
			return TACP_ERROR_PROTOCOL_DATA_TOO_LARGE;
		}

		if (position + attributeData.dataSize >= MAX_SIZE_PROTOCOL_DATA - 1) {
			releaseProtocolData(&attributeData);
			return TACP_ERROR_PROTOCOL_DATA_TOO_LARGE;
		}

		memcpy(buff + position, attributeData.data, attributeData.dataSize);
		position += attributeData.dataSize;

		buff[position] = FLAG_UNIT_SPLITTER;
		position++;
		
		if (position >= MAX_SIZE_PROTOCOL_DATA - 1) {
			releaseProtocolData(&attributeData);
			return TACP_ERROR_PROTOCOL_DATA_TOO_LARGE;
		}

		releaseProtocolData(&attributeData);

		attribute = attribute->next;
	}

	if (protocol->text) {
		int textSize = strlen(protocol->text);
		if (position + textSize >= MAX_SIZE_PROTOCOL_DATA - 1)
			return TACP_ERROR_PROTOCOL_DATA_TOO_LARGE;

		memcpy(buff + position, protocol->text, textSize);
		position += textSize;
	}

	int dataSize;
	if (buff[position - 1] == FLAG_UNIT_SPLITTER) {
		buff[position - 1] = FLAG_DOC_BEGINNING_END;
		dataSize = position;
	} else {
		if (position >= MAX_SIZE_PROTOCOL_DATA - 1)
			return TACP_ERROR_PROTOCOL_DATA_TOO_LARGE;

		buff[position] = FLAG_DOC_BEGINNING_END;
		dataSize = position + 1;
	}

	pData->data = NULL;
	pData->dataSize = 0;

	pData->data = malloc(dataSize * sizeof(uint8_t));
	if (!pData->data)
		return TACP_ERROR_OUT_OF_MEMEORY;

	memcpy(pData->data, buff, dataSize);
	pData->dataSize = dataSize;

	return 0;
}

int translateAndRelease(Protocol *protocol, ProtocolData *pData) {
	int result = translateProtocol(protocol, pData);
	releaseProtocol(protocol);

	return result;
}

int translateLanExecution(TinyId tinyId, Protocol *action, ProtocolData *pData) {
	ProtocolData pDataAction;
	int translateActionResult = translateProtocol(action, &pDataAction);
	if (translateActionResult != 0)
		return TACP_ERROR_FAILED_TO_TRANSLATE_PROTOCOL;

	ProtocolData pDataEscapedTinyId;
	if (escape(tinyId, SIZE_THINGS_TINY_ID, &pDataEscapedTinyId) != 0) {
		return TACP_ERROR_FAILED_TO_ESCAPE;
	}

	int pDataActionSize = pDataAction.dataSize - 2;
	int lanExecutionSize = MIN_SIZE_PROTOCOL_DATA + 3 + pDataEscapedTinyId.dataSize + pDataActionSize;
	if (lanExecutionSize > MAX_SIZE_PROTOCOL_DATA)
		return TACP_ERROR_PROTOCOL_DATA_TOO_LARGE;

	uint8_t leBuff[MAX_SIZE_PROTOCOL_DATA];
	
	int position = 0;
	memcpy(leBuff, LAN_EXECUTION_PREFIX_BYTES, SIZE_LAN_EXECUTION_PREFIX_BYTES);
	position += SIZE_LAN_EXECUTION_PREFIX_BYTES;

	leBuff[position] = 0x06;
	position++;
	leBuff[position] = FLAG_BYTES_TYPE;
	position++;

	memcpy(leBuff + position, pDataEscapedTinyId.data, pDataEscapedTinyId.dataSize);
	position += pDataEscapedTinyId.dataSize;

	leBuff[position] = FLAG_UNIT_SPLITTER;
	position++;

	memcpy(leBuff + position, pDataAction.data + 1, pDataAction.dataSize - 2);
	position += pDataAction.dataSize - 2;

	leBuff[position] = FLAG_DOC_BEGINNING_END;
	
	pData->data = malloc(sizeof(uint8_t) * (position + 1));
	if (!pData->data)
		return TACP_ERROR_OUT_OF_MEMEORY;

	memcpy(pData->data, leBuff, (position + 1));
	pData->dataSize = position + 1;

	return 0;
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
	if(!isValidProtocolData(pData))
		return false;

	if (pData->dataSize < MIN_SIZE_LAN_RESPONSE_DATA)
		return false;

	return pData->data[1] == 0xf8 &&
		pData->data[2] == 0x00 &&
		pData->data[3] == 0x07;
}

LanAnswer createLanResonse(TinyId requestId) {
	LanAnswer answer;
	makeResponseTinyId(requestId, answer.traceId);
	answer.errorNumber = 0;

	return answer;
}

LanAnswer createLanError(TinyId requestId, uint8_t errorNumber) {
	LanAnswer answer;
	makeErrorTinyId(requestId, answer.traceId);
	answer.errorNumber = errorNumber;

	return answer;
}

bool isLanExecution(ProtocolData *pData) {
	if(!isValidProtocolData(pData))
		return false;

	return pData->data[1] == 0xf8 &&
		pData->data[2] == 0x03 &&
		pData->data[3] == 0x05;
}

int parseLanAnswer(ProtocolData *pData, LanAnswer *answer) {
	if (!isLanAnswer(pData))
		return TACP_ERROR_MALFORMED_PROTOCOL_DATA;

	int position = SIZE_LAN_ANSWER_PREFIX_BYTES;
	if (pData->data[position] != 0x06)
		return TACP_ERROR_MALFORMED_PROTOCOL_DATA;

	position++;
	if(pData->data[position] != FLAG_BYTES_TYPE)
		return TACP_ERROR_MALFORMED_PROTOCOL_DATA;

	position++;
	int traceIdEndPosition = -1;
	for (int i = position; i < pData->dataSize; i++) {
		if (pData->data[i] == FLAG_UNIT_SPLITTER || pData->data[i] == FLAG_DOC_BEGINNING_END) {
			traceIdEndPosition = i;
			break;
		}
	}

	if (traceIdEndPosition == -1)
		return TACP_ERROR_MALFORMED_PROTOCOL_DATA;

	ProtocolData traceId;
	if (unescape(pData->data + position, traceIdEndPosition - position, &traceId) != 0)
		return TACP_ERROR_FAILED_TO_UNESCAPE;

	if (traceId.dataSize != SIZE_THINGS_TINY_ID)
		return TACP_ERROR_FAILED_TO_UNESCAPE;

	memcpy(answer->traceId, traceId.data, SIZE_THINGS_TINY_ID);

	if (isResponseTinyId(answer->traceId)) {
		if (pData->data[traceIdEndPosition] != FLAG_DOC_BEGINNING_END)
			return TACP_ERROR_MALFORMED_PROTOCOL_DATA;

		answer->errorNumber = 0;

		return 0;
	}

	if(!isErrorTinyId(answer->traceId))
		return TACP_ERROR_UNKNOWN_ANSWER_TINY_ID_TYPE;

	position = traceIdEndPosition;
	if(pData->data[position] != FLAG_UNIT_SPLITTER)
		return TACP_ERROR_MALFORMED_PROTOCOL_DATA;

	if (position + 1 + 1 + 1 + 1 > (pData->dataSize - 1))
		return TACP_ERROR_MALFORMED_PROTOCOL_DATA;

	position++;
	if (pData->data[position] != 0x08)
		return TACP_ERROR_MALFORMED_PROTOCOL_DATA;

	position++;
	if(pData->data[position] != FLAG_BYTE_TYPE)
		return TACP_ERROR_MALFORMED_PROTOCOL_DATA;

	position++;
	if (pData->data[position] == FLAG_ESCAPE) {
		answer->errorNumber = pData->data[position + 1];
	} else {
		answer->errorNumber = pData->data[position];
	}

	return 0;
}

int translateLanResonse(LanAnswer *answer, ProtocolData *pData, ProtocolData *escapedTraceId) {
	uint8_t buff[MAX_SIZE_PROTOCOL_DATA];

	int position = 0;
	buff[position] = 0xff;
	memcpy(buff + 1, LAN_RESPONSE_PREFIX_BYTES, SIZE_LAN_ANSWER_PREFIX_BYTES);
	position += SIZE_LAN_ANSWER_PREFIX_BYTES;

	buff[position] = 0x06;
	position++;

	buff[position] = FLAG_BYTES_TYPE;
	position++;
	memcpy(buff + position, escapedTraceId->data, escapedTraceId->dataSize);
	position += escapedTraceId->dataSize;

	buff[position] = 0xff;

	pData->data = malloc(sizeof(uint8_t) * position);
	if(!pData->data)
		return TACP_ERROR_OUT_OF_MEMEORY;

	memcpy(pData->data, buff, position);
	pData->dataSize = position;

	return 0;
}

int translateLanError(LanAnswer *answer, ProtocolData *pData, ProtocolData *escapedTraceId) {
	uint8_t buff[MAX_SIZE_PROTOCOL_DATA];

	int position = 0;
	memcpy(buff, LAN_ERROR_PREFIX_BYTES, SIZE_LAN_ANSWER_PREFIX_BYTES);
	position += SIZE_LAN_ANSWER_PREFIX_BYTES;

	buff[position] = 0x06;
	position++;

	buff[position] = FLAG_BYTES_TYPE;
	position++;
	memcpy(buff + position, escapedTraceId->data, escapedTraceId->dataSize);
	position += escapedTraceId->dataSize;

	buff[position] = FLAG_UNIT_SPLITTER;
	position++;

	buff[position] = 0x08;
	position++;

	buff[position] = FLAG_BYTE_TYPE;
	position++;

	if (isEscapedByte(answer->errorNumber)) {
		buff[position] = FLAG_ESCAPE;
		buff[position] = answer->errorNumber;
		position += 2;
	} else {
		buff[position] = answer->errorNumber;
		position++;
	}
	buff[position] = 0xff;

	int dataSize = position + 1;
	pData->data = malloc(sizeof(uint8_t) * dataSize);
	if(!pData->data)
		return TACP_ERROR_OUT_OF_MEMEORY;

	memcpy(pData->data, buff, dataSize);
	pData->dataSize = dataSize;

	return 0;
}

int translateLanAnswer(LanAnswer *answer, ProtocolData *pData) {
	ProtocolData escapedTraceId = {NULL, 0};
	if (escape(answer->traceId, SIZE_THINGS_TINY_ID, &escapedTraceId) != 0) {
		releaseProtocolData(&escapedTraceId);
		return TACP_ERROR_FAILED_TO_ESCAPE;
	}

	int result;
	if (isResponseTinyId(answer->traceId)) {
		result = translateLanResonse(answer, pData, &escapedTraceId);
	} else if (isErrorTinyId(answer->traceId)) {
		result = translateLanError(answer, pData, &escapedTraceId);
	} else {
		releaseProtocolData(&escapedTraceId);
		return TACP_ERROR_UNKNOWN_ANSWER_TINY_ID_TYPE;
	}

	releaseProtocolData(&escapedTraceId);
	if (result != 0) {
		releaseProtocolData(pData);
		return TACP_ERROR_FAILED_TO_TRANSLATE_ANSWER;
	}

	return 0;
}

int parseLanExecution(ProtocolData *pData, TinyId requestId, Protocol *action) {
	if (!isLanExecution(pData) || pData->dataSize < MIN_SIZE_LAN_EXECUTION_DATA)
		return TACP_ERROR_MALFORMED_PROTOCOL_DATA;

	uint8_t attributeSize = pData->data[4];
	uint8_t childrenSize = pData->data[5] & 0x7f;
	bool hasText = (pData->data[5] & 0x80) == 0x80;

	if (attributeSize != 1 || childrenSize != 1 || hasText)
		return TACP_ERROR_MALFORMED_PROTOCOL_DATA;

	int requestIdStartPosition = 1 + 5 + 1 + 1;
	memcpy(requestId, pData->data + requestIdStartPosition, SIZE_THINGS_TINY_ID);

	int actionStartPosition = requestIdStartPosition + SIZE_THINGS_TINY_ID + 1;
	uint8_t actionBuff[MAX_SIZE_PROTOCOL_DATA];
	int actionDataSize = pData->dataSize - actionStartPosition - 1;
	memcpy(actionBuff + 1, pData->data + actionStartPosition, actionDataSize);
	actionBuff[0] = 0xff;
	actionBuff[actionDataSize + 1] = 0xff;

	ProtocolData pDataAction = {actionBuff, actionDataSize + 2};
	int parseActionResult = parseProtocol(&pDataAction, action);
	if(parseActionResult != 0) {
		releaseProtocol(action);
		return parseActionResult;
	}

	return 0;
}

int translateLanNotify(struct Protocol *event, uint8_t *data) {
	return -1;
}
