
class Transport {
public:
	int8_t error;
	void begin() { error = 0; }
	int8_t readMany(void* buf, uint8_t r_addr, uint8_t num);
	int8_t writeMany(uint8_t r_addr, const void* buf, uint8_t num);
};

class Transport_SPI : Transport {
private:
	uint8_t _cs_pin;
public:
	Transport_SPI(uint8_t cs_pin) : _cs_pin(cs_pin) {};
	void begin();
	void readMany(void* buf, uint8_t r_addr, uint8_t num);
	void writeMany(uint8_t r_addr, const void* buf, uint8_t num);
};
void Transport_SPI::begin() {
	Transport::begin();
	SPI.begin();
	SPI.setDataMode(SPI_MODE3);
	digitalWrite(_cs_pin, HIGH);
	pinMode(_cs_pin, OUTPUT);
}
void Transport_SPI::readMany(void* buf, uint8_t r_addr, uint8_t num) {
	uint8_t* b = (uint8_t*)buf;
	digitalWrite(_cs_pin, LOW);
	SPI.transfer(r_addr | ((num > 1) ? 0xc0 : 0x80));	// Set b7 for read, b6 for multi-byte.
	while (num-- > 0) 
		*b++ = SPI.transfer(0x00);
	digitalWrite(_cs_pin, HIGH);
}
void Transport_SPI::writeMany(uint8_t r_addr, const void* buf, uint8_t num) {
	const uint8_t* b = (const uint8_t*)buf;
	digitalWrite(_cs_pin, LOW);
	SPI.transfer(r_addr);
	while (num-- > 0) 
		SPI.transfer(*b++);
	digitalWrite(_cs_pin, HIGH);
}

#include <Wire.h>
class Transport_I2C : Transport {
private:
	uint8_t _i2c_address;
public:
	Transport_I2C(uint8_t i2c_address) : _i2c_address(i2c_address) {};
	void begin();
	void readMany(void* buf, uint8_t r_addr, uint8_t num);
	void writeMany(uint8_t r_addr, const void* buf, uint8_t num);
};
void Transport_I2C::begin() {
	Transport::begin();
}
void Transport_I2C::readMany(void* buf, uint8_t r_addr, uint8_t num) {
	uint8_t* b = (uint8_t*)buf;
	Wire.beginTransmission(_i2c_address);  
	Wire.write(r_addr);             
	Wire.endTransmission();         

	Wire.beginTransmission(_i2c_address); 
	Wire.requestFrom(_i2c_address, num);  // Stupid Arduino lib needs to know count befire starting the read. 
	while(Wire.available()) {
		if (num > 0) {
			*b++ = Wire.read();
			num -= 1;
		}
	}
	if (num > 0)
		error = 1;
	Wire.endTransmission();         	
}
void Transport_I2C::writeMany(uint8_t r_addr, const void* buf, uint8_t num) {
	const uint8_t* b = (const uint8_t*)buf;
	Wire.beginTransmission(_i2c_address); 
	Wire.write(r_addr);             
	while (num-- > 0) 
		Wire.write(*b++);                 
	Wire.endTransmission();         
}
	
class ADXL345_ {
private:
	Transport& _transport;

public:
	ADXL345_(Transport& transport) : _transport(transport) {};
	
	// Initialise the driver, does no writes to the chip.
	void begin();
	
	// Write one byte to a register. 
	void write(uint8_t r_addr, uint8_t val);

	// Read one byte from a register. 
	uint8_t read(uint8_t r_addr);

	// Read a set of values, starting at a register. 
	void readMany(void* buf, uint8_t r_addr, uint8_t num);
	
	// Set/get bit states. Note that these take masks, not bit positions.
	void setRegisterMask(uint8_t r_addr, uint8_t mask, uint8_t state);
	uint8_t getRegisterMask(uint8_t r_addr, uint8_t mask);  

