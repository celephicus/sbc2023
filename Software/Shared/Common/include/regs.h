#ifndef REGS_H__
#define REGS_H__

// Registers hold values of this type. It can be signed if required.
typedef uint16_t regs_t;
static const regs_t REGS_MAXVAL = 65535U;

#include "regs_local.h"

// Access registers as a block of memory somewhere else.
uint16_t* regsGetRegs();
#define REGS (regsGetRegs())

// Return flags register, this compiles to two lds instructions.
static inline regs_t& regsFlags() { return REGS[REGS_IDX_FLAGS]; }

// Set/clear all bits in the mask in the given register. Return true if value has changed.
bool regsWriteMask(uint8_t idx, regs_t mask, bool s);

// Write the register at index idx bits in mask m with value val. Obviously no bits are set in v that are clear in m. Return true if value has changed.
bool regsUpdateMask(uint8_t idx, regs_t mask, regs_t val);

// Set/clear all bits in the mask in the flags register. Return true if value has changed.
bool regsWriteMaskFlags(regs_t mask, bool s);

// Update bits in flags register with set bits in mask m with mask value. Return true if value has changed.
bool regsUpdateMaskFlags(regs_t mask, regs_t val);

// Print register value to console
void regsPrintValue(uint8_t reg_idx);

// Set all regs to default values.
void regsSetDefaultAll();

// Set just a range of registers _only_ to default values. Exposed just for the use case when a range of registers are used for something special,
//  like a set of menu options. Note that the non-NV regs will just be set to zero, which is unlikely to be useful.
void regsSetDefaultRange(uint8_t start, uint8_t end);

// Return the name of the register derived from the settings.local.h file.
const char* regsGetRegisterName(uint8_t idx);

// Return a help text derived from the settings.local.h file.
const char* regsGetRegisterDescription(uint8_t idx);

// Return an additional help string, contents generated in the settings.local.h file.
const char* regsGetHelpStr();

#endif // REGS_H__