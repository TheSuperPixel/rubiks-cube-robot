CFLAGS = -Wall -O3

solve: solve.o prun_table.o 
	gcc -o solve solve.o prun_table.o 

.PHONY clean:
	-rm *.o solve
	
#-Wl,--gc-sections,--print-gc-sections
