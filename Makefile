# See gcc/clang manual to understand all flags
CFLAGS += -std=c99 # Define which version of the C standard to use
CFLAGS += -Wall # Enable the 'all' set of warnings
CFLAGS += -Werror # Treat all warnings as error
CFLAGS += -Wshadow # Warn when shadowing variables
CFLAGS += -Wextra # Enable additional warnings
CFLAGS += -O2 -D_FORTIFY_SOURCE=2 # Add canary code, i.e. detect buffer overflows
CFLAGS += -fstack-protector-all # Add canary code to detect stack smashing
CFLAGS += -D_POSIX_C_SOURCE=201112L -D_XOPEN_SOURCE # feature_test_macros for getpot and getaddrinfo
CFLAGS += -lz # zlib library

# We have no libraries to link against except libc, but we want to keep
# the symbols for debugging
LDFLAGS= -rdynamic

# Default target
all: clean sender receiver
	  


# If we run `make debug` instead, keep the debug symbols for gdb
# and define the DEBUG macro.
debug: CFLAGS += -g -DDEBUG -Wno-unused-parameter -fno-omit-frame-pointer
debug: 	clean sender receiver

sender : src/packet_implem.o src/create_socket.o src/real_address.o src/sender.o 
	gcc -o sender src/sender.o src/create_socket.o src/real_address.o src/packet_implem.o $(CFLAGS)

receiver: src/packet_implem.o src/create_socket.o src/real_address.o src/receiver.o 
	gcc -o receiver src/receiver.o src/create_socket.o src/real_address.o src/packet_implem.o $(CFLAGS)

.PHONY: clean

clean:
	@rm -f sender receiver src/packet_implem.o src/create_socket.o src/real_address.o src/sender.o src/receiver.o
