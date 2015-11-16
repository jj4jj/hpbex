
#include <algorithm>
#include "test.pb.h"
#include "test.hpb.h"


#include "iostream"


int main(){
	Hello_ST hs1, hs2;
	Hello hello;
	hs1.d.b.f1 = 22;
	hs1.convto(hello);
	std::cout << "dump:" << hello.ShortDebugString() << std::endl;
	return 0;
}

