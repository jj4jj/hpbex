
hpbex: main.cpp ext_meta.cpp
	g++ $^ -I/usr/local/include -lprotobuf -o $@ -std=c++11 -g -Wall
clean:
	rm -f hpbex
test:
	./hpbex test.proto Hello
src: test.proto hpbex ext
	./hpbex test.proto Hello > test.hpb.h
cl: pb src
	g++ test.cpp ext_meta.cpp mysql_gen.cpp *.cc -o test -I/usr/local/include -std=c++11 -g -Wall -lprotobuf -lmysqlclient
pb: test.proto
	protoc test.proto extensions.proto -I. -I/usr/local/include --cpp_out=.
ext : extensions.proto
	protoc extensions.proto -I. -I/usr/local/include --cpp_out=.


