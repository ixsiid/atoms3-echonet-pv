#include <esp_log.h>
#include <esp_netif_ip_addr.h>
#include "ECHONETlite-object.hpp"

/// ELObject

static const char* TAG = "  EL Obj";

uint8_t ELObject::buffer[]	 = {};
size_t ELObject::buffer_length = sizeof(elpacket_t);
elpacket_t* ELObject::p		 = (elpacket_t*)ELObject::buffer;
uint8_t* ELObject::epc_start	 = buffer + sizeof(elpacket_t);

uint8_t* ELObject::maker_code = new uint8_t[4]{0x03, 0xff, 0xff, 0xff};  // 開発用

ELObject::ELObject(uint8_t instance, uint16_t class_group) : 
	instance(instance), class_group(class_group),
	class_id(class_group >> 8), group_id(class_group & 0xff) {
	p->_1081			= 0x8110;
	p->dst_device_class = ELObject::CLASS_HEMS;
	p->dst_device_id	= 0x01;
}

int ELObject::send(UDPSocket* udp, const esp_ip_addr_t* addr) {
	int len = udp->write(addr, EL_PORT, buffer, buffer_length);
	ESP_LOG_BUFFER_HEXDUMP(TAG, buffer, len, ESP_LOG_INFO);
	return len;
}

bool ELObject::process(const elpacket_t* recv, uint8_t* epcs) {
	// ESP_LOGI(TAG, "Profile: process");
	// GET
	p->packet_id = recv->packet_id;
	if (recv->esv == 0x62 || recv->esv == 0x61 || recv->esv == 0x60) {
		buffer_length = 0;
		recv->esv == 0x62 ? p->epc_count = get(epcs, recv->epc_count) : p->epc_count = set(epcs, recv->epc_count);

		if (recv->esv == 0x60) return false;

		p->src_device_class = class_group;
		p->src_device_id = instance;

		if (p->epc_count > 0) {
			p->esv = recv->esv + 0x10;
		} else {
			p->esv					 = recv->esv - 0x10;
			buffer[sizeof(elpacket_t) + 0] = 0;
			buffer_length				 = sizeof(elpacket_t) + 1;
		}

		// ESP_LOG_BUFFER_HEXDUMP(TAG, buffer, buffer_length, ESP_LOG_INFO);
		return true;
	}

	return false;
};

//// Profile
static const char* TAG_P = " EL Prof";

// Versionは、ECHONET lite規格書のVersion
// not 機器オブジェクト詳細規定
Profile::Profile(uint8_t major_version, uint8_t minor_version) : ELObject(1, 0xf00e), profile{} {
	profile[0x8a] = maker_code;
	profile[0x82] = new uint8_t[0x05]{0x04, major_version, minor_version, 0x01, 0x00};
	profile[0x83] = new uint8_t[0x12]{0x11, 0xfe, maker_code[0], maker_code[1], maker_code[2], 0x0d,
	                                              0x05, 0x06, 0x07, 0x08,
									      0x09, 0x0a, 0x0b, 0x0c,
									      0x0d, 0x0e, 0x0f, 0x10}; // 識別ID
	profile[0xd6] = new uint8_t[0xfe]{0x01, 0x00};
};

void Profile::add(ELObject * object) {
	int i = profile[0xd6][1]++;
	ESP_LOGI(TAG_P, "i: %d", i);
	profile[0xd6][2 + i * 3 + 0] = object->group_id;
	profile[0xd6][2 + i * 3 + 1] = object->class_id;
	profile[0xd6][2 + i * 3 + 2] = object->instance;
	profile[0xd6][0] += 3;
	
	ESP_LOG_BUFFER_HEXDUMP("d6", profile[0xd6], profile[0xd6][0] + 1, ESP_LOG_INFO);
};

uint8_t Profile::set(uint8_t* epcs, uint8_t count) { return 0; }

uint8_t Profile::get(uint8_t* epcs, uint8_t count) {
	ESP_LOGI(TAG_P, "Profile: get %d", count);
	p->src_device_class = class_group;
	p->src_device_id	= instance;

	uint8_t* t = epcs;
	uint8_t* n = epc_start;
	uint8_t res_count;

	for (res_count = 0; res_count < count; res_count++) {
		uint8_t epc = t[0];
		uint8_t len = t[1];
		ESP_LOGI(TAG_P, "EPC 0x%02x [%d]", epc, len);
		t += 2;

		if (profile[epc] == nullptr) return 0;

		if (len > 0) {
			ESP_LOG_BUFFER_HEXDUMP(TAG_P, t, len, ESP_LOG_INFO);
			t += len;
		}

		*n = epc;
		n++;
		memcpy(n, profile[epc], profile[epc][0] + 1);
		n += profile[epc][0] + 1;
	}

	buffer_length = sizeof(elpacket_t) + (n - epc_start);

	return res_count;
};

