
verify:
	protoc -I. -I/usr/local/include extensions.proto --cpp_out=.
	protoc -I. -I/usr/local/include comm.proto --cpp_out=.
	protoc -I. -I/usr/local/include cs.proto --cpp_out=.
	protoc -I. -I/usr/local/include db.proto --cpp_out=.
	protoc -I. -I/usr/local/include mm.proto --cpp_out=.

gen: verify ../../hpbex
	cp ../../hpbex ./
	./hpbex mm.proto mm.Hero > mm.hpb.h
	./hpbex db.proto db.DBTest > db.hpb.h
	./hpbex comm.proto CommBase > comm.hpb.h
	g++ *.cc test.cpp -lprotobuf -std=c++11 -o test
	echo "generate hpb file success !"
