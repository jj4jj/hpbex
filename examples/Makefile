PROTOBUF_INC=-I/usr/local/include
PROTOBUF_LIB=/usr/local/lib/libprotobuf.a
PROTOC=/usr/local/bin/protoc
verify:
	cd proto && ${PROTOC} -I. ${PROTOBUF_INC} extensions.proto --cpp_out=.
	cd proto && ${PROTOC} -I. ${PROTOBUF_INC} comm.proto --cpp_out=.
	cd proto && ${PROTOC} -I. ${PROTOBUF_INC} cs.proto  --cpp_out=.
	cd proto && ${PROTOC} -I. ${PROTOBUF_INC} db.proto  --cpp_out=.
	cd proto && ${PROTOC} -I. ${PROTOBUF_INC} mm.proto  --cpp_out=.

cpp: verify ../hpbex
	../hpbex mm.proto mm.Hero -o cpp/mm.hpb.h -Iproto
	../hpbex db.proto db.DBTest -o cpp/db.hpb.h -Iproto
	../hpbex comm.proto CommBase -o cpp/comm.hpb.h -Iproto
	g++ proto/*.cc cpp/test.cpp ${PROTOBUF_INC} -std=c++11 -o test_cpp -Iproto ${PROTOBUF_LIB}
	echo "generate hpb file success !"

ext: proto/test.proto proto/extensions.proto ../hpbex
	cd proto && ${PROTOC} test.proto extensions.proto -I. ${PROTOBUF_INC} --cpp_out=.
	../hpbex test.proto DBHello -o sql/test.hpb.h -Iproto

sql: ext
	cd dcagent && make
	g++ sql/test.cpp ../ext_meta.cpp ../mysql_gen.cpp ../extensions_option.cpp proto/extensions.pb.cc proto/test.pb.cc -I.. -Iproto -o test_sql ${PROTOBUF_INC} -std=c++11 -g -Wall ${PROTOBUF_LIB} -lmysqlclient -Ldcagent/lib -ldcbase -ldcutil -Idcagent

clean:
	rm -f proto/*.pb.cc
	rm -f proto/*.pb.h
	rm -f cpp/*.hpb.h
	rm -f sql/*.hpb.h
	rm -f test_cpp
	rm -f test_sql


