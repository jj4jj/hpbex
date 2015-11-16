
hpbex: main.cpp
	g++ $^ -I/usr/local/include -lprotobuf -o $@ -std=c++11 -g
clean:
	rm -f hpbex
test:
	./hpbex test.proto Hello
