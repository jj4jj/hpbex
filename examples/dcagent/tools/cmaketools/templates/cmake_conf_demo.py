PROJECT='dcagent'
VERSION='0.0.1'
DEBUG = 0	#0/1
DEFS = []
VERBOSE = 'off'	#on/off
EXTRA_C_FLAGS = ''
EXTRA_CXX_FLAGS = '-std=c++11'
EXTRA_LD_FLAGS = '-ldl -lm -lrt -pthread'
LIBS = [
        {
            'name':'dcbase',
            'subdir':'base',
            'linklibs' : [
                'libprotobuf.a',
                'python2.7',
            ],
            'src_dirs':['base'],
            'extra_srcs': [''],
        },
        {
            'name':'dcnode',
            'subdir':'dcnode',
            'src_dirs':['base','dcnode/proto'],
        },
        {
            'name':'dagent',
            'subdir':'dagent',
            'src_dirs':['base','dcnode','dcnode/proto','dagent/proto'],
        },
        {
            'name':'dagent_py',
            'subdir':'dagent/python',
            'type': 'SHARED',
            'includes':['base','dcnode','dagent'],
            'linkpaths':[],
            'src_dirs':['base','dcnode','dcnode/proto','dagent','dagent/proto'],
            'linklibs' : [
                'protobuf','python2.7'
            ]
        },

]
EXES = [
        {
            'name':'testagent',
            'subdir':'app/test',
            'linklibs' : [
                'dagent',
                'python2.7',
                'protobuf',
            ]
        },
        {
            'name':'reporter',
            'subdir':'app/reporter',
            'linklibs' : [
                'dagent',
                'python2.7',
                'protobuf',
            ]
        },
        {
            'name':'collector',
            'subdir':'app/collector',
            'linklibs' : [
                'dagent',
                'python2.7',
                'protobuf',
            ]
        },
        {
            'name':'pingpong',
            'subdir':'app/pingpong',
            'linklibs' : [
                'dcnode',
                'protobuf',
            ]
        },
]
