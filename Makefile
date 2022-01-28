#fisier folosit pentru compilarea serverului concurent & clientului TCP

all:
	gcc server.c -o server
	gcc client.c -o client
clean:
	rm -f *~ client server