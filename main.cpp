#include "flatmsg_gen.h"
#include "ext_meta.h"
#include "mysql_gen.h"
#include "google/protobuf/compiler/importer.h"

extern std::stringstream error_stream;
using namespace std;
using namespace google::protobuf;
int main(int argc, char * argv[]){
	if (argc < 3){
		cout << "usage:" << argv[0] << " <file name> <message type> [-I<proto path>] [-O<output path>] [-o<output file>]" << endl;
		return -1;
	}
	const char * includes[32] = {0};
	const char * output_path = nullptr;
	const char * output_file = nullptr;
	int ninc = 0;
	for(int i = 3; i < argc; ++i){
		if(strncmp("-I",argv[i],2) == 0 ){
			if(argv[i][2]){
				includes[ninc++] = argv[i] + 2;
			}
			else if(i+1 < argc){
				includes[ninc++] = argv[i+1];
				++i;
			}
		}
		if(!output_path && strncmp("-O",argv[i],2) == 0){
			if(argv[i][2]){
				output_path = argv[i] + 2;
			}
			else if(i+1 < argc){
				output_path = argv[i+1];
				++i;
			}	
		}
		if(!output_file && strncmp("-o",argv[i],2) == 0){
			if(argv[i][2]){
				output_file = argv[i] + 2;
			}
			else if(i+1 < argc){
				output_file  = argv[i+1];
				++i;
			}	
		}
	}

	EXTProtoMeta pm;
	if (pm.Init(includes, ninc)){
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
	EXTMessageMeta	smm;
	if (smm.AttachDesc(desc)){
		cerr << "parse from message desc error !" << error_stream.str() << endl;
		return -2;
	}
	//cerr << error_stream.str() << endl;
	error_stream.clear();
	try {
		if(output_path || output_file){
			string file = "";
			if(output_path){
				string file = output_path;
				file += "/" ;
				file +=  argv[1];
				file.replace(file.find(".proto"),6,".hpb.h");
			}
			if(!output_file){
				output_file = file.c_str();
			}
			clog << "generating file:" << output_file << " ... " << endl;
			gen.DumpFile(output_file);
			clog << "generate code file:" << output_file << " sucess !" << endl;
		}
		else {
			std::cout << gen.GetCodeText() << endl;
		}
	}
	catch ( exception & e){
		cerr << "generate code error ! for:" << e.what() << " extra info:" << error_stream.str() << endl;
		return -3;
	}
	return 0;
}
