/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include "esp_log.h"
#include "nvs_flash.h"

#include <M5GFX.h>

#include <button.hpp>
#include <wifiManager.hpp>
#include <udp-socket.hpp>
#include <pv-object.hpp>

#include "wifi_credential.h"
/**
#define SSID "your_ssid"
#define WIFI_PASSWORD "your_wifi_password"
 */

#define tag "PV"

M5GFX display;

extern "C" {
void app_main();
}

void app_main(void) {
	ESP_LOGI(tag, "Start");
	esp_err_t ret;

	/* Initialize NVS — it is used to store PHY calibration data */
	ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);

	display.init();
	display.startWrite();

	display.setBrightness(64);
	display.setColorDepth(lgfx::v1::color_depth_t::rgb565_2Byte);
	display.fillScreen(TFT_GREENYELLOW);

	// ATOMS3
	const uint8_t buttonPins[] = {41};
	static Button *button = new Button(buttonPins, sizeof(buttonPins));

	xTaskCreatePinnedToCore([](void *_) {
		while (true) {
			// Main loop
			vTaskDelay(100 / portTICK_RATE_MS);

			button->check(nullptr, [&](uint8_t pin) {
				switch (pin) {
					case 41:
						ESP_LOGI(tag, "released gpio 41");
						break;
				}
			});
		}
	}, "ButtonCheck", 2048, nullptr, 1, nullptr, 1);

	vTaskDelay(3 * 1000 / portTICK_PERIOD_MS);

	ret = WiFi::Connect(SSID, WIFI_PASSWORD);
	if (!ret)
		ESP_LOGI("SBC", "IP: %s", WiFi::get_address());
	else
		display.fillScreen(TFT_RED);

	UDPSocket *udp = new UDPSocket();

	esp_ip_addr_t _multi;
	_multi.type		   = ESP_IPADDR_TYPE_V4;
	_multi.u_addr.ip4.addr = ipaddr_addr(EL_MULTICAST_IP);

	if (udp->beginReceive(EL_PORT)) {
		ESP_LOGI(tag, "EL.udp.begin successful.");
	} else {
		ESP_LOGI(tag, "Reseiver udp.begin failed.");	 // localPort
	}

	if (udp->beginMulticastReceive(&_multi)) {
		ESP_LOGI(tag, "EL.udp.beginMulticast successful.");
	} else {
		ESP_LOGI(tag, "Reseiver EL.udp.beginMulticast failed.");  // localPort
	}

	int packetSize;

	PV *pv = new PV(1);

	Profile *profile = new Profile(pv);

	/*
	evps->set_update_mode_cb([](EVPS_Mode current_mode, EVPS_Mode request_mode) {
		EVPS_Mode next_mode;
		switch (request_mode) {
			case EVPS_Mode::Charge:
				next_mode = EVPS_Mode::Charge;
				break;
			case EVPS_Mode::Stanby:
			case EVPS_Mode::Stop:
			case EVPS_Mode::Auto:
				next_mode = EVPS_Mode::Stanby;
				break;
			default:
				return EVPS_Mode::Unacceptable;
		}

		// モード繊維がないため、何もしない
		if (current_mode == next_mode) return current_mode;

		return next_mode;
	});
	*/

	uint8_t rBuffer[EL_BUFFER_SIZE];
	elpacket_t *p = (elpacket_t *)rBuffer;
	uint8_t *epcs = rBuffer + sizeof(elpacket_t);

	esp_ip_addr_t remote_addr;
	// esp_ip_addr_t multi_addr;
	// multi_addr.u_addr.ip4.addr = esp_ip4addr_aton(EL_MULTICAST_IP);
	// ESP_LOGI("Multicast addr", "%x", multi_addr.u_addr.ip4.addr);

	while (true) {
		vTaskDelay(100 / portTICK_PERIOD_MS);

		packetSize = udp->read(rBuffer, EL_BUFFER_SIZE, &remote_addr);

		if (packetSize > 0) {
			if (epcs[0] == 0xda || true) {
				ESP_LOGI("EL Packet", "(%04x, %04x) %04x-%02x -> %04x-%02x: ESV %02x [%02x]",
					    p->_1081, p->packet_id,
					    p->src_device_class, p->src_device_id,
					    p->dst_device_class, p->dst_device_id,
					    p->esv, p->epc_count);
				ESP_LOG_BUFFER_HEXDUMP("EL Packet", epcs, packetSize - sizeof(elpacket_t), ESP_LOG_INFO);
			}

			uint8_t epc_res_count = 0;
			switch (p->dst_device_class) {
				case 0xf00e:
					epc_res_count = profile->process(p, epcs);
					if (epc_res_count > 0) {
						profile->send(udp, &remote_addr);
						continue;
					}
					break;
				case 0x7902: // PV
					epc_res_count = pv->process(p, epcs);
					if (epc_res_count > 0) {
						pv->send(udp, &remote_addr);
						continue;
					}
					break;
				default:
					ESP_LOGE("EL Packet", "Unknown class access: %hx", p->dst_device_class);
					break;
			}
		}
	}
}
