#include "adc1.hpp"

const char ADC1::TAG[] = "ADC1";

#define DEFAULT_VREF 1100

ADC1::ADC1(gpio_num_t pin, adc_channel_t channel, size_t sampling_count, adc_bits_width_t width, adc_atten_t atten) {
	gpio_config_t pin_config;
	pin_config.pin_bit_mask = (uint64_t)1 << pin;
	pin_config.mode	    = GPIO_MODE_INPUT;
	pin_config.pull_up_en   = GPIO_PULLUP_DISABLE;
	pin_config.pull_down_en = GPIO_PULLDOWN_ENABLE;
	pin_config.intr_type    = GPIO_INTR_DISABLE;
	gpio_config(&pin_config);

	this->channel		 = (adc1_channel_t)channel;
	this->sampling_count = sampling_count;

	adc1_config_width(width);
	adc1_config_channel_atten(this->channel, atten);

	/* esp_adc_cal_value_t val_type = */
	esp_adc_cal_characterize(unit, atten, width, DEFAULT_VREF, &chars);
};

uint32_t ADC1::get_voltage() {
	uint32_t adc_reading = 0;
	for (int i = 0; i < sampling_count; i++) adc_reading += adc1_get_raw(channel);
	adc_reading /= sampling_count;

	return esp_adc_cal_raw_to_voltage(adc_reading, &chars);
};
