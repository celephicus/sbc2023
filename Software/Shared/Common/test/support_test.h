#ifndef SUPPORT_TEST_H__
#define SUPPORT_TEST_H__

#include <stdint.h>

uint32_t millis();
uint32_t micros();
void support_test_set_millis(uint32_t m=0);
void support_test_add_micros(uint32_t inc);

# endif  	// SUPPORT_TEST_H__
