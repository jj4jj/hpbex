hpbex: main.cpp ext_meta.cpp extensions_option.cpp extensions.pb.cc flatmsg_gen.h
	g++ main.cpp ext_meta.cpp extensions_option.cpp extensions.pb.cc -I/usr/local/include -lprotobuf -o $@ -std=c++11 -g -Wall
extensions.pb.cc: extensions.proto
	protoc extensions.proto -I. -I/usr/local/include --cpp_out=.
test: hpbex
	./hpbex test.proto Hello
src: test.proto hpbex
	./hpbex test.proto Hello > test.hpb.h
clean:
	rm -f hpbex
	rm -f *.pb.cc *.hpb.h
	rm -f test_ext test
	rm -f ./examples/proto/*.h
	rm -f ./examples/proto/*.cc
	rm -f ./examples/proto/hpbex

eg:	hpbex
	cd ./examples/proto/ && make gen

##########################################################################
pbt: test.proto
	protoc test.proto extensions.proto -I. -I/usr/local/include --cpp_out=.
test_ext: extensions.pb.cc pbt
	g++ -I/usr/local/include -g test_ext.cpp extensions_option.cpp extensions.pb.cc test.pb.cc -o test_ext -lprotobuf -std=c++11
mcl: pbt src
	g++ test.cpp ext_meta.cpp mysql_gen.cpp extensions_option.cpp *.cc -o test -I/usr/local/include -std=c++11 -g -Wall -lprotobuf -lmysqlclient -L../dcagent/lib -ldcbase

##########################################################################
