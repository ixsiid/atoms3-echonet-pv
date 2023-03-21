#include "adc1.hpp"

const char ADC1::TAG[] = "ADC1";

#define DEFAULT_VREF 1100

ADC1::ADC1(adc_channel_t channel, size_t sampling_count, adc_bits_width_t width, adc_atten_t atten) {
	this->channel = (adc1_channel_t)channel;
	this->sampling_count = sampling_count;

	adc1_config_width(width);
	adc1_config_channel_atten(this->channel, atten);

	/* esp_adc_cal_value_t val_type = */
	esp_adc_cal_characterize(unit, atten, width, DEFAULT_VREF, &chars);
};

uint32_t ADC1::get_voltage() {
	uint32_t adc_reading = 0;
	for(int i=0; i<sampling_count; i++) adc_reading +=  adc1_get_raw(channel);
	adc_reading /= sampling_count;

	return esp_adc_cal_raw_to_voltage(adc_reading, &chars);
};
