#ifndef MUD_PROTOCOLS_H
#define MUD_PROTOCOLS_H

#include <stdint.h>
#include <stdbool.h>

#define DATA_SIZE(data) sizeof(data) / sizeof(uint8_t)
#define ARRAY_SIZE(array) sizeof(array) / sizeof(array[0])
#define CREATE_PROTOCOL_DATA(data) {data, DATA_SIZE(data)}

typedef enum DataType {
	TYPE_BYTE,
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
	ProtocolAttributeDescription *attributes;
	uint8_t attributesSize;
	bool acceptText;
} ProtocolDescription;

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
	ProtocolAttributeDescription attributes[], int attributeSize, bool acceptText);

#endif
