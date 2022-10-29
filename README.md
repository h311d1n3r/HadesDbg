# HadesDbg
## Description
### The Linux x86/x86-64 last chance debugging tool.
**HadesDbg** is the debugger you want to use if other dynamic tools fail to do the job.  
For example, it should handle processes that **fork** into multiple sub-processes.
## How does it work ?
The goal was to create a debugger that wouldn't alter the process **normal execution**.  
To deal with this hard task, **HadesDbg** attaches to the target, **injects instructions to communicate by the way of a pipe file**, and then detaches from the target.
## Table of contents
[Build the tool](#build)  
[How to use ?](#how)  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[Syntax](#how_syntax)  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[Parameters](#how_params)  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[Flags](#how_flags)  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[Configuration file](#how_config)  
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[Example](#how_example)  
[Warning](#warning)  

<a name="build"/>

## Build the tool  
1. You need to have cmake and g++-multilib installed on your system.  
2. Clone the repository.  
3. Compile the tool with `make`.  
4. Add the generated `bin` directory to your path using `PATH=$PATH:~/path/to/the/repo/bin`.  
## How to use ?

<a name="how_syntax"/>

### Syntax
`hadesdbg32/hadesdbg64 binary [-param value] [--flag]`

<a name="how_params"/>

### Parameters
`entry` -> Specifies the program entry point. e.g: 'entry 0x401000'  
`bp` -> Injects a breakpoint at the given address. **Notice that you must specify a size (at least 8 for 32bit and 12 for 64bit) that corresponds to an amount of bytes that can be replaced without cutting an instruction in two.** e.g: 'bp 0x401080:14'  
`args` -> Specifies the arguments to be passed to the traced binary. e.g: 'args "./name.bin hello world"'  
`script` -> Provides a script to automatically execute the debugger. e.g: 'script "./auto.hscript"'  
`output` -> Redirects the formatted output of the tool into a file. e.g: 'output "./output.hout"'  
`config` -> Provides command parameters from a file. e.g: 'config "./config.hconf"'  

<a name="how_flags"/>

### Flags
`help` -> Displays a help message.  
### Debugging commands
The following commands can be used when the target hits a breakpoint :  
`readreg` or `rr` -> Returns value of the specified register. e.g: 'readreg rax'  
`readregs` or `rrs` -> Returns values of all registers at once.  
`readmem` or `rm` -> Returns n bytes at the specified address of target. e.g: 'readmem [rax+0x20]-5 0xa'  
`readmem_ascii` or `rma` -> Returns an ascii string containing n bytes at the specified address of target. e.g: 'readmem_ascii @0x2008 0xa'  
`writereg` or `wr` -> Writes the specified value into the specified register. e.g: 'writereg rsi 0x12345678'  
`writemem` or `wm` -> Writes the specified hex chain at the specified address of target. e.g: 'writemem 0xffe439cc 0102030405abcdef'  
`run` or `r` -> Resumes execution after a breakpoint was hit.  
`info` or `i` -> Displays information concerning the debugging session.  
`exit` or `e` -> Ends the debugging session.

<a name="how_config"/>

### Configuration file
After running HadesDbg for the first time, a config file will be generated at `${HOME}/.hadesdbg/config.json`.  
You can edit it in order to change the default behaviour of HadesDbg.  
The following settings can be modified :  
`open_delay_milli` -> How often (in milliseconds) the debugger checks for target messages in a pipe file.  
`theme` -> Theme to be used for printing in console. Values are `cetus`, `cerberus` and `basilisk`.  

<a name="how_example"/>

### Example
#### Command
The following command will execute the binary ./target given 0x12c0 as the entry, and inject a breakpoint at address 0x13f7.  
`hadesdbg64 ./test/bin/x64/test_3 -entry 0x12c0 -bp 0x13f7:14`
#### Result
Here is a screenshot of a hit breakpoint during a debugging session :  
  
<p align="center">
  <img src="https://i.imgur.com/0qB7mPn.png" alt="result.png"/>
</p>

<a name="warning"/>

## Warning
**This software must only be used to carry out lawful experiments and I am not responsible for any breach of this rule !**  
