
hpbex: main.cpp
	g++ $^ -I/usr/local/include -lprotobuf -o $@ -std=c++11 -g -Wall
clean:
	rm -f hpbex
test:
	./hpbex test.proto Hello
src: test.proto hpbex ext
	./hpbex test.proto Hello > test.hpb.h
cl: src
	g++ test.cpp *.cc -o test -I/usr/local/include -std=c++11 -g -Wall -lprotobuf
pb: test.proto
	protoc test.proto extensions.proto -I. -I/usr/local/include --cpp_out=.
ext : extensions.proto
	protoc extensions.proto -I. -I/usr/local/include --cpp_out=.


