#include "protocols.h"

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

	pd.attributes = (ProtocolAttributeDescription *)malloc(sizeof(struct ProtocolAttributeDescription) * pd.attributesSize);
	for(int i = 0; i < pd.attributesSize; i++) {
		struct ProtocolAttributeDescription *pad = pd.attributes + i;
		pad->mnemonic = attributes[i].mnemonic;
		pad->name = attributes[i].name;
		pad->dataType = attributes[i].dataType;
	}

	return pd;
}