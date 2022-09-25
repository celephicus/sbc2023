#ifndef REGS_LOCAL_H__
#define REGS_LOCAL_H__

// Define version of regs schema. If you change this file, increment the number to force any existing EEPROM to flag as corrupt.
// Also increment to force the default values to be set for testing. 
const uint16_t REGS_VERSION = 1;

// Extra NV stuff. 
typedef struct {
	/* empty */
} regs_extra_t;
void regsExtraSetDefaults()  { /* empty */ }

// Registers are defined with 3 parameters, name, default value, and a short comment that is turned into help text. 
#define REGS_DEFS(gen_)																														\
 gen_(FLAGS,						REGS_NO_DEFAULT,					"Various flags, see REGS_FLAGS_MASK_xxx")							\
 gen_(EEPROM_RC,					REGS_NO_DEFAULT,					"Last NV read status")												\
 gen_(RESTART,						REGS_NO_DEFAULT,					"Snapshot of MCUSR")												\
 gen_(ADC_12V_MON,					REGS_NO_DEFAULT,					"Raw ADC value DC power volts")										\
 gen_(CONFIG,						REGS_NO_DEFAULT,					"Config bits read from links at startup")							\
																																			\
 /* NV regs. */																																\
 gen_(ENABLES,						0,									"Enable flags.")													\

// Define the start of the NV regs. The region is from this index up to the end of the register array. 
#define REGS_START_NV_IDX REGS_IDX_ENABLES

// Define which regs to print in hex. Note that since we only have 16 bits of mask all hex flags must be in the first 16.
#define REGS_PRINT_HEX_MASK (_BV(REGS_IDX_FLAGS) | _BV(REGS_IDX_ENABLES))

// Flags are defined with 3 parameters, name, bit position and a short description that is turned into help text. 
#define REGS_FLAGS_DEFS(gen_)																							\
	gen_(DC_VOLTS_LOW,			0,	"External power volts low")															\
																														\
	/* Fault flags. */																									\
	gen_(WATCHDOG_RESTART,		15, "Whoops...")																		\

// Enables are defined with 3 parameters, name, bit position and a short description that is turned into help text. 
#define REGS_ENABLES_DEFS(gen_)																							\
 gen_(DUMP_REGS,		0,	"Regs values dump to console")																\
 gen_(DUMP_REGS_FAST,	1,	"Dump at 5/s rather than 1/s")																\

// Generate FLAGS defs.
//

// Macro that generates enum items intialised to a mask for a set of bit flags, SO gen_(FLAGS, 4,	"Stuff") => 'REGS_FLAGS_MASK_FLAGS = (int)_BV(4),'. 
#define REGS_FLAGS_GEN_ENUM_MASK(name_, bit_, desc_) \
 REGS_FLAGS_MASK_##name_ = (int)_BV(bit_),

// Declare the flags enum.
enum {
	REGS_FLAGS_DEFS(REGS_FLAGS_GEN_ENUM_MASK)
};

// Macro that expands to a help string for the flag, so gen_(FLAGS, 4,	"Stuff") => "\r\n FLAGS: 4: Stuff."
#define REGS_GENERIC_MASK_GEN_HELP_STR(name_, bit_, desc_) \
 "\r\n " #name_ ": " #bit_ " (" desc_ ".)"

// Generate ENABLES defs. Similar to FLAGS
//

#define REGS_ENABLES_GEN_ENUM_MASK(name_, bit_, desc_) \
 REGS_ENABLES_MASK_##name_ = (int)_BV(bit_),

// Declare the enables enum.
enum {
	REGS_ENABLES_DEFS(REGS_ENABLES_GEN_ENUM_MASK)
};

// Generate help string for flags & enables.
#define REGS_EXTRA_HELP_STR												\
 "\r\nFlags:" REGS_FLAGS_DEFS(REGS_GENERIC_MASK_GEN_HELP_STR)			\
 "\r\nEnables:" REGS_ENABLES_DEFS(REGS_GENERIC_MASK_GEN_HELP_STR)		\

#endif // REGS_LOCAL_H__

	