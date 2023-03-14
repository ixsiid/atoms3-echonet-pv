#pragma once
#include "ECHONETlite-object.hpp"

class EVPS : public ELObject {
    public:
	static const uint16_t class_u16 = 0x7e02;

	enum class Mode : uint8_t {
		Charge			 = 0x42,
		Discharge			 = 0x43,
		Stanby			 = 0x44,
		Charge_And_Discharge = 0x46,
		Stop				 = 0x47,
		Starting			 = 0x48,
		Auto				 = 0x49,
		Undefine			 = 0x40,
		Unacceptable		 = 0x00,
	};

	typedef Mode (*update_mode_cb_t)(Mode current_mode, Mode request_mode);

    private:
	static const char TAG[8];

	uint8_t* evps[0xff];

	uint8_t get(uint8_t* epcs, uint8_t epc_count);
	uint8_t set(uint8_t* epcs, uint8_t epc_count);

	update_mode_cb_t update_mode_cb;

    public:
	EVPS(uint8_t instance);
	void set_update_mode_cb(update_mode_cb_t cb);
	void notify_mode();
};
