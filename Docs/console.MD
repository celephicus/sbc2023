# Console
Some documentation on the Console that is used in the Sargood system. It is a text-based "shell" similar to the DOS command line that allows you to type simple commands and to print results on a serial terminal.

## Connecting
First get a serial interface, these days it will be something *like* a FDTI module that has a USB interface to logic level serial converter IC on it. 

1. Make sure that the interface board is operating at the correct voltage, they usually have a switch or solder link to switch between 5V and 3.3V. It might damage the target board if you use 5V level on a 3.3V board. And a 5V board might not work reliably on a 3.3V target.
1. These have a 6 pin interface that connects to the 6 pin header on the various boards, make sure that the pin marked "GND" is connected to pin 1. 
1. Install a serial terminal on a PC. I use [Termite](https://www.compuphase.com/software_termite.htm) as it is easy to use, and has good help available.
1. Set your terminal to the serial port of your USB adaptor.
2. Set the terminal speed to 38400, 8 data bits, 1 stop bit, no parity, no hardware or software flow control.
3. Set the terminal to *send* a carriage return character on enter.
4. Set the terminal to print a carriage return then a newline on *receive*.
3. Open the connection to the target board.

***Coming soon: detailed instructions for Termite.***

Restart the target by turning it back off and then back on again. You should see a line of text back from the target like:
```
    TSA SBC2022 Relay Module V1.0 (1234) 20221110T105347
>
```
Type "enter" a few times, you should see a response like the following, this indicates that all is working.
```
> ->
> ->
```

## Commands and Arguments
The console allows you to type commands, which take arguments with may be numbers, text strings, or arbitrary data. The user types a set of commands and arguments, and then types "enter" to make the Console interpret them and print the result. 
Commands and numbers are case insensitive, you can use any combination of case you like. Strings handle case _exactly_ as typed. 
There is no editing or backspace, if you make a mistake your terminal program will have a line cancel function or in Termite you can edit the line before sending it.

Commands and arguments are simply seperated with one or more space characters. 

Arguments are stored on a stack as they are evaluated, and commands take as many arguments as they need from the stack. The Console will issue an error if the stack would overflow or if too many values are removed from it. This is similar to how the old "RPN" Hewlett-Packard calculators were used. As astack, the items are access from the last entered, ot top item on down. This takes some getting used to, but it does allow a very simple interpreter to be written.

To demonstrate, type a number, "1234", then enter. The Console will print:
```
> 1234 ->
>

```
The first '>' is a prompt, and what you have *sent* to the Console is echoed next. What is printed after the '->' is what the Console has *sent* back,in this case nothing as all you have done is entered a number. Now type a single dot ".". The result is:
```
> . -> 1234
> 
```
The "." is a Console command that prints the top stack item, and also removed it from the stack. Youcan combine commandsand arguments on one line, try typing "1234 .".

If you type a "." again the command will print an error, as the stack is empty.
```
> . -> Error stack underflow (2)
> 
```
The only other error that you might see is that for an unknown command, which the Console prints if it doesn't understand a command or an argument, which is similar to the above. 

## Argument Formats

###Numbers
The default word size of the console is 16 bits, so a number range of -32768 to 32767 inclusive for signed integers, 0 through 65535 for unsigned integers. You can copile it to use 32 bit words quite easily too if you want. 

Signed integers are simply entered as is, with a leading '-' for negative numbers. Any numbers that are out of range cause an error. I have found this range checking very useful to catch silly mistakes.

Unsigned integers are entered with a leading '+', and are similarly checked for range.

Hex numbers are entered with a leading '$', eg `$12af`, they are also range checked.

### Strings
Strings are entered with a leading '"', but with the wrinkle that they cannot contain a space, as this seperates the string into two tokens. But a space can be entered as an escape sequence `\20`. Other escape sequences are `\\`, `\r` & `\n`, and any character can be entered with a backslash followed by exactly 2 hex digits. The string inmemory is terminated with a nul character so it can be used as a regular "C" string, and the address is placed on the stack.

Note that the string is actually stored in the input buffer, so the data is only valid for the remaining commands on the line being processed. The next line entered will probably corrupt it. So strings must be used in the same line that they are entered. 

The string "Hello, world." would be entered as `"Hello,\20world.`.

### Data
Arbitrary data may be entered as a string of hex digits with a leading "#". Again the data is stored in the input buffer, so the command that uses it must follow the data on the input line.

So `#0603feff` results in 4 bytes in memory. 

## Commands
Commands are not like arguments, they do not add a value to the stack. They might pull values from the stack, add a value themselves, or do something else. Commands are a "word", a set of characters that is seperated from the next command or value by space. They can have any name, but the name cannot be mistaken for a value, so you cannot have a command like `12345678`, it looks like a decimal integer.

There are a few conventions, commands with a leading query e.g `?foo` print something, but in general commands have a variety of names. 

As commands may consume and add values to the stack, there is a simple way to document this as the "stack effect". This shows values on the stack before and after a command is run as an expression in brackets separated by two dashes, so `(before -- after )`. The part *before* the `--` describes the state of the stack before the execution of the command, i.e., the parameters that are passed into the command; the part *after* the `--` is the state of the stack after the execution of the command, i.e., the results of the command. The stack comment only shows the top stack items that the definition accesses and/or changes. The type of the items on the stack is also shown with a simple syntax.

E.g the print TOS command `.` has a stack effect `(d -- )`, meaning that it takes a single value representing a decimal number, and consumes it. The `HASH` command that takes a string and replaces it with the hash value (a number used to look up a string in a table that only has a small memory footprint)has an effect `($ -- u)`.

The types of item as as follows:

|Type   | Descrption           |
|-------|-----------------------|
| u      | Unsigned integer 16 bit |
| u8     | Unsigned integer 8 bit |
| d     | Signed integer |
| $     | Null-terminated string. |
| #     | Arbitrary binary data. |

Additionally the *meaning* of the items can be documented. E.g the `LIM` command has effect `(lower-limit:d upper-limit:d axis-idx:u8 -- )`. Generally this is only done for more than one stack item.


## Builtin Commands

A few commands are standard in the console, these are described below. Note that any argument from the stack is always removed. The abbreviation "TOS" refers to the value on top of the stack. Note that all printed values are followed by a single space. 

|Command|Description|
|-------|-----------|
| .     | (d -- ) Prints TOS as a signed decimal, so in the range -32768 through 32767 inclusive.                                                |
| U.    | (u -- ) Prints TOS as an unsigned decimal, with a leading '+', so in the range 0 through 65535 inclusive.                            | 
| $.    | (u -- ) Prints TOS as 4 hex digits with a leading '$'.                                                                             |
| ."    | ($ -- ) Prints TOS as a string.                                                                                                |
| DEPTH | ( -- u) Pushes the number of items on the stack _before_ this command was run.                                                      |
| CLEAR | ( -- ) Discards all values on the stack. Useful if you have lost track of what you have entered so far.                        |
| DROP  | (d -- ) Discards TOS, _the topmost item_ on the stack.                                                                              |
| HASH  | ($ -- u) Computes the 16 bit hash value of the string address on the stack and pushes it onto the stack.                             |

The HASH command is used only by programmers.

## Common Commands
Some commands are common to the Relay, Sensor and Controller units.

|Command|Description|
|-------|-----------|
| ?VER | ( -- ) Prints the name of the project, and a version number, build number and build date in ISO 8601 format. E.g `TSA SBC2022 Relay Module V1.0 (1234) 20221110T105347`. |
| LED | (u -- ) Sets the blink pattern on the LED to the index in TOS. Note that the system may well overwrite your setting, so you should disable this by other means. If the index is out of range an error is flagged. A valueof zero turns off the LED. |
| ?LED | ( -- ) Print the index of the current blink pattern on the LED. |
| M | ( -- ) Enable dumping MODBUS events. Use command `X` to disable. There are registers that further control MODBUS dumping. |
| ATN | ( -- ) Set the Bus ATN line active for a while. |
| SL | (u8 -- ) The value in TOS is used to set the MODBUS slave ID. This is not saved in non-volatile storage and is reloaded at each restart. Legal values are from 1 though 247 inclusive. Setting the slave ID to an illegal value will prevent that slave from responding to any MODBUS requests. |
| ?SL | ( -- ) Print the MODBUS slave ID. Legal values are from 1 though 247 inclusive. |
| SEND-RAW | (x -- ) Sends arbitrary data on the MODBUS. No CRC is appended to the data, it is sent exactly as given. Used for testing that corrupted MODBUS packets are rejected. |
| SEND | (x -- ) Sends arbitrary data on the MODBUS and appends a correct CRC. |
| RELAY | (state:u8 relay:u8 slave-id:u8 -- ) Writes to one of those cheap 8 way Ebay MODBUS boards. Value `slave-id` is the MODBUS slave ID set on the board. Value `relay` is the index of the relay, from 1 through 8 inclusive. Value `state` closes the relay for `1`, opens it for `2`, there are other values possible. |
| WRITE | (val:u16 reg:u16 slave-id:u8 -- ) Sends a MODBUS write single register request to a slave. Value `slave-id` is the MODBUS slave ID. Value `reg` is the MODBUS register address, value `val` is the value to write. Note that the only way to see the response (if any) is by dumping MODBUS responses |
| READ | (count:u16 reg:u16 slave-id:u8 -- ) Sends a MODBUS read multiple registers request to a slave. Value `slave-id` is the MODBUS slave ID. Value `reg` is the MODBUS register address to start, value `count` is the number of values to read. Note that the only way to see the response (if any) is by dumping MODBUS responses |
| ?V | (reg-idx:u8 -- ) Print the value of the register at index in TOS. The printing format may be signed decimal, unsigned decimal or hex depending on the register, the number will have appropriate prefix so that the format may be deduced. If the index is out of range an argument error is flagged. |
| V | (val:u16 reg-idx:u8 -- ) Set the value of the register at index in TOS to NOS. No check is made on the value. If the index is out of range an argument error is flagged. |
| ??V | ( -- ) Print the values of all registers on a single line. The printing format may be signed decimal, unsigned decimal or hex depending on the register, the number will have appropriate prefix so that the format may be deduced. |
| ???V | ( -- ) Print a verbose dump of all register values, together with descriptive text for each register and the meanings of all flags. See the registers writeup for more information. |
| DUMP | (u8 -- ) Stops/starts a periodic dump of all the registers that are not backed up into non-volatile storage. The idea is that this set of registers change frequently and to allow the developers to view them over time. The behaviour depends on TOS. If zero, register dump is stopped. If 1, they are dumped every second. If 2 or higher,they are dumped 5 times a second. |
| X | ( -- ) A shortcut for stopping register dump and MODBUS event dump. If there is too much output the outputcan be confusing so this command kills all dump. |
| RESTART | ( -- ) Runs an infinite loop so that the watchdog will not get patted, causing a restart. Can also be used to check that the watchdog is working. |
| CLI | ( -- ) Sets the processor I flag to prevent interrupts being serviced. This should cause a watchdog restart as part of the watchdog service schedule is done by the timer interrupt. |
| ABORT | (u8 -- ) Aborts the program with the runtime error in TOS. This error is only used during development to check for logic errors in the code. The command just verifies that the runtime error system is working. |
| ASSERT | (u16 -- ) Asserts that the value in TOS is true (non-zero). Asserts trigger a special runtime error and are used to check for logic errors in the code. They do not cause a restart, the program will print a message to the Console and then become unresponsive. |
| NV-DEFAULT | ( -- ) This command sets all non-volatile data to default values, but does not write the values to non-volatile storage. This will clear any changes that have been made but not saved to NV storage. For the Controller this will clear limits and presets. |
| NV-W | ( -- ) Write all values to non-volatile storage. This will overwrite any existing data in NV storage. |
| NV-R | ( -- ) Read all values from non-volatile storage. This will clear any changes that have been made but not saved to NV storage. |
| PWRITE | (pin-num:u8 state:b -- ) Writes the output pin given in value `pin-num` to boolean value `state`, directly via a call to digitalWrite(). E.g. `1 11 PWRITE` will set pin 11 high. Note that the system may also write the pin. |
| PMODE | (pin-num:u8 mode:u8 -- ) Sets the output mode of a pin given in value `pin-num` tomode in value `mode`, via a call to pinMode(). E.g. 1 11 PMODE sets the mode of pin 11 to 1 (output). |
| ?T | ( -- ) Prints the system time in ms since restart as a 32 bit unsigned decimal. It overflows after about 50 days. |

##Commands for Sargood Controller

|Command|Description|
|-------|-----------|
| CMD | (u8 -- ) Queue a command from TOS to be run by the controller. Values given in APP_CMD_xxx in app.h. Note than commands are only accepted if the command processor is not busy, with the exception of APP_CMD_STOP which immediately aborts any command and stops motion, and the timed motion commands APP_CMD_HEAD_UP etc. which if repeated just extend the running time of the motor. |
| ?S | ( -- ) Prints the values of all tilt sensors in signed decimal. If the sensor is not reponding or is reading bad data then the value |
| ?PR | ( -- ) Print the position presets as a row of sensor values for each preset on a separate line. |
| PR | (foot-pos:d head-pos:d preset-index:u8 -- ) Write a position preset. E.g. `1234 2345 2 PR` writes preset index 2, setting sensor 0 (head) to 2345 and sensor 1 (foot) to 1234. If the preset index is out of range an error is flagged. No check is made of the sensor values. Note that the change is not writen to non-volatile memory. The user must use the command `NV-W` to do that. |
| ?LIM | ( -- ) Print the motion limits for each axis as a list of sensor values, as head-lower head-upper foot lower foot upper. |
| LIM | (lower-limit:d upper-limit:d axis-idx:u8 -- ) Set the motion limits for an axis. E.g. `456 234 1 LIM` sets the limits for axis 1 (foot) to 234 (lower) and 456 (upper). If the preset index is out of range an error is flagged. No check is made of the limit values. Note that the change is not writen to non-volatile memory. The user must use the command `NV-W` to do that. |
| BL | (u8 -- ) Sets the LCD backlight brightness as an 8 bit value from 0 through 255 inclusive. Note that the system may also set the brightness and overwrite the effects of this command. |
| EVENT | (u8 -- ) Publishes an event with ID set by the 8 bit value in TOS, and the event's 8 & 16 bit payloads set to zero. The event is queued and read by all state machines. For event IDs refer to event_local.h. |
| EVENT-EX | (payload-16:u16 payload-8:u8 is:u8-- ) Publishes an event with ID and the 8 & 16 bit payloads set by values on the stack. E.g `1234 56 78 EVENT-EX` will publish an event with 16 bit payload 1234, 8 bit payload 56 and ID 78. The event is queued and read by all state machines. For event IDs refer to event_local.h. |
| CTM | ( -- ) Clears the event trace mask so that no events are traced. The changes must be written to non-volatile storage with the `NV-W` command to be made permanent. |
| DTM | ( -- ) Sets the event trace mask to trace most interesting events. The changes must be written to non-volatile storage with the `NV-W` command to be made permanent. |
| ?TM | ( -- ) Print the event trace mask as a set of 16 bit hex values. Only the mask for used events is printed, rather than the entire mask of 16 values. |
| ??TM | ( -- ) Prints a verbose list of all defined events: ID, name and whether they are being traced. |
| STM | (enable:b id:u8 -- ) Sets whether an individual event is traced. Event ID on TOS, value on NOS sets traced if nonzero, not traced otherwise. E.g `1 33 STM` will set the tracemask bit for event ID 33. |

##Commands for Relay Unit

|Command|Description|
|-------|-----------|
| RLY | (u8 -- ) Write the lower 8 bits to the 8 relays. This is a shortcut for writing a value to the register controlling the relays. Note that the system will overwrite your setting, so you should disable this by disconnecting the unit from the bus. |
| ?RLY | ( -- ) Print the state of the 8 relays. This is a shortcut for printing the value of the register controlling the relays. |

##Commands for Sensor Unit

|Command|Description|
|-------|-----------|
