PROTOBUF_INC=-I/usr/local/include
PROTOBUF_LIB=/usr/local/lib/libprotobuf.a
PROTOC=/usr/local/bin/protoc

hpbex: main.cpp ext_meta.cpp extensions_option.cpp extensions.pb.cc flatmsg_gen.h extensions.pb.h
	g++ main.cpp ext_meta.cpp extensions_option.cpp extensions.pb.cc ${PROTOBUF_INC} ${PROTOBUF_LIB} -o $@ -std=c++11 -g -Wall
extensions.pb.h extensions.pb.cc: extensions.proto
	${PROTOC} extensions.proto -I. ${PROTOBUF_INC} --cpp_out=.
test: hpbex
	./hpbex test.proto Hello -I examples/proto/  -o test.hpb.h
ex:
	cd examples && make cpp
	cd examples && make sql
clean:
	rm -f hpbex
	rm -f *.pb.cc *.hpb.h
	rm -f *.pb.h
	cd examples && make clean

