static bool console_cmds_user(char* cmd) {
  switch (console_hash(cmd)) {
	case /** ?VER **/ 0xc33b: print_banner(); break;

	// Driver
    case /** LED **/ 0xdc88: driverSetLedPattern(console_u_pop()); break;
    case /** RLY **/ 0x07a2: driverRelayWrite(console_u_pop()); break;
    case /** ?RLY **/ 0xb21d: consolePrint(CFMT_D, driverRelayRead()); break;

    default: return false;
  }
  return true;
}

