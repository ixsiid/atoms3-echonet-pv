#include <esp_log.h>
#include <esp_netif_ip_addr.h>
#include "pv-object.hpp"

//// PV

#undef ___tag
#define ___tag "EL PV"

PV::PV(uint8_t instance) : ELObject(instance, 0x7902), pv{} {
	update_mode_cb	 = nullptr;

	//// スーパークラス
	// 設置場所
	pv[0x81] = new uint8_t[0x02]{0x01, 0b01111101}; // その他
	// 規格Version情報
	pv[0x82] = new uint8_t[0x05]{0x04, 0x00, 0x00, 'Q', 0x01};  // Qでは登録不可
	// 異常発生状態
	pv[0x88] = new uint8_t[0x02]{0x01, 0x42};	// 異常なし
	// メーカコード
	pv[0x8a] = maker_code;
	// 状態変更通知アナウンスマップ
	pv[0x9d] = new uint8_t[0x03]{0x02, 0x80, 0xb1};
	// Setプロパティマップ
	pv[0x9e] = new uint8_t[0x04]{0x03, 0xa0, 0xa1, 0xc1};
	// Getプロパティマップ
	pv[0x9f] = new uint8_t[0x12]{0x11, 0x14,
						     // fedcba98
							 0b01101101,
							 0b01111100,
							 0b00011100,
							 0b00010001,
							 0b00011000,
							 0b00000000,  // 0x_5
							 0b00000000,
							 0b00000010,
							 0b01000010,
							 0b00000000,
							 0b00000000,  // 0x_a
							 0b00000000,
							 0b00000000,
							 0b00000000,
							 0b00000000,
							 0b00000000};

	// ^^^^

	pv[0x80] = new uint8_t[0x02]{0x01, 0x30};
	pv[0x83] = new uint8_t[0x12]{0x11, 0xfe, 0x10, 0x0f, 0x0e, 0x0d,
	                                         0x0c, 0x0b, 0x0a, 0x09,
	                                         0x08, 0x07, 0x06, 0x05,
	                                         0x04, 0x03, 0x02, 0x01};
	
	pv[0x97] = new uint8_t[0x03]{0x02, 0x0a, 0x12}; // 時刻
	pv[0x98] = new uint8_t[0x05]{0x04, 0x30, 0x01, 0x02, 0x03}; // 年月日
	
	pv[0xa0] = new uint8_t[0x02]{0x01, 0x32}; // 出力制御50％
	pv[0xa1] = new uint8_t[0x03]{0x02, 0x30, 0x00}; // 出力制御12.3kW
	pv[0xa2] = new uint8_t[0x02]{0x01, 0x41}; // 余剰買取有効
	
	pv[0xb0] = new uint8_t[0x65]{};
	pv[0xb0][0] = 0x64;
	pv[0xb1] = new uint8_t[0x08]{0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	pv[0xb2] = new uint8_t[0x02]{0x01, 0x42};
	pv[0xb4] = new uint8_t[0x03]{0x02, 0x75, 0x30};
	
	pv[0xc1] = new uint8_t[0x02]{0x01, 0x43};
	pv[0xc2] = new uint8_t[0x02]{0x01, 0x41};
	pv[0xc3] = new uint8_t[0x03]{0x02, 0xff, 0xff};
	pv[0xc4] = new uint8_t[0x02]{0x01, 0x32};
	
	pv[0xd0] = new uint8_t[0x02]{0x01, 0x00};
	pv[0xd1] = new uint8_t[0x02]{0x01, 0x44};
	
	pv[0xe0] = new uint8_t[0x03]{0x02, 0x00, 0x30};
	pv[0xe1] = new uint8_t[0x05]{0x04, 0x00, 0x00, 0x00, 0x01};
	pv[0xe8] = new uint8_t[0x03]{0x02, 0x75, 0x30};
};

uint8_t PV::set(uint8_t* epcs, uint8_t count) {
	// ESP_LOGI(___tag, "PV: get %d", count);
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

		if (pv[epc] == nullptr) return 0;

		// ここをほんとはコールバックをしたい
		switch(epc) {
			case 0xda: // 運転モード
				if (update_mode_cb) {
					PV_Mode new_mode = update_mode_cb((PV_Mode)pv[epc][1], (PV_Mode)*t);
					if (new_mode == PV_Mode::Unacceptable) return 0;

					pv[epc][1] = (uint8_t)new_mode;
				} else {
					pv[epc][1] = *t;
				}
				break;
			default:
				memcpy(&(pv[epc][1]), t, len);
				break;
		}

		if (len > 0) {
			ESP_LOG_BUFFER_HEXDUMP(___tag, t, len, ESP_LOG_INFO);
			t += len;
		}

		// メモリ更新後の値を返却する
		*n = epc;
		n++;
		memcpy(n, pv[epc], pv[epc][0] + 1);
		n += pv[epc][0] + 1;
	}

	buffer_length = sizeof(elpacket_t) + (n - epc_start);

	return res_count;
}

uint8_t PV::get(uint8_t* epcs, uint8_t count) {
	ESP_LOGI(___tag, "PV: get %d", count);
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

		if (pv[epc] == nullptr) return 0;

		if (len > 0) {
 			ESP_LOG_BUFFER_HEXDUMP(___tag, t, len, ESP_LOG_INFO);
			t += len;
		}

		*n = epc;
		n++;
		memcpy(n, pv[epc], pv[epc][0] + 1);
		n += pv[epc][0] + 1;
	}

	buffer_length = sizeof(elpacket_t) + (n - epc_start);

	return res_count;
};

void PV::set_update_mode_cb(update_mode_pv_cb_t cb) {
	update_mode_cb = cb;
};

void PV::notify_mode() {
	p->epc_count = 1;
	p->esv = 0x74; // ESV_INFC
	epc_start[0] = 0xda;
	epc_start[1] = pv[0xda][0];
	epc_start[2] = pv[0xda][1];
	buffer_length = sizeof(elpacket_t) + 3;
};
