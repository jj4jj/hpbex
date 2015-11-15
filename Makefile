pbtest: pbmain.cpp
	g++ pbmain.cpp -I/usr/local/include -lprotobuf -o pbtest --std=c++11 -g
