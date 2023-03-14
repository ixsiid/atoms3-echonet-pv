#pragma once
#include "ECHONETlite-object.hpp"

class Battery : public ELObject {
    public:
	static const uint16_t class_u16 = 0x7d02;

	enum class Mode : uint8_t {
		RapidCharge = 0x41,
		Charge = 0x42,
		Discharge = 0x43,
		Stanby = 0x44,
		Test = 0x45,
		Auto = 0x46,
		Rebooting = 0x48,
		Caluclation = 0x49,
		Other = 0x40,
	};

	enum class Type : uint8_t {
		Unknown = 0x00,
		Pb = 0x01, // 鉛
		NiH = 0x02,
		NiCd = 0x03,
		Li_ion = 0x04,
		Zn = 0x05,
		Chargable_Alkali = 0x06,
		// Reserved: 0x07 ~ 0xff
	};

    private:
	static const char TAG[8];
	uint8_t* battery[0xff];

	uint8_t get(uint8_t* epcs, uint8_t epc_count);
	uint8_t set(uint8_t* epcs, uint8_t epc_count);

    public:
	Battery(uint8_t instance);
	void update();
	void update(int watt);
	void notify_mode();
};
