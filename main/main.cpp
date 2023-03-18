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

#include <button.hpp>
#include <wifiManager.hpp>
#include <udp-socket.hpp>
#include <pv-object.hpp>
#include <evps-object.hpp>
#include <battery-object.hpp>

#include "wifi_credential.h"
/**
static const char SSID[] = "your_ssid";
static const char WIFI_PASSWORD[] = "your_wifi_password";
 */

extern "C" {
void app_main();
}

void app_main(void) {
	static const char TAG[] = "PV";
	ESP_LOGI(TAG, "Start");

	esp_err_t ret;

	/* Initialize NVS — it is used to store PHY calibration data */
	ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);

	// ATOMS3
	const uint8_t buttonPins[] = {41};
	static Button *button	  = new Button(buttonPins, sizeof(buttonPins));

	xTaskCreatePinnedToCore([](void *_) {
		while (true) {
			// Main loop
			vTaskDelay(100 / portTICK_RATE_MS);

			button->check(nullptr, [&](uint8_t pin) {
				switch (pin) {
					case 41:
						ESP_LOGI(TAG, "released gpio 41");
						break;
				}
			});
		}
	}, "ButtonCheck", 2048, nullptr, 1, nullptr, 1);

	vTaskDelay(3 * 1000 / portTICK_PERIOD_MS);

	ret = WiFi::Connect(SSID, WIFI_PASSWORD);
	ESP_ERROR_CHECK(ret);

	ESP_LOGI(TAG, "IP: %s", WiFi::get_address());

	UDPSocket *udp = new UDPSocket();

	esp_ip_addr_t _multi;
	_multi.type		   = ESP_IPADDR_TYPE_V4;
	_multi.u_addr.ip4.addr = ipaddr_addr(ELConstant::EL_MULTICAST_IP);

	if (udp->beginReceive(ELConstant::EL_PORT)) {
		ESP_LOGI(TAG, "EL.udp.begin successful.");
	} else {
		ESP_LOGI(TAG, "Reseiver udp.begin failed.");	 // localPort
	}

	if (udp->beginMulticastReceive(&_multi)) {
		ESP_LOGI(TAG, "EL.udp.beginMulticast successful.");
	} else {
		ESP_LOGI(TAG, "Reseiver EL.udp.beginMulticast failed.");  // localPort
	}


	Profile *profile = new Profile(1, 13);

	PV *pv = new PV(1);

	Battery *battery = new Battery(4);

	profile
	    ->add(pv)
	    ->add(battery);

	uint8_t rBuffer[ELConstant::EL_BUFFER_SIZE];
	ELObject::elpacket_t *p = (ELObject::elpacket_t *)rBuffer;
	uint8_t *epcs		    = rBuffer + sizeof(ELObject::elpacket_t);

	esp_ip_addr_t remote_addr;

	const int pv_reset_count = 6;
	int pv_update_count		= pv_reset_count;

	pv->update(1000);
	battery->update(1000);
	battery->set_cb = [](ELObject * obj, uint8_t epc, uint8_t len, uint8_t*current, uint8_t*request){
		ESP_LOGI(TAG, "Set callback %d, %d %d, %d", epc, len, current[0], request[0]);
		return ELObject::SetRequestResult::Reject;
	};
	battery->get_cb = [](ELObject * obj, uint8_t epc, uint8_t len, uint8_t * current) {
		if (epc == 0xa8) {
			ESP_LOGE(TAG, "Battery update");
			((Battery *)obj)->update();
		}
	};

	int packetSize;
	while (true) {
		vTaskDelay(100 / portTICK_PERIOD_MS);
		if (--pv_update_count < 0) {
			pv_update_count = pv_reset_count;
			pv->update();
		}

		packetSize = udp->read(rBuffer, ELConstant::EL_BUFFER_SIZE, &remote_addr);

		if (packetSize > 0) {
			if (epcs[0] == 0xda || true) {
				ESP_LOGI("EL Packet", "(%04x, %04x) %04x-%02x -> %04x-%02x: ESV %02x [%02x]",
					    p->_1081, p->packet_id,
					    p->src_device_class, p->src_device_id,
					    p->dst_device_class, p->dst_device_id,
					    p->esv, p->epc_count);
				ESP_LOG_BUFFER_HEXDUMP("EL Packet", epcs, packetSize - sizeof(ELObject::elpacket_t), ESP_LOG_INFO);
			}

			uint8_t epc_res_count = 0;
			switch (p->dst_device_class) {
				case Profile::class_u16:
					epc_res_count = profile->process(p, epcs);
					if (epc_res_count > 0) {
						profile->send(udp, &remote_addr);
						continue;
					}
					break;
				case PV::class_u16:
					epc_res_count = pv->process(p, epcs);
					if (epc_res_count > 0) {
						pv->send(udp, &remote_addr);
						continue;
					}
					break;
				case Battery::class_u16:
					epc_res_count = battery->process(p, epcs);
					if (epc_res_count > 0) {
						battery->send(udp, &remote_addr);
						continue;
					}
					break;
				default:
					ESP_LOGE(TAG, "Unknown class access: %hx", p->dst_device_class);
					break;
			}
		}
	}
}
