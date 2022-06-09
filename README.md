# HadesDbg
## Description
### The Linux x64 last chance debugging tool.
**HadesDbg** is the debugger you want to use if other dynamic tools fail to do the job.  
For example, it should handle processes that **fork** into multiple sub-processes.
## How does it work ?
The goal was to create a debugger that wouldn't alter the process **normal execution**.  
To deal with this hard task, **HadesDbg** attaches to the target, **injects instructions to communicate by the way of a pipe file**, and then detaches from the target.
## Build the tool
1. Clone the repository.
2. Compile the tool with `make`.
3. Add the generated `bin` directory to your path using `PATH=$PATH:~/path/to/the/repo/bin`.
## How to use ?
### Syntax
`hadesdbg binary [-param value] [--flag]`
### Parameters
`entry` -> Specifies the program entry point. e.g: 'entry 0x401000'  
`bp` -> Injects a breakpoint at the given address. **Notice that you must specify a size (at least 14) that corresponds to an amount of bytes that can be replaced without cutting an instruction in two.** e.g: 'bp 0x401080:14'  
`args` -> Specifies the arguments to be passed to the traced binary. e.g: 'args "./name.bin hello world"'  
### Flags
`help` -> Displays a help message.  
### Debugging commands
The following commands can be used when the target hits a breakpoint :  
`readreg` or `rr` -> Returns value of the specified register. e.g: 'readreg rax'  
`readregs` or `rrs` -> Returns values of all registers at once.  
`readmem` or `rm` -> Returns n bytes at the specified address of target. e.g: 'readmem 0x562f9d400000 0xa'  
`writereg` or `wr` -> Writes the specified value into the specified register. e.g: 'writereg rsi 0x12345678'  
`writemem` or `wm` -> Writes the specified hex chain at the specified address of target. e.g: 'writemem @0x401ab5 0102030405abcdef'  
`run` or `r` -> Resumes execution after a breakpoint was hit.  
`info` or `i` -> Displays information concerning the debugging session.  
`exit` or `e` -> Ends the debugging session.
### Example
#### Command
The following command will execute the binary ./target given 0x401000 as the entry, and inject a breakpoint at address 0x401080.  
`sudo hadesdbg ./target -entry 0x401000 -bp 0x401080:0x10`
#### Result
Here is a screenshot of a hit breakpoint during a debugging session :  
  
<p align="center">
  <img src="https://i.imgur.com/QIJYiJh.png" alt="result.png"/>
</p>