	// Access error indicator for transport. Never set to zero, only written non-zero on errors.
	int8_t& error() { return _transport.error; }
	
public:
	enum {
		REG_DEVID			= 0x00,		// Device ID, set to 0xE5. Just a magic number.
		REG_RESERVED1		= 0x01,		// Reserved. Do Not Access. 
		REG_THRESH_TAP		= 0x1D,		// Tap Threshold, unsigned 8 bits. 
		REG_OFSX			= 0x1E,		// X-Axis Offset, signed 8 bits. 
		REG_OFSY			= 0x1F,		// Y-Axis Offset, signed 8 bits.
		REG_OFSZ			= 0x20,		// Z-Axis Offset, signed 8 bits.
		REG_DUR				= 0x21,		// Tap Duration threshold, 160ms F.S.
		REG_LATENT			= 0x22,		// Tap Latency for double tap, 320ms F.S.
		REG_WINDOW			= 0x23,		// Tap Window for double tap, 320ms F.S.
		REG_THRESH_ACT		= 0x24,		// Activity Threshold
		REG_THRESH_INACT	= 0x25,		// Inactivity Threshold
		REG_TIME_INACT		= 0x26,		// Inactivity Time, 255s F.S.
		REG_ACT_INACT_CTL	= 0x27,		// Axis Enable Control for Activity and Inactivity Detection. [ACT_(AC | X | Y | Z) | INACT_(AC | X | Y | Z)]
		REG_THRESH_FF		= 0x28,		// Free-Fall Threshold.
		REG_TIME_FF			= 0x29,		// Free-Fall Time.
		REG_TAP_AXES		= 0x2A,		// Axis Control for Tap/Double Tap.
		REG_ACT_TAP_STATUS	= 0x2B,		// Source of Tap/Double Tap {-|-|-|-|SUPPRESS|X_EN|Y_EN|Z_EN]
		REG_BW_RATE			= 0x2C,		// Data Rate and Power mode Control
		REG_POWER_CTL		= 0x2D,		// Power-Saving Features Control
		REG_INT_ENABLE		= 0x2E,		// Interrupt Enable Control
		REG_INT_MAP			= 0x2F,		// Interrupt Mapping Control
		REG_INT_SOURCE		= 0x30,		// Source of Interrupts
		REG_DATA_FORMAT		= 0x31,		// Data Format Control
		REG_DATAX0			= 0x32,		// X-Axis Data LSB
		REG_DATAX1			= 0x33,		// X-Axis Data MSB
		REG_DATAY0			= 0x34,		// Y-Axis Data LSB
		REG_DATAY1			= 0x35,		// Y-Axis Data MSB
		REG_DATAZ0			= 0x36,		// Z-Axis Data LSB
		REG_DATAZ1			= 0x37,		// Z-Axis Data MSB
		REG_FIFO_CTL		= 0x38,		// FIFO Control
		REG_FIFO_STATUS		= 0x39,		// FIFO Status
	};

	enum {
		BIT_REG_ACT_INACT_CTL_ACT_AC_EN = 7,
		BIT_REG_ACT_INACT_CTL_ACT_X_EN = 6,
		BIT_REG_ACT_INACT_CTL_ACT_Y_EN = 5,
		BIT_REG_ACT_INACT_CTL_ACT_Z_EN = 4,
		BIT_REG_ACT_INACT_CTL_INACT_AC_EN = 7,
		BIT_REG_ACT_INACT_CTL_INACT_X_EN = 6,
		BIT_REG_ACT_INACT_CTL_INACT_Y_EN = 5,
		BIT_REG_ACT_INACT_CTL_INACT_Z_EN = 4,
	};

	enum {
		BIT_REG_TAP_AXES_SUPPRESS = 3,
		BIT_REG_TAP_AXES_X_EN = 2,
		BIT_REG_TAP_AXES_Y_EN = 1,
		BIT_REG_TAP_AXES_Z_EN = 0,
	};

	enum {
		BIT_REG_ACT_TAP_STATUS_ACT_X_SRC = 6,
		BIT_REG_ACT_TAP_STATUS_ACT_Y_SRC = 5,
		BIT_REG_ACT_TAP_STATUS_ACT_Z_SRC = 4,
		BIT_REG_ACT_TAP_STATUS_ASLEEP = 3,
		BIT_REG_ACT_TAP_STATUS_TAP_X_SRC = 2,
		BIT_REG_ACT_TAP_STATUS_TAP_Y_SRC = 1,
		BIT_REG_ACT_TAP_STATUS_TAP_Z_SRC = 0,
	};

