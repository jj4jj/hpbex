PROTOBUF_INC=-I/usr/local/include
PROTOBUF_LIB=/usr/local/lib/libprotobuf.a

hpbex: main.cpp ext_meta.cpp extensions_option.cpp extensions.pb.cc flatmsg_gen.h
	g++ main.cpp ext_meta.cpp extensions_option.cpp extensions.pb.cc ${PROTOBUF_INC} ${PROTOBUF_LIB} -o $@ -std=c++11 -g -Wall
extensions.pb.cc: extensions.proto
	protoc extensions.proto -I. ${PROTOBUF_INC} --cpp_out=.
test: hpbex
	./hpbex test.proto Hello
src: test.proto hpbex
	./hpbex test.proto Hello > test.hpb.h
clean:
	rm -f hpbex
	rm -f *.pb.cc *.hpb.h
	rm -f *.pb.h
	rm -f test_ext test
	rm -f ./examples/proto/*.h
	rm -f ./examples/proto/*.cc
	rm -f ./examples/proto/hpbex
	rm -f ./examples/proto/test

eg:	hpbex
	cd ./examples/proto/ && make gen

##########################################################################
pbt: test.proto
	protoc test.proto extensions.proto -I. ${PROTOBUF_INC} --cpp_out=.
test_ext: extensions.pb.cc pbt
	g++ -I/usr/local/include -g test_ext.cpp extensions_option.cpp extensions.pb.cc test.pb.cc -o test_ext ${PROTOBUF_LIB} -std=c++11
mcl: pbt src
	g++ test.cpp ext_meta.cpp mysql_gen.cpp extensions_option.cpp *.cc -o test ${PROTOBUF_INC} -std=c++11 -g -Wall ${PROTOBUF_LIB} -lmysqlclient -L../dcagent/lib -ldcbase -I../dcagent ../dcagent/utility/utility_mysql.cpp

##########################################################################
