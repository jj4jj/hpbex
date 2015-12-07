#D[C]Agent
D[C]Agent is a distribute system basic comunication componets [agent] solution .

##DCAgent main feature##

- lib for distribute agent client
- distribute agent server for report bussiness, monitor, machine statistics , python extension etc.



##Architecture##

- Bussiness Application [reporter , execter , monitor , machine statistics]
- DCAgent [wrap DCNode with python extension]
- DCNode [local:system mq, remote:tcp]

```

    1:N communication module



	               			agent(root)[smq:pull,tcp:server]
								|					|
								|					|
	        	agent[tcp:client,tcp:server]  	smq leaf[smq:push]
					|					|
         agent[smq:pull,tcp:client] 	|
			|							|
    smq leaf[smq:push]    			tcp leaf[tcp:client]


    leaf node:
          1. register name
          2. send msg by msgq to parent
    agent node:
          1. register name
          2. send msg by msgq or tcp to parent
          3. forward msg [route] to other [known/unknown] node
    root node:
          1. an agent node with no parent node

	NOTE:
	    msgq push and pull that is a 1:N communication model
	    sms: msgq pull end [server]
	    smc: msgq push end [client]
	    sms:
	        1. get shm for name maping
	        2. get a request for register name
	        3. if name is collision [no host] . reject registering
	        4. set name map the name->id 
	        	the ID must not a pid[long :seed+seq].
	        5. resonse to client
	    smc:
	        1. register name with sms [using pid temp communication]
	        2. recv name response . got a valid key for communication.
	        3. in ready state, smc can use name map shm to lookup peer node.
```





##depends##

1. libprotobuf 2.6+ (libprotobuf-dev)
2. libpython 2.7.5+ (python-dev)
3. cmake 2.6+


##doing / done / todo / opt##

- router caching [done]
- alloc msg buffer with zero copy [by lower layer allocated]  [opt]
- msgq name manage      [done]
- python extension in agent module  [done]
- same agent [brother] communication with msgq directly [p2p] [done]
- msg persistence [opt]
- dcnode_send should create a send queue . [todo]
- bench mark todo [doing]
- dagent python export [test swig? ]  [done]
- dagent python plugins   [done]
- add dbproxy for mysql [orm]  [done]
- push service [done with (sdv)[https://github.com/jj4jj/sdv.git] ]
- data visualization with echarts [done with (sdv)[https://github.com/jj4jj/sdv.git]]
- utilities




##build##
    make clean #optional
    make



##test##
    ls bin/				#show the the test programs 
	./dagent n	l1		#agent
	./dagent n  l2		#root
	./dagent n	l		#leaf
	mkdir -p /tmp/dagent 	#for path token to key
	./collector			#collector recv from reporter
	./reporter			#reporter send to collector


