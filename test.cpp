#include "mysql_gen.h"
#include "test.pb.h"
#include "test.hpb.h"
#include <iostream>
#include "google/protobuf/compiler/importer.h"

extern std::stringstream error_stream;
using namespace std;
using namespace google::protobuf;


#include "../dcagent/base/utility_mysql.h"
#include "../dcagent/base/logger.h"

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
	MySQLMsgCvt	msc("test.proto", nullptr);
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
	//dhello.mutable_hello()->set_id(20);
	bzero(buffer_db, sizeof(buffer_db));
	if (!dhello.SerializeToArray(buffer_db, sizeof(buffer_db))){
		cerr << "pack error !" << endl;
		return -2;
	}
	int buffer_len = dhello.ByteSize();
	cout << "origin bytes:" << dhello.ByteSize() << "buffer:" << buffer_db << endl;
	Message * pMsg = msc.GetProtoMeta().NewDynMessage("DBHello", buffer_db, buffer_len);
	if (hellogen.AttachMsg(pMsg)){
		cerr << "init meta error ! ret:" << iret << endl;
		cerr << error_stream.str() << endl;
		return -2;
	}
	cout << "parsed bytes:" << pMsg->ByteSize() << endl;
	bzero(buffer_db, sizeof(buffer_db));
	if (!pMsg->SerializeToArray(buffer_db, sizeof(buffer_db))){
		cerr << "pack error !" << endl;
		return -2;
	}
	cout << "parsed bytes:" << pMsg->ByteSize() << "buffer:" << buffer_db << endl;
	cout << "===============================================" << endl;

	///////////////////////////////////////////////////////////////////////////////
	global_logger_init(logger_config_t());
	using namespace dcsutil;
	mysqlclient_t	mc;
	mysqlclient_t::cnnx_conf_t	conf;
	conf.ip = "127.0.0.1";
	conf.uname = "test";
	conf.passwd = "123456";
	conf.port = 3306;
	if (mc.init(conf)){
		return -1;
	}
	mc.execute("use test;");


	string sql;
	msc.CreateDB("test_msc", sql);
	mc.execute(sql);

	hellogen.CreateTable(sql);
	mc.execute(sql);

	hellogen.Insert(sql);
	mc.execute(sql);

	hellogen.Update(sql);
	mc.execute(sql);

	hellogen.Delete(sql);
	mc.execute(sql);

	hellogen.Replace(sql);
	mc.execute(sql);

	hellogen.Select(sql, nullptr, "");
	mc.execute(sql);

	struct _test {
		static void 	cb(void* ud, INOUT bool & need_more, const dcsutil::mysqlclient_t::mysqlclient_row_t & row){
			LOGP("cb ud:%p row:%s (%zu) name:%s total:%zu offset:%zu! more:%d",
				ud, row.row_data[0], row.row_length[0], row.fields_name[0], row.row_total, row.row_offset, need_more);
			static char buffer[128];
			MySQLRow msr;
			msr.row_data = row.row_data;
			msr.row_lengths = row.row_length;
			msr.num_fields = row.fields_count;
			msr.fields_name = row.fields_name;
			msr.table_name = row.table_name;
			MySQLMsgCvt * pmsc = (MySQLMsgCvt*)ud;
			int msglen = 128;
			pmsc->GetMsgBufferFromMySQLRow(buffer, &msglen, msr);
			cout << "get msg buffer froom mysql row buffer len:" << msglen << endl;
			DBHello parse_hello;
			if (!parse_hello.ParseFromArray(buffer, msglen)){
				cout << "unpack error !" << endl;
				return;
			}
			cout << "unpacked msg:" << parse_hello.ShortDebugString() << endl;
		}
	};
	int ret = mc.result(&msc, _test::cb);
	if (ret < 0){
		std::cerr << ret << " error:" << mc.err_msg() << endl;
		

		return -1;
	}


	hellogen.DropTable(sql);
	mc.execute(sql);

	msc.DropDB("test_msc", sql);
	mc.execute(sql);

	//msc.CreateTables("DBHello", sql);
	//cout << sql << endl;

	return 0;
}

