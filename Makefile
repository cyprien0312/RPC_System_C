CC=cc
RPC_SYSTEM=rpc.o
RPC_ALONE=rpcAlone.o
UTILS=utils.o
TEST_SERVER = server.c
TEST_CLIENT = client.c

SERVER = server
CLIENT = client

.PHONY: format all

all:$(SERVER) $(CLIENT)

$(UTILS): utils.c utils.h
	$(CC) -c -Wall -o $@ $<

$(RPC_ALONE): rpc.c rpc.h 
	$(CC) -c -Wall -o $@ $<

$(RPC_SYSTEM): $(RPC_ALONE) $(UTILS)
	ld -r $^ -o $@

$(SERVER): $(TEST_SERVER) $(RPC_SYSTEM)
	$(CC) -Wall -o $@ $^

$(CLIENT): $(TEST_CLIENT) $(RPC_SYSTEM)
	$(CC) -Wall -o $@ $^

clean:
	rm -f $(RPC_SYSTEM) $(UTILS) $(CLIENT) $(SERVER) $(RPC_ALONE)

format:
	clang-format -style=file -i *.c *.h