	enum {
		BIT_REG_BW_RATE_LOW_POWER = 4,
		VAL_REG_BW_RATE_BW_1600	=		0xF,			// 1111		IDD = 40uA
		VAL_REG_BW_RATE_BW_800 =		0xE,			// 1110		IDD = 90uA
		VAL_REG_BW_RATE_BW_400 =		0xD,			// 1101		IDD = 140uA
		VAL_REG_BW_RATE_BW_200 =		0xC,			// 1100		IDD = 140uA
		VAL_REG_BW_RATE_BW_100 =		0xB,			// 1011		IDD = 140uA 
		VAL_REG_BW_RATE_BW_50 =			0xA,			// 1010		IDD = 140uA
		VAL_REG_BW_RATE_BW_25 =			0x9,			// 1001		IDD = 90uA
		VAL_REG_BW_RATE_BW_12_5 =	    0x8,			// 1000		IDD = 60uA 
		VAL_REG_BW_RATE_BW_6_25 =		0x7,			// 0111		IDD = 50uA
		VAL_REG_BW_RATE_BW_3_13 =		0x6,			// 0110		IDD = 45uA
		VAL_REG_BW_RATE_BW_1_56 =		0x5,			// 0101		IDD = 40uA
		VAL_REG_BW_RATE_BW_0_78 =		0x4,			// 0100		IDD = 34uA
		VAL_REG_BW_RATE_BW_0_39 =		0x3,			// 0011		IDD = 23uA
		VAL_REG_BW_RATE_BW_0_20 =		0x2,			// 0010		IDD = 23uA
		VAL_REG_BW_RATE_BW_0_10 =		0x1,			// 0001		IDD = 23uA
		VAL_REG_BW_RATE_BW_0_05 =		0x0,			// 0000		IDD = 23uA
	};

	enum {
		BIT_REG_POWER_CTL_LINK = 5,
		BIT_REG_POWER_CTL_AUTO_SLEEP = 4,
		BIT_REG_POWER_CTL_MEASURE = 3,
		BIT_REG_POWER_CTL_SLEEP = 2,
		VAL_REG_POWER_CTL_WAKEUP_FREQ_8 = 0, 
		VAL_REG_POWER_CTL_WAKEUP_FREQ_4 = 1, 
		VAL_REG_POWER_CTL_WAKEUP_FREQ_2 = 2, 
		VAL_REG_POWER_CTL_WAKEUP_FREQ_1 = 3, 
	};

	// Apply to REG_INT_ENABLE, REG_INT_MAP, REG_INT_SOURCE
	enum {
		BIT_REG_INT_XXX_DATA_READY = 7,
		BIT_REG_INT_XXX_SINGLE_TAP = 6,
		BIT_REG_INT_XXX_DOUBLE_TAP = 5,
		BIT_REG_INT_XXX_ACTIVITY = 4,
		BIT_REG_INT_XXX_INACTIVITY = 3,
		BIT_REG_INT_XXX_FREE_FALL = 2,
		BIT_REG_INT_XXX_WATERMARK = 1,
		BIT_REG_INT_XXX_OVERRUN = 0,
	};

	enum {
		BIT_REG_DATA_FORMAT_SELF_TEST = 7,
		BIT_REG_DATA_FORMAT_SPI = 6,
		BIT_REG_DATA_FORMAT_INT_INVERT = 5,
		BIT_REG_DATA_FORMAT_FULL_RES = 3,
		BIT_REG_DATA_FORMAT_JUSTIFY = 2,
		VAL_REG_DATA_FORMAT_RANGE_2G = 0,
		VAL_REG_DATA_FORMAT_RANGE_4G = 1,
		VAL_REG_DATA_FORMAT_RANGE_8G = 2,
		VAL_REG_DATA_FORMAT_RANGE_16G = 3,
	};

	enum {
		VAL_REG_FIFO_CTL_MODE_BYPASS = 0<<6,
		VAL_REG_FIFO_CTL_MODE_FIFO = 1<<6,
		VAL_REG_FIFO_CTL_MODE_STREAM = 2<<6,
		VAL_REG_FIFO_CTL_MODE_TRIGGER = 3<<6,
		BIT_REG_FIFO_CTL_TRIGGER = 5,
		MASK_REG_FIFO_CTL_SAMPLES = 0x1f, // Values range from 0 to 31. 
	};

	enum {
		BIT_REG_FIFO_STATUS_FIFO_TRIG = 7,
		MASK_REG_FIFO_STATUS_ENTRIES = 0x3f, // Values range from 0 to 31. 
	};
};

void ADXL345_::begin() {
	_transport.begin();
}
uint8_t ADXL345_::read(uint8_t r_addr) {
	uint8_t v;
	_transport.readMany(&v, r_addr, 1);
	return v; 
}

void ADXL345_::readMany(void* buf, uint8_t r_addr, uint8_t num) {
	_transport.readMany(buf, r_addr, num); 
}

void ADXL345_::write(uint8_t r_addr, uint8_t val) {
	_transport.writeMany(r_addr, &val, 1); 
}

void ADXL345_::setRegisterMask(uint8_t r_addr, uint8_t mask, uint8_t state) {
	uint8_t b = read(r_addr);
	write(r_addr, state ? (b|mask) : (b&~mask));  
}

uint8_t ADXL345_::getRegisterMask(byte r_addr, uint8_t mask) {
	return read(r_addr) & mask;
}

