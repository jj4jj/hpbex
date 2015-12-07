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



##doing
1. extend generators
2. fixing bugs


##usage
1. install libprotobuf-dev9 (2.6.1 git clone from google/protobuf)
2. install libmysqlclient-dev for test mysql msg converter [optional]
3. read main.cpp code for generate POD *.hpb.h, example is simple , read the test.cpp for mysqlmsg releated. make && make test. (make cl for mysqlmsgconvert it also need [dcagent](https://github.com/jj4jj/dcagent "dcagent") repo) 
4. do yourself work, the proto should import the extensions.proto , protoc with -I(the protobuf include path)

##compile && run
    #for simple test 
	clone the repo
	install libprotobuf-dev (2.6.1)
	make				#for generating hpbex program
	make test 			#or excute: ./hpbex test.proto Hello
    make ex             #make the examples 
    cd examples         #clone the dcagent to uplayer and build it 
    ./test_sql          #test mysql generateor, -h show the usage
    ./test_cpp          #test cpp pod struct generateor, -h show the usage

###test mysql generate demo###
####create table in flatmode####
![mysql_create_table][4]
####mysql sql####
![mysql_sql][5]
####mysql top layer unfold sql####
![mysql_toplayer][6]

##tips
- don't support the proto mutlti-level unfold a mysql table schema ;
- just unfold 1 level , don't support the repeate filed in 1st level;
- not process the PROGRAMMING key words collision yet (var name and struct name).

##index
[1]: refer to google protobuf developers document , extend , custom options .


[2]: extend the protobuf MessageOptions / FieldOptions etc ...


[3]: top 1 level unfolding .


[4]: https://github.com/jj4jj/hpbex/blob/master/mysql_create_table.png
[5]: https://github.com/jj4jj/hpbex/blob/master/mysql_sql.png
[6]: https://github.com/jj4jj/hpbex/blob/master/mysql_toplayer.png
























