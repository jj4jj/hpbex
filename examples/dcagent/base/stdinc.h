#pragma once

//------------------compatible c part--------------
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cstdarg>
#include <cmath>
#include <ctime>
#include <cstdint>
#include <cstddef>
#include <csetjmp>
#include <cctype>
#include <csignal>
//#include <cfenv>		//float env
#include <clocale>
#include <cstdalign>
#include <cassert>
#include <cerrno>
#include <fcntl.h>
#include <strings.h>
//-------------------std c++03----------------------

#include <string>
using std::string;
#include <vector>
using std::vector;
#include <map>
using std::map;
#include <set>
using std::set;
#include <algorithm>
#include <list>
#include <stack>
#include <iterator>
#include <bitset>
#include <stack>
#include <queue>

/////////////////////////std=c++11/////////////////
#include <memory>
using std::shared_ptr;
using std::make_shared;
#include <functional>
#include <unordered_map>
#include <unordered_set>
using std::unordered_map;
using std::unordered_set;
#include <random>

#include <bitset>
using std::bitset;
#include <sstream>
#include <iostream>
#include <fstream>

//--------------------c++11 thread-----------------
#include <thread>
#include <mutex> 
#include <condition_variable>
#include <atomic>
//---------------c++11 code converting ------------
//#include <codecvt>		//codecvt_utf8/16 8<->16
#include <regex>		//using ECMAScript grammar

//---------------c++11 time during ----------------
//duration	time_point  high_resolution_clock
#include <chrono>	//std::chrono

//------------------linux system call -------------
//-------------------------------------------------
#ifndef gettid
#include <sys/syscall.h>
#define gettid() syscall(__NR_gettid)
#endif


#include <unistd.h>
#include <dirent.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/file.h>
#include <sys/syscall.h>

#include <arpa/inet.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/un.h>
#include <sys/mman.h>
#include <netinet/tcp.h>

//thread
#include <pthread.h>
#include <semaphore.h>


//////////////////////////////////////////////////////
#ifndef restrict
#define restrict 
#endif

#ifndef INOUT
#define INOUT
#endif

#ifndef OUT
#define OUT
#endif

#ifndef IN
#define IN
#endif


///////////////////////////////////////////////////////
#ifndef NS_BEGIN
#define NS_BEGIN(ns)	namespace ns {
#define NS_END()		};
#endif
