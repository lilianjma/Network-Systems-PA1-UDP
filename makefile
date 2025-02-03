# Compiler
CC = gcc

# Compiler Flags
CFLAGS = -Wall -Wextra -g

# Target Executables
CLIENT = client
SERVER = server

# Source Files
CLIENT_SRC = udp_client.c
SERVER_SRC = udp_server.c

# Default rule: Compile both client and server
all: $(CLIENT) $(SERVER)

# Compile the client
$(CLIENT): $(CLIENT_SRC)
	$(CC) $(CFLAGS) -o $(CLIENT) $(CLIENT_SRC)

# Compile the server
$(SERVER): $(SERVER_SRC)
	$(CC) $(CFLAGS) -o $(SERVER) $(SERVER_SRC)

# Clean up compiled files
clean:
	rm -f $(CLIENT) $(SERVER)
