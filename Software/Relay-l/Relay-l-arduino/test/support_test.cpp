#include <stdint.h>
#include "support_test.h"

static uint32_t l_mock_millis;
uint32_t millis() { return l_mock_millis; }
void support_test_set_millis(uint32_t m) { l_mock_millis = m; }
