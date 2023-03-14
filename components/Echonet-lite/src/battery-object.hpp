#pragma once
#include "ECHONETlite-object.hpp"

class Battery : public ELObject {
    public:
	static const uint16_t class_u16 = 0x7d02;

    private:
	static const char TAG[8];
	uint8_t* battery[0xff];

	uint8_t get(uint8_t* epcs, uint8_t epc_count);
	uint8_t set(uint8_t* epcs, uint8_t epc_count);

    public:
	Battery(uint8_t instance);
	void notify_mode();
};
