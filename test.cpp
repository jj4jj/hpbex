#include "mysql_gen.h"
#include "test.pb.h"
#include "test.hpb.h"
#include <iostream>
#include "google/protobuf/compiler/importer.h"
#include "google/protobuf/dynamic_message.h"

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
	//msg_buffer -> sql



	MySQLMsgMeta hellogen(&msc);
	DBHello dhello;
	struct MyPBLogger {
		static void log(LogLevel level, const char* filename, int line,
					const std::string& message){
			cerr << "log level" << level << "file:" << filename << "line:" << line << "msg:" << message << endl;
		}
	};
	SetLogHandler(MyPBLogger::log);
	static char buffer_db[1024];
	dhello.set_pk1(20555);
	dhello.set_pk2("86888");
	dhello.set_b4(true);
	if (!dhello.SerializeToArray(buffer_db, sizeof(buffer_db))){
		cerr << "pack error !" << endl;
		return -2;
	}
	int buffer_len = dhello.ByteSize();
	hellogen.msg_desc = msc.GetMsgDesc("DBHello");
#if 0
	/*
	UnknownFieldSet	ufs;
	if (!ufs.ParseFromArray(buffer_db, buffer_len)){
		cerr << "unpack error !" << endl;
		return -2;
	}
	
	cout << "parsed count:" << ufs.field_count() << endl;
	for (int i = 0; i < ufs.field_count(); ++i){
		cout << i << ":" << ufs.field(i).number() << " -> " << ufs.field(i).type() << endl;
	}
	*/
#endif
	DynamicMessageFactory	dmf(msc.GetProtoMeta().GetPool());
	Message * pMsg = dmf.GetPrototype(hellogen.msg_desc)->New();

	cout << "get type name:" << pMsg->GetTypeName() << endl;
	if (!pMsg->ParseFromArray(buffer_db, buffer_len)){
		cerr << "unpack error !" << endl;
		return -2;
	}
	cout << "dyn msg parse from buffer ok !" << endl;

	if (hellogen.AttachMsg(pMsg)){
		cerr << "init meta error ! ret:" << iret << endl;
		cerr << error_stream.str() << endl;
		return -2;
	}
	string sql;
	msc.CreateDB("test_msc", sql);
	cout << sql << endl;

	msc.DropDB("test_msc", sql);
	cout << sql << endl;

	//msc.CreateTables("DBHello", sql);
	//cout << sql << endl;

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

