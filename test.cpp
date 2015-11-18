#include "mysql_gen.h"
#include "test.pb.h"
#include "test.hpb.h"
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
#if 0

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
#endif

	Hello hello;
	MySQLMsgConverter	msc("test.proto", nullptr);
	int iret = msc.InitSchema();
	if (iret){
		cerr << "init schama error ! ret:" << iret << endl;
		cerr << error_stream.str() << endl;
		return -1;
	}
	//auto hellogen =	msc.GetMsgDesc("Hello");
	MySQLMsgMeta hellogen(&msc);
	DBHello dhello;
	if (hellogen.AttachMsg(&dhello)){
		cerr << "init meta error ! ret:" << iret << endl;
		cerr << error_stream.str() << endl;
		return -2;
	}
	string sql;
	msc.CreateDB("test_msc", sql);
	cout << sql << endl;

	msc.DropDB("test_msc", sql);
	cout << sql << endl;

	msc.CreateTables("DBHello", sql);
	cout << sql << endl;


	hellogen.CreateTable(sql);
	cout << sql << endl;

	hellogen.DropTable(sql);
	cout << sql << endl;

	hellogen.Insert(sql);
	cout << sql << endl;

	hellogen.Update(sql);
	cout << sql << endl;

	hellogen.Delete(sql);
	cout << sql << endl;

	hellogen.Replace(sql);
	cout << sql << endl;

	hellogen.Select(sql);
	cout << sql << endl;


	return 0;
}

