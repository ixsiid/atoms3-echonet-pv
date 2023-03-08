#include <esp_log.h>
#include <esp_netif_ip_addr.h>
#include "ECHONETlite-object.hpp"

/// ELObject

#define ___tag "EL Object"

uint8_t ELObject::buffer[]	 = {};
size_t ELObject::buffer_length = sizeof(elpacket_t);
elpacket_t* ELObject::p		 = (elpacket_t*)ELObject::buffer;
uint8_t* ELObject::epc_start	 = buffer + sizeof(elpacket_t);

uint8_t* ELObject::maker_code = new uint8_t[4]{0x03, 0xff, 0xff, 0xff};  // 開発用

ELObject::ELObject() {
	p->_1081			= 0x8110;
	p->dst_device_class = ELObject::CLASS_HEMS;
	p->dst_device_id	= 0x01;
}

uint16_t ELObject::class_id() { return _class_id; }
uint8_t ELObject::instance() { return _instance; }

int ELObject::send(UDPSocket* udp, const esp_ip_addr_t* addr) {
	int len = udp->write(addr, EL_PORT, buffer, buffer_length);
	return len;
}

bool ELObject::process(const elpacket_t* recv, uint8_t* epcs) {
	// ESP_LOGI(___tag, "Profile: process");
	// GET
	p->packet_id = recv->packet_id;
	if (recv->esv == 0x62 || recv->esv == 0x61 || recv->esv == 0x60) {
		buffer_length = 0;
		recv->esv == 0x62 ? p->epc_count = get(epcs, recv->epc_count) : p->epc_count = set(epcs, recv->epc_count);

		if (recv->esv == 0x60) return false;

		p->src_device_class = _class_id;

		if (p->epc_count > 0) {
			p->esv = recv->esv + 0x10;
		} else {
			p->esv					 = recv->esv - 0x10;
			buffer[sizeof(elpacket_t) + 0] = 0;
			buffer_length				 = sizeof(elpacket_t) + 1;
		}

		// ESP_LOG_BUFFER_HEXDUMP(___tag, buffer, buffer_length, ESP_LOG_INFO);
		return true;
	}

	return false;
};

//// Profile

#undef ___tag
#define ___tag "EL Profile"

Profile::Profile() : ELObject(), profile{} {
	_instance = 1;
	_class_id = 0xf00e;	 // Profileオブジェクト

	profile[0x8a] = maker_code;
	profile[0x82] = new uint8_t[0x05]{0x04, 0x01, 0x0d, 0x01, 0x00};	 // 1.13 type 1
	profile[0x83] = new uint8_t[0x12]{0x11, 0xfe, 0x02, 0x03, 0x04,
							    0x05, 0x06, 0x07, 0x08, 0x09,
							    0x0a, 0x0b, 0x0c, 0x0d, 0x0e,
							    0x0f, 0x10, 0x11};			 // 識別ID
	profile[0xd6] = new uint8_t[0x05]{0x04, 0x01, 0x02, 0x7e, 0x01};	 // インスタンスリスト（1つ、EVPSだけ）
};

uint8_t Profile::set(uint8_t* epcs, uint8_t count) { return 0; }

uint8_t Profile::get(uint8_t* epcs, uint8_t count) {
	ESP_LOGI(___tag, "Profile: get %d", count);
	p->src_device_class = _class_id;
	p->src_device_id	= _instance;

	uint8_t* t = epcs;
	uint8_t* n = epc_start;
	uint8_t res_count;

	for (res_count = 0; res_count < count; res_count++) {
		uint8_t epc = t[0];
		uint8_t len = t[1];
		ESP_LOGI(___tag, "EPC 0x%02x [%d]", epc, len);
		t += 2;

		if (profile[epc] == nullptr) return 0;

		if (len > 0) {
			ESP_LOG_BUFFER_HEXDUMP(___tag, t, len, ESP_LOG_INFO);
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

