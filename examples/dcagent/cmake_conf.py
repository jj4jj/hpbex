PROJECT='dcagent-base'
VERSION='0.0.1'
DEBUG = 0    #0/1
DEFS = []
VERBOSE = 'off'    #on/off
EXTRA_C_FLAGS = ''
EXTRA_CXX_FLAGS = '-std=c++11'
EXTRA_LD_FLAGS = '-ldl -lm -lrt -pthread'
LIBS = [
        {
            'name':'dcbase',
            'subdir':'base',
            'linklibs' : [],
            'includes':[],
            'src_dirs':[],
            'extra_srcs': [''],
        },
        {
            'name':'dcutil',
            'subdir':'utility',
            'linklibs' : [],
            'includes':[],
            'src_dirs':[],
            'extra_srcs': [''],
        }

]
EXES = []
