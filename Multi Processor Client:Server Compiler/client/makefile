OBJECTS = client.o command_handler.o server_connector.o file_handler.o duplicate.o
all: $(OBJECTS)
	cc $(OBJECTS) -o a.out
client.o: command_handler.h
command_handler.o: server_connector.h
duplicate.o: common.h
timer.o: common.h
