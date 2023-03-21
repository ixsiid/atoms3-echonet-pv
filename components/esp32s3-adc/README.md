```cpp
#include <adc1.hpp>

app_main() {
	ADC1 * adc = new ADC1(ADC_CHANNEL_7, 3);

	while (true) {
		uint32_t v = adc->get_voltage();
		printf("voltage: %d [V]", v);
	}
}
```
