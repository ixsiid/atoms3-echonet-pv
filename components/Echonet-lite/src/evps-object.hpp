#pragma once
#include "ECHONETlite-object.hpp"

enum class EVPS_Mode : uint8_t {
	Charge			 = 0x42,
	Discharge			 = 0x43,
	Stanby			 = 0x44,
	Charge_And_Discharge = 0x46,
	Stop				 = 0x47,
	Starting			 = 0x48,
	Auto				 = 0x49,
	Undefine			 = 0x40,
	Unacceptable         = 0x00,
};

typedef EVPS_Mode (*update_mode_cb_t)(EVPS_Mode current_mode, EVPS_Mode request_mode);

class EVPS : public ELObject {
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
