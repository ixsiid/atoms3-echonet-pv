#pragma once
#include <driver/adc.h>
#include <esp_adc_cal.h>

class ADC1 {
    private:
	static const char TAG[8];

	static const adc_unit_t unit = ADC_UNIT_1;

	esp_adc_cal_characteristics_t chars;
	adc1_channel_t channel;

	size_t sampling_count;

    protected:

    public:
	ADC1(adc_channel_t channel, size_t sampling_count = 1, adc_bits_width_t width = ADC_WIDTH_BIT_12, adc_atten_t atten = ADC_ATTEN_DB_11);
	uint32_t get_voltage();
};
