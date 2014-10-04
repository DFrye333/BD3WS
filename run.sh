# 		This compiles and conditionally runs the BD3WS program.  It also allows
#	for specification of the "-d" command-line argument which will run BD3WS
#	with gdb attached.

#!/bin/bash

echo -e "--------------------------------------------"

# Attempt compilation.
echo -e "Compiling..."
gcc -g -w -std=gnu99 -pthread -o BD3WS BD3WS.c

# Successful compile. Run the program.
if [ $? -eq 0 ]; then
	echo -e "Successful compilation! Executing program..."
	echo -e "--------------------------------------------\n"
	if [ "$1" = "-d" ]; then
		gdb BD3WS
	else
		./BD3WS
	fi

# Failed compile. Do not run the program.
else
	echo -e "\n\nFailed to compile!"
	echo -e "--------------------------------------------\n"
fi