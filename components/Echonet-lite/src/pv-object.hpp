#pragma once
#include "ECHONETlite-object.hpp"

class PV : public ELObject {
    private:
	static const char TAG[8];

	uint8_t* pv[0xff];

	uint8_t get(uint8_t* epcs, uint8_t epc_count);
	uint8_t set(uint8_t* epcs, uint8_t epc_count);

	portTickType timer;

    public:
	PV(uint8_t instance);
	void update();
	void update(uint16_t watt);
	void notify_mode();
};
