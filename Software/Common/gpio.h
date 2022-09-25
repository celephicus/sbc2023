#ifndef GPIO_H__
#define GPIO_H__

#define CC(a, b) a##b
#define C(a, b) CC(a, b)
#define CC3(a, b, c) a##b##c
#define C3(a, b, c) CC3(a, b, c)

// Generate lots of inline functions to access a port pin.
#define GPIO_DECLARE_PIN_ACCESS_FUNCS(name_, port_, bit_) 																				\
 static inline void C3(gpio, name_, SetModeOutput)() { C(DDR, port_) |= _BV(bit_); }													\
 static inline void C3(gpio, name_, SetModeInput)() { C(DDR, port_) &= ~_BV(bit_); }													\
 static inline void C3(gpio, name_, SetMode)(bool fout) { if (fout) C(DDR, port_) |= _BV(bit_); else C(DDR, port_) &= ~_BV(bit_);}		\
 static inline bool C3(gpio, name_, Read)() { return !!(C(PIN, port_) | _BV(bit_)); }													\
 static inline void C3(gpio, name_, Toggle)() { C(PORT, port_) ^= _BV(bit_); }															\
 static inline void C3(gpio, name_, Set)() { C(PORT, port_) |= _BV(bit_); }																\
 static inline void C3(gpio, name_, Clear)() { C(PORT, port_) &= ~_BV(bit_); }															\
 static inline void C3(gpio, name_, Write)(bool b) { if (b) C(PORT, port_) |= _BV(bit_); else C(PORT, port_) &= ~_BV(bit_); }			\

// As for GPIO_DECLARE_PIN_ACCESS_FUNCS but with empty bodies.
#define GPIO_DECLARE_PIN_ACCESS_FUNCS_NULL(name_, port_, bit_) 																			\
 static inline void C3(gpio, name_, SetModeOutput)() {}																					\
 static inline void C3(gpio, name_, SetModeInput)() {}																					\
 static inline void C3(gpio, name_, SetMode)(bool fout) { (void)fout; }																	\
 static inline bool C3(gpio, name_, Read)() { return false; }																			\
 static inline void C3(gpio, name_, Toggle)() {}																						\
 static inline void C3(gpio, name_, Set)() {}																							\
 static inline void C3(gpio, name_, Clear)() {}																							\
 static inline void C3(gpio, name_, Write)(bool b) { (void)b; }																			\

#include "..\..\gpio.local.h"

#endif  // GPIO_H__
