// Our MODBUS presence is limited to two blocks of holding registers. 
// Address zero mirror the internal REGS and are read only.
// Other addresses are for the relay or sensors. For simplicity they all use the same address. 
// 
enum {
	SBC2022_MODBUS_REGISTER_RELAY = 100,
	SBC2022_MODBUS_REGISTER_SENSOR = 100,
};
enum {
	SBC2022_MODBUS_SLAVE_ADDRESS_RELAY = 16,
	SBC2022_MODBUS_SLAVE_ADDRESS_SENSOR_0 = 1,
	SBC2022_MODBUS_SLAVE_COUNT_SENSOR = 4,
};