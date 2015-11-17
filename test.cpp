#include "mysql_gen.h"
#include "test.pb.h"
#include "test.hpb.h"
#include <iostream>

extern std::stringstream error_stream;
using namespace std;

int main(){
	Hello_ST hs1, hs2;
	Hello hello;
	hs1.d.b.f1 = 22;
	hs1.convto(hello);
	std::cout << "dump:" << hello.ShortDebugString() << std::endl;

	MySQLMsgConverter	msc("test.proto", nullptr);
	int ret = msc.InitSchema(hello);
	if (ret){
		cerr << "init schama error ! ret:"<< ret << endl;
		cerr << error_stream.str() << endl;
		return -1;
	}
	string sql;
	msc.Insert(sql);
	cout << sql << endl;

	msc.Update(sql);
	cout << sql << endl;

	msc.Delete(sql);
	cout << sql << endl;

	msc.Replace(sql);
	cout << sql << endl;

	msc.Select(sql);
	cout << sql << endl;



	return 0;
}

