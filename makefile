noncanonical: *.h *.c
	gcc *.c -Wall -o noncanonical
	
writenoncanonical: *.h *.c 
	gcc *.c -Wall -o writenoncanonical

clean :
	rm *.o noncanonical writenoncanonical