all: bin bin/server bin/client
	
bin:
	mkdir bin

bin/server: source/server.c
	gcc source/server.c -o bin/server -Wall -Werror -lm -fsanitize=address,leak

bin/client: source/client.c
	gcc source/client.c -o bin/client -Wall -Werror -lm -fsanitize=address,leak

clean:
	rm bin/server bin/client
	rmdir bin

