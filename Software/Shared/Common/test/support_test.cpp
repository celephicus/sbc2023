#include "support_test.h"

static uint32_t l_mock_millis, l_mock_micros;

uint32_t millis() { return l_mock_millis; }
uint32_t micros() { return l_mock_micros; }

void support_test_set_millis(uint32_t m) { l_mock_millis = m; l_mock_micros = 0; }
