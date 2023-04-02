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
#include <water-heater-object.hpp>
#include <power-distribution-object.hpp>

#include <adc1.hpp>

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
	},
					    "ButtonCheck", 2048, nullptr, 1, nullptr, 1);

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
	PV *pv		  = new PV(1);
	EVPS *evps	  = new EVPS(5);

	profile
	    ->add(pv)
	    ->add(evps);

	pv->get_cb = [](ELObject *obj, uint8_t epc, uint8_t len, uint8_t *value) {
		if (epc == 0xe1) ((PV*)obj)->update();
	};
	evps->get_cb = [](ELObject *obj, uint8_t epc, uint8_t len, uint8_t *value) {
		if (epc == 0xd6 || epc == 0xd8) ((EVPS*)obj)->update();
	};

	ADC1 * adc = new ADC1(gpio_num_t::GPIO_NUM_8, ADC_CHANNEL_7, 3);

	// 搭載ソーラーの気電圧と設置想定ソーラー出力の変換関数
	auto pv_volt_to_watt = [](uint32_t milli_volt){
		float R = 30.0f; // [ohm]
		float I = milli_volt / R; // [mA]
		float W = I * milli_volt / 1000.0f / 1000.0f; // [W]
		
		float k = 10000.0f;

		ESP_LOGI(TAG, "V: %d, I: %f, R: %f, W: %f, k: %f (%d)", milli_volt, I, R, W, W * k, (uint32_t)(W * k));
		return (uint32_t)(W * k);
	};

	Profile::el_packet_buffer_t packet_buffer;
	while (true) {
		uint32_t pv_watt = pv_volt_to_watt(adc->get_voltage());
		pv->update(pv_watt);
		evps->update_input_output(pv_watt);

		vTaskDelay(100 / portTICK_RATE_MS);
		profile->process_all_instance(udp, &packet_buffer);
	}
}
