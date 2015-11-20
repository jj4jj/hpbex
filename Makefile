
hpbex: main.cpp ext_meta.cpp extensions_option.cpp extensions.pb.cc
	g++ $^ -I/usr/local/include -lprotobuf -o $@ -std=c++11 -g -Wall
clean:
	rm -f hpbex
	rm -f *.pb.cc *.hpb.h
	rm -f test_ext test
test: hpbex
	./hpbex test.proto Hello
src: test.proto hpbex ext
	./hpbex test.proto Hello > test.hpb.h
cl: pb src
	g++ test.cpp ext_meta.cpp mysql_gen.cpp extensions_option.cpp *.cc -o test -I/usr/local/include -std=c++11 -g -Wall -lprotobuf -lmysqlclient -L../dcagent/lib -ldcbase
pb: test.proto
	protoc test.proto extensions.proto -I. -I/usr/local/include --cpp_out=.
ext : extensions.proto
	protoc extensions.proto -I. -I/usr/local/include --cpp_out=.
test_ext:
	g++ -I/usr/local/include -g test_ext.cpp extensions_option.cpp extensions.pb.cc test.pb.cc -o test_ext -lprotobuf -std=c++11

