all: bin bin/server bin/client resource/cgi-bin/hello resource/cgi-bin/get-marks resource/cgi-bin/send-marks
bin:
	mkdir bin
	mkdir resource/cgi-bin

bin/server: source/server.c
	gcc source/server.c -o bin/server -Wall -Werror -lm -fsanitize=address,leak

bin/client: source/client.c
	gcc source/client.c -o bin/client -Wall -Werror -lm -fsanitize=address,leak

resource/cgi-bin/hello: resource/cgi-source/hello.c
	gcc resource/cgi-source/hello.c -o resource/cgi-bin/hello -Wall -Werror -lm -fsanitize=address,leak

resource/cgi-bin/get-marks: resource/cgi-source/get-marks.c
	gcc resource/cgi-source/get-marks.c -o resource/cgi-bin/get-marks -Wall -Werror -lm -fsanitize=address,leak

resource/cgi-bin/send-marks: resource/cgi-source/send-marks.c
	gcc resource/cgi-source/send-marks.c -o resource/cgi-bin/send-marks -Wall -Werror -lm -fsanitize=address,leak

clean:
	rm bin/server bin/client
	rmdir bin
	rm resource/cgi-bin/hello
	rm resource/cgi-bin/get-marks
	rm resource/cgi-bin/send-marks
	rmdir resource/cgi-bin

