#include "flatmsg_gen.h"
#include "ext_meta.h"
#include "mysql_gen.h"
#include "google/protobuf/compiler/importer.h"

extern std::stringstream error_stream;
using namespace std;
using namespace google::protobuf;
int main(int argc, char * argv[]){
	if (argc < 3){
		cout << "usage:" << argv[0] << " <file name> <message type> [path]" << endl;
		return -1;
	}
	ProtoMeta pm;
	if (pm.Init(nullptr)){
		cerr << "init proto meta error !" << endl;
		return -2;
	}
	
	auto ret = pm.LoadFile(argv[1]);
	if (!ret){
		cerr << "error import !" << endl;
		return -3;
	}
	auto pool = pm.GetPool();
	auto desc = pool->FindMessageTypeByName(argv[2]);
	if (!desc){
		cerr << "not found message type:" << argv[2] << endl;
		return -1;
	}
	GenerateCXXFlat gen(desc);
	STMessageMeta	smm;
	if (smm.ParseFrom(desc)){
		cerr << "parse from message desc error :" << error_stream.str() << endl;
		return -2;
	}
	cerr << error_stream.str() << endl;
	error_stream.clear();
	try {
		gen.print();
	}
	catch (...){
		cerr << "generate code error ! for:" << error_stream.str() << endl;
		return -3;
	}
	return 0;
}