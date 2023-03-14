#pragma once
#include "udp-socket.hpp"

#pragma pack(1)

typedef struct {
	uint16_t _1081;
	uint16_t packet_id;
	uint16_t src_device_class;
	uint8_t src_device_id;
	uint16_t dst_device_class;
	uint8_t dst_device_id;
	uint8_t esv;
	uint8_t epc_count;
} elpacket_t;

#pragma pack()

#define EL_MULTICAST_IP "224.0.23.0"
#define EL_PORT 3610
#define EL_BUFFER_SIZE 256

// to HEMS用
class ELObject {
    protected:
	static uint8_t buffer[EL_BUFFER_SIZE + 32];
	static size_t buffer_length;
	static elpacket_t* p;
	static uint8_t* epc_start;

	static uint8_t* maker_code;

	static const uint16_t CLASS_HEMS = 0xff05;

	virtual uint8_t get(uint8_t* epcs, uint8_t epc_count) = 0;
	virtual uint8_t set(uint8_t* epcs, uint8_t epc_count) = 0;

    public:
	const uint8_t instance;
	const uint16_t class_group;
	const uint8_t class_id;
	const uint8_t group_id;

	ELObject(uint8_t instance, uint16_t class_group);

	int send(UDPSocket* udp, const esp_ip_addr_t* addr);
	bool process(const elpacket_t* recv, uint8_t* epc_array);
};

class Profile : public ELObject {
    private:
	uint8_t* profile[0xff];

	// uint16_t _class_id	 = 0x7e02;

	uint8_t get(uint8_t* epcs, uint8_t epc_count);
	uint8_t set(uint8_t* epcs, uint8_t epc_count);

    public:
	Profile(uint8_t major_version, uint8_t minor_version);

	void add(ELObject * object);
	
	/* Stack over flowになって起動できない
	Profile operator<<(ELObject * object) {
		this->add(object);
		int i = profile[0xd6][1]++;
		ESP_LOGI(___tag, "i: %d", i);
		profile[0xd6][2 + i * 3 + 0] = object->group_id;
		profile[0xd6][2 + i * 3 + 0] = object->class_id;
		profile[0xd6][2 + i * 3 + 0] = object->instance;
		profile[0xd6][0] += 3;
		
		ESP_LOG_BUFFER_HEXDUMP("d6", profile[0xd6], profile[0xd6][1] + 1, ESP_LOG_INFO);
		return *this;
	}; */
};
