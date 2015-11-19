

#include "test.pb.h"
#include <iostream>
#include <stdio.h>
#include "extensions_option.h"

extern std::stringstream error_stream;
using namespace std;
using namespace google::protobuf;



int main(){

	Hello hello;

    const char * ptext = "heelofffffffffffxfsfd";
    string cs = ptext;
    printf("%p %p\n", ptext, cs.data());
	const Descriptor * dp = hello.GetDescriptor();
	/*
	if (dp->options().HasExtension(pks)){
		cout << 1 << endl;
	}

	string v = hello.GetDescriptor()->options().GetExtension(pks);
	cout << v << endl;
	cout << hello.GetDescriptor()->options().HasExtension(pks) << endl;

	v = dp->field(0)->options().GetExtension(cn);
	cout << v << endl;
	*/
	cout << proto_msg_opt(dp, "pks") << endl;
	return 0;
}
