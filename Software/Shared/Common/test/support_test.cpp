#include "support_test.h"

// We keep a fake count of micros in 32 bits, range about 70 mins. From this we compute millis.
static uint32_t l_fake_micros;
static uint32_t micros_ovf;

uint32_t millis() { return l_fake_micros / 1000U; }
uint32_t micros() { return l_fake_micros; }

void support_test_set_millis(uint32_t m) {
	l_fake_micros = m * 1000U;
	micros_ovf = 0;
}

void support_test_add_micros(uint32_t inc) {
	uint32_t old_fake_micros = l_fake_micros;
	l_fake_micros += inc;
	if (l_fake_micros < old_fake_micros)
		micros_ovf += 1;
}
