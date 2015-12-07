#include "mysql_gen.h"
#include "test.pb.h"
#include "test.hpb.h"
#include <iostream>
#include "google/protobuf/compiler/importer.h"

extern std::stringstream error_stream;
using namespace std;
using namespace google::protobuf;

#include "../dcagent/utility/util_mysql.h"
#include "../dcagent/base/logger.h"
#include "../dcagent/base/cmdline_opt.h"

int main(int argc, char ** argv){
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
	cmdline_opt_t	cmdopt(argc, &argv[0]);
	cmdopt.parse("flatmode:n::generate mysql sql in flat mode;:r:u:mysql user name;:r:p:mysql password");
	bool flatmode = cmdopt.getoptstr("flatmode") ? true : false;


	Hello hello;
    Hello_ST hellost;
    hellost.construct();
	MySQLMsgCvt	msc("proto/test.proto", nullptr);
	int iret = msc.InitMeta();
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
	//cout << "origin bytes:" << dhello.ByteSize() << "buffer:" << buffer_db << endl;
	cout << "flatmode:" << flatmode << endl;
	if (msc.CheckMsgValid(msc.GetProtoMeta().GetMsgDesc("DBHello"),true, flatmode)){
		cerr << "verify msg desc error " << endl;
		return -3;
	}

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
	//cout << "parsed bytes:" << pMsg->ByteSize() << "buffer:" << buffer_db << endl;
	cout << "===============================================" << endl;

	///////////////////////////////////////////////////////////////////////////////
	if (!cmdopt.getoptstr("u") || !cmdopt.getoptstr("p")){
		cmdopt.pusage();
		return -1;
	}
	global_logger_init(logger_config_t());
	using namespace dcsutil;
	mysqlclient_t	mc;
	mysqlclient_t::cnnx_conf_t	conf;
	conf.ip = "127.0.0.1";
	conf.uname = cmdopt.getoptstr("u");
	conf.passwd = cmdopt.getoptstr("p");
	conf.port = 3306;
	if (mc.init(conf)){
		GLOG_TRA("mysql init error !");
		return -1;
	}
	mc.execute("use test;");

	///////////////////////////////////////////////////////////////////////

	string sql;
	msc.CreateDB("test_msc", sql);
	mc.execute(sql);

	hellogen.CreateTable(sql, flatmode);
	mc.execute(sql);

	hellogen.Insert(sql, flatmode);
	mc.execute(sql);

	hellogen.Update(sql, flatmode);
	mc.execute(sql);

	hellogen.Delete(sql, nullptr, flatmode);
	mc.execute(sql);

	hellogen.Replace(sql, flatmode);
	mc.execute(sql);

	hellogen.Select(sql, nullptr, nullptr, flatmode);
	mc.execute(sql);
	struct param_t  {
		MySQLMsgCvt * cvt;
		bool	flatmode;
	};
	struct _test {
		static void 	cb(void* ud, INOUT bool & need_more, const dcsutil::mysqlclient_t::table_row_t & row){
			GLOG_TRA("cb ud:%p row:%s (%zu) name:%s total:%zu offset:%zu! more:%d",
				ud, row.row_data[0], row.row_length[0], row.fields_name[0], row.row_total, row.row_offset, need_more);
			MySQLRow msr;
			msr.row_data = row.row_data;
			msr.row_lengths = row.row_length;
			msr.num_fields = row.fields_count;
			msr.fields_name = row.fields_name;
			msr.table_name = row.table_name;
			param_t * pmsc = (param_t*)ud;
			static char buffer[260];
			int msglen = sizeof(buffer);
			pmsc->cvt->GetMsgBufferFromSQLRow(buffer, &msglen, msr, pmsc->flatmode);
			cout << "get msg buffer from mysql row buffer len:" << msglen << endl;
			DBHello parse_hello;
			if (!parse_hello.ParseFromArray(buffer, msglen)){
				cout << "unpack error !" << endl;
				return;
			}
			cout << "unpacked msg:" << parse_hello.ShortDebugString() << endl;
		}
	};
	param_t  pm = { &msc, flatmode };
	int ret = mc.result(&pm, _test::cb);
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

