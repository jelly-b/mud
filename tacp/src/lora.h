#ifndef TACP_LORA_H
#define TACP_LORA_H

struct Allocation {
	uint16_t gatewayUplinkAddress;
	uint8_t gatewayUplinkFrequencyBand;
	uint16_t gatewayDownlinkAddress;
	uint8_t gatewayDownlinkFrequencyBand;
	uint16_t allocatedAddress;
	uint8_t allocatedFrequencyBand;
};

bool isAllocation(uint8_t data[]);
int parseAllocation(uint8_t data[], struct Allocation *allocation);
bool isLanExecution(uint8_t data[]);

#endif