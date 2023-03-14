#include <esp_log.h>
#include <esp_netif_ip_addr.h>
#include "battery-object.hpp"

//// Battery

#undef ___tag
#define ___tag "EL Battery"

Battery::Battery(uint8_t instance) : ELObject(instance, 0x7d02), battery{} {
	//// スーパークラス
	// 設置場所
	battery[0x81] = new uint8_t[0x02]{0x01, 0b01111101}; // その他
	// 規格Version情報
	battery[0x82] = new uint8_t[0x05]{0x04, 0x00, 0x00, 'Q', 0x01};  // Qでは登録不可
	// 異常発生状態
	battery[0x88] = new uint8_t[0x02]{0x01, 0x42};	// 異常なし
	// メーカコード
	battery[0x8a] = maker_code;
	// 状態変更通知アナウンスマップ
	battery[0x9d] = new uint8_t[0x08]{0x07, 0x80, 0xaa, 0xab, 0xc1, 0xc2, 0xcf, 0xda};
	// Setプロパティマップ
	battery[0x9e] = new uint8_t[0x04]{0x03, 0xaa, 0xab, 0xda};
	// Getプロパティマップ
	battery[0x9f] = new uint8_t[0x12]{0x11, 0x19,
						     // fedcba98
							 0b00000101,
							 0b00010100,
							 0b01010100,
							 0b01000101,
							 0b01000100,
							 0b00000100,  // 0x_5
							 0b01000000,
							 0b00000010,
							 0b00010110,
							 0b00010100,
							 0b00100100,  // 0x_a
							 0b00100100,
							 0b00000000,
							 0b00000000,
							 0b00000000,
							 0b00010000};

	// 固有クラス

	battery[0x80] = new uint8_t[0x02]{0x01, 0x30};
	battery[0x83] = new uint8_t[0x12]{0x11, 0xfe, maker_code[0], maker_code[1], maker_code[2], 0x0d,
	                                              0x0c, 0x0b, 0x0a, 0x09,
	                                              0x08, 0x07, 0x06, 0x05,
	                                              0x04, 0x03, 0x02, 0x04};
	
	battery[0x97] = new uint8_t[0x03]{0x02, 0x0a, 0x12}; // 時刻
	battery[0x98] = new uint8_t[0x05]{0x04, 0x30, 0x01, 0x02, 0x03}; // 年月日
	
	battery[0xa0] = new uint8_t[0x05]{0x04, 0x32, 0x00, 0x00, 0x00};
	battery[0xa1] = new uint8_t[0x05]{0x04, 0x30, 0x00, 0x00, 0x00};
	battery[0xa2] = new uint8_t[0x05]{0x04, 0x41, 0x00, 0x00, 0x00};
	battery[0xa3] = new uint8_t[0x05]{0x04, 0x32, 0x00, 0x00, 0x00};
	battery[0xa4] = new uint8_t[0x05]{0x04, 0x30, 0x00, 0x00, 0x00};
	battery[0xa5] = new uint8_t[0x05]{0x04, 0x41, 0x00, 0x00, 0x00};
	battery[0xa8] = new uint8_t[0x05]{0x04, 0x41, 0x00, 0x00, 0x00};
	battery[0xa9] = new uint8_t[0x05]{0x04, 0x32, 0x00, 0x00, 0x00};
	battery[0xaa] = new uint8_t[0x05]{0x04, 0x30, 0x00, 0x00, 0x00};
	battery[0xab] = new uint8_t[0x05]{0x04, 0x41, 0x00, 0x00, 0x00};
	
	battery[0xc1] = new uint8_t[0x02]{0x01, 0x43};
	battery[0xc2] = new uint8_t[0x02]{0x01, 0x41};
	battery[0xc8] = new uint8_t[0x09]{0x08, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	battery[0xc9] = new uint8_t[0x09]{0x08, 0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	battery[0xcf] = new uint8_t[0x02]{0x01, 0x32};
	
	battery[0xda] = new uint8_t[0x02]{0x01, 0x00};
	battery[0xdb] = new uint8_t[0x02]{0x01, 0x44};
	
	battery[0xe2] = new uint8_t[0x05]{0x04, 0x00, 0x00, 0x00, 0x30};
	battery[0xe3] = new uint8_t[0x03]{0x02, 0x00, 0x00};
	battery[0xe4] = new uint8_t[0x03]{0x02, 0x75, 0x30};
	battery[0xe6] = new uint8_t[0x03]{0x02, 0x75, 0x30};
};

uint8_t Battery::set(uint8_t* epcs, uint8_t count) {
	// ESP_LOGI(___tag, "Battery: get %d", count);
	p->src_device_class = class_group;
	p->src_device_id	= instance;

	uint8_t* t = epcs;
	uint8_t* n = epc_start;
	uint8_t res_count;

	for (res_count = 0; res_count < count; res_count++) {
		uint8_t epc = t[0];
		uint8_t len = t[1];
		ESP_LOGI(___tag, "EPC 0x%02x [%d]", epc, len);
		t += 2;

		if (battery[epc] == nullptr) return 0;

		switch(epc) {
			default:
				memcpy(&(battery[epc][1]), t, len);
				break;
		}

		if (len > 0) {
			ESP_LOG_BUFFER_HEXDUMP(___tag, t, len, ESP_LOG_INFO);
			t += len;
		}

		// メモリ更新後の値を返却する
		*n = epc;
		n++;
		memcpy(n, battery[epc], battery[epc][0] + 1);
		n += battery[epc][0] + 1;
	}

	buffer_length = sizeof(elpacket_t) + (n - epc_start);

	return res_count;
}

uint8_t Battery::get(uint8_t* epcs, uint8_t count) {
	ESP_LOGI(___tag, "Battery: get %d", count);
	p->src_device_class = class_group;
	p->src_device_id	= instance;

	uint8_t* t = epcs;
	uint8_t* n = epc_start;
	uint8_t res_count;

	for (res_count = 0; res_count < count; res_count++) {
		uint8_t epc = t[0];
		uint8_t len = t[1];
 		ESP_LOGD(___tag, "EPC 0x%02x [%d]", epc, len);
		t += 2;

		if (battery[epc] == nullptr) return 0;

		if (len > 0) {
 			ESP_LOG_BUFFER_HEXDUMP(___tag, t, len, ESP_LOG_INFO);
			t += len;
		}

		*n = epc;
		n++;
		memcpy(n, battery[epc], battery[epc][0] + 1);
		n += battery[epc][0] + 1;
	}

	buffer_length = sizeof(elpacket_t) + (n - epc_start);

	return res_count;
};

void Battery::notify_mode() {
	p->epc_count = 1;
	p->esv = 0x74; // ESV_INFC
	epc_start[0] = 0xda;
	epc_start[1] = battery[0xda][0];
	epc_start[2] = battery[0xda][1];
	buffer_length = sizeof(elpacket_t) + 3;
};
