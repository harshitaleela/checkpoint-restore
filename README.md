**************************************************************************************************

				CHECKPOINTING HW1 & HW2

**************************************************************************************************

Run the command _make check_ for quick testing.

'make check' would first execute 'make clean' followed by 'make all' and then proceed with 
executing the checkpoint program followed by restart.

The program was tested on Ubuntu 22.04, with a -fno-stack-protector flag used in the Makefile. 

The executable ./sample prints the counting infinitely with a delay of 1 second between two 
successive counts.

_What works:_ The program is successfully able to write to a checkpoint file and restart from 
	      the saved checkpoint.

**************************************************************************************************
