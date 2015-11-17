#include "mysql_gen.h"
//#include "test.pb.h"
//#include "test.hpb.h"
#include <iostream>
#include "google/protobuf/compiler/importer.h"

extern std::stringstream error_stream;
using namespace std;
using namespace google::protobuf;

int main(){
	/*
	Hello_ST hs1, hs2;
	hs1.d.b.f1 = 22;
	hs1.convto(hello);
	std::cout << "dump:" << hello.ShortDebugString() << std::endl;
	*/

	ProtoMeta pm;
	if (pm.Init(nullptr)){
		cerr << "init proto meta error !" << endl;
		return -2;
	}

	auto ret = pm.LoadFile("test.proto");
	if (!ret){
		cerr << "error import !" << endl;
		return -3;
	}
	auto pool = pm.GetPool();
	auto desc = pool->FindMessageTypeByName("Hello");
	if (!desc){
		cerr << "not found message type: Hello" << endl;
		return -1;
	}
	STMessageMeta	smm;
	if (smm.ParseFrom(desc)){
		cerr << "parse from message desc error :" << error_stream.str() << endl;
		return -2;
	}
	clog << error_stream.str() << endl;
	error_stream.clear();

#if 0
	Hello hello;
	MySQLMsgConverter	msc("test.proto", nullptr);
	int iret = msc.InitSchema(hello);
	if (iret){
		cerr << "init schama error ! ret:" << iret << endl;
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
#endif


	return 0;
}

