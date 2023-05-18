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
The default word size of the console is 16 bits, so a number range of -32768 to 32767 inclusive for signed integers, 0 through 65535 for unsigned integers. You can use 32 bit words quite easily too if you want. 

Signed integers are simply written out, with a leading '-' for negative numbers. Any numbers that are out of range cause an error. I have found this range checking very useful to catch silly mistakes.

Unsigned integers are written with a leading '+', and are similarly checked for range.

Hex numbers are written with a leading '$', eg `$12af`, they are also range checked.

### Strings
Strings are written with a leading '"', but with the wrinkle that they cannot contain a space, as this seperates the string into two tokens. But a space can be entered as an escape sequence `\20`. Other escape sequences are `\\`, `\r` & `\n`, and any character can be entered with a backslash followed by exactly 2 hex digits. The string is terminated with a nul character so it can be used as a regular "C" string, and the address is placed on the stack.

Note that the string is actually stored in the input buffer, so the data is only valid for the remaining commands on the line being processed. The next line will probably corrupt it. So strings must be used in the same line that they are entered. 

The string "Hello, world." would be entered as `"Hello,\20world.`.

### Data
Arbitrary data may be entered as a string of hex digits with a leading "#". Again the data is stored in the input buffer, so the command that uses it must follow the data on the input line.

So `#0603feff` results in 4 bytes in memory. 

## Commands
Commands are not like arguments, they do not add a value to the stack. They might pull values from the stack, add a value themselves, or do something else. Commands are a "word", a set of characters that is seperated from the next command or value by space. They can have any name, but the name cannot be mistaken for a value, so you cannot have a commandcalled `12345678`, it looks like a decimal integer.

There are a few conventions, commands with a leading query e.g `?foo` print something, but in general commands have a variety of names. 

As commands may consume and add values to the stack, there is a simple way to document this as the "stack effect". This shows values on the stack before and after a command is run as an expression in brackets separated by two dashes, so `(before -- after )`. E.g the print TOS command `.` has a stack effect `(d -- ), meaning that it takes a single value representing a decimal number, and consumes it. 

## Builtin Commands

A few commands are standard in the console, these are described below. Note that any argument from the stack is always removed. The abbreviation "TOS" refers to the value on top of the stack. Note that all printed values are followed by a single space. 

|Command|Description                                                                                                                               |
|-------|------------------------------------------------------------------------------------------------------------------------------------------|
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

|Command       |Description                                                                                           |
|--------------|------------------------------------------------------------------------------------------------------|
