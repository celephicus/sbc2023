static bool console_cmds_user(char* cmd) {
  switch (console_hash(cmd)) {
	// MODBUS
    case /** SL **/ 0x74fa: modbusSetSlaveId(console_u_pop()); break;
    case /** ?SL **/ 0x79e5: consolePrint(CFMT_D, modbusGetSlaveId()); break;
    case /** SEND-RAW **/ 0xf690: {
        uint8_t* d = (uint8_t*)console_u_pop(); uint8_t sz = *d; modbusSendRaw(d + 1, sz);
      } break;
    case /** SEND **/ 0x76f9: {
        uint8_t* d = (uint8_t*)console_u_pop(); uint8_t sz = *d; modbusMasterSend(d + 1, sz);
      } break;
    case /** RELAY **/ 0x1da6: { // (state relay slave -- )      
        uint8_t slave_id = console_u_pop();
        uint8_t rly = console_u_pop();
        modbusRelayBoardWrite(slave_id, rly, console_u_pop());
      } break;

    default: return false;
  }
  return true;
}

