
verify:
	protoc -I. -I/usr/local/include comm.proto --cpp_out=.
	protoc -I. -I/usr/local/include cs.proto --cpp_out=.
	protoc -I. -I/usr/local/include db.proto --cpp_out=.
	protoc -I. -I/usr/local/include mm.proto --cpp_out=.

gen: ../../hpbex
	cp ../../hpbex ./
	./hpbex mm.proto mm.Hero > mm.hpb.h
	cat mm.hpb.h
