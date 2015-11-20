# hpbex
be short for "Header Protobuf Extensions"


##description
- using protobuf describe c/c++ flat memory structure and mysql db schema or other schema data;
- additional , it can generate common data structure for the repeat field (linear list, binaray searching) in memory struct (C++); 
- it includes flatmsg (structure), mysql and Proto generator , offen , it can use in game dev  related area.



##implements

1. extend protobuf 2.6.1 meta (options) [1]
2. simple wrapper for protobuf runtime importer
3. convert each-other in extend protobuf msg (EXPB)[2] and flat layout C-style struct (POD)
4. convert each-other in EPB and  mysql table record (ROW)[3]



##todo
1. test proto with package
2. extend generators
3. fixing bugs


##usage
1. install libprotobuf-dev9 (2.6.1 git clone from google/protobuf)
2. install libmysqlclient-dev for test mysql msg converter [optional]
3. read main.cpp code for generate POD *.hpb.h, example is simple , read the test.cpp for mysqlmsg releated. make && make test. (make cl for mysqlmsgconvert it also need [dcagent](https://github.com/jj4jj/dcagent "dcagent") repo) 
4. do yourself work, the proto should import the extensions.proto , protoc with -I(the protobuf include path)

##compile
    #for simple test 
	clone the repo
	install libprotobuf-dev (2.6.1)
	make				#for generating hpbex program
	make test 			#or excute: ./hpbex test.proto Hello

##tips
- don't support the proto mutlti-level unfold a mysql table schema ;
- just unfold 1 level , don't support the repeate filed in 1st level;
- not process the PROGRAMMING key words collision yet (var name and struct name).

##index
- [1] refer to google protobuf developers document , extend , custom options .
- [2] extend the protobuf MessageOptions / FieldOptions etc ...
- [3] top 1 level unfolding .























