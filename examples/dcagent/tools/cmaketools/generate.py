#!/bin/python
#-*-coding:utf8-*-
import sys
import os

template_path=os.path.join(os.path.dirname(__file__),'templates')

def copy_replace_file(srcfile, dstfile, replaces):
    print("generate file:"+dstfile+" ...")
    with open(srcfile, "r") as f:
        data = f.read()
        for key in replaces:
            data = data.replace(key, str(replaces[key]))
        with open(dstfile, "w") as of:
            of.write(data)

def path_fixing(path):
    if path.find('/') == 0:
        return path
    else:
        return '${PROJECT_SOURCE_DIR}/'+path

def src_files(path):
    incpath = path
    if path.find('/') != 0:
        incpath = '${PROJECT_SOURCE_DIR}'+'/'+path;
    aux_source='aux_source_directory(' + incpath + ' cur_aSRCS)\nset(cur_SRCS "${cur_SRCS};${cur_aSRCS}")'
    return aux_source

def generate(desc , path):
    rootf = os.path.join(template_path,'root.txt')
    libf = os.path.join(template_path,'lib.txt')
    exef = os.path.join(template_path,'exe.txt')
    definations = '\n'.join(map(lambda s:'ADD_DEFINITIONS(-D'+s+')',desc.DEFS))
    if len(desc.DEFS) == 0 :
        definations=''

    if len(desc.LIBS) + len(desc.EXES) == 0:
        print("not found lib or exe modules")
        sys.exit(-1)

    subdirs = ''
    if len(desc.LIBS) > 0 :
        subdirs = subdirs + '\n'.join(map(lambda l:'add_subdirectory('+l['subdir']+')', desc.LIBS))
    subdirs = subdirs + '\n';
    if len(desc.EXES) > 0 :
        subdirs = subdirs + '\n'.join(map(lambda l:'add_subdirectory('+l['subdir']+')', desc.EXES))
    copy_replace_file(rootf, path+'/CMakeLists.txt',
            {'<definations>': definations,
             '<debug_mode>': desc.DEBUG,
             '<project_name>': desc.PROJECT,
             '<extra_c_flags>': desc.EXTRA_C_FLAGS,
             '<extra_cxx_flags>': desc.EXTRA_CXX_FLAGS,
             '<verbose>': desc.VERBOSE,
             '<extra_ld_flags>': desc.EXTRA_LD_FLAGS,
             '<add_subdirectory_area>': subdirs,
             '<project_version>': desc.VERSION})

    """todo
        VERBOSE = 0
        EXTRA_C_FLAGS = ''
        EXTRA_LD_FLAGS = ''
        EXTRA_SRCS
    """


    for lib in desc.LIBS:

        subf=os.path.join(path,lib['subdir'],'CMakeLists.txt')

        includes = ''
        if lib.has_key('includes') and len(lib['includes']) > 0:
            includes = '\n'.join(map(path_fixing,lib['includes']))

        linkpaths = ''
        if lib.has_key('linkpaths') and len(lib['linkpaths']) > 0:
            linkpaths = '\n'.join(map(path_fixing,lib['linkpaths']))

        linklibs = ''
        if lib.has_key('linklibs') and len(lib['linklibs']) > 0:
            linklibs = '\n'.join(lib['linklibs'])

        extra_srcs = ''
        if lib.has_key('src_dirs') and len(lib['src_dirs']) > 0:
            extra_srcs = '\n'.join(map(src_files, lib['src_dirs']))

        lib_type = 'STATIC'
        if lib.has_key('type'):
            lib_type = lib['type']

        copy_replace_file(libf, subf,
            {'<lib_name>': lib['name'],
             '<includes>': includes,
             '<lib_type>': lib_type,
             '<linkpaths>': linkpaths,
             '<linklibs>': linklibs,
             '<extra_srcs>': extra_srcs})

    for exe in desc.EXES:
        subf=os.path.join(path,exe['subdir'],'CMakeLists.txt')

        includes = ''
        if exe.has_key('includes') and len(exe['includes']) > 0:
            includes = '\n'.join(map(path_fixing,exe['includes']))


        linkpats = ''
        if exe.has_key('linkpaths') and len(exe['linkpaths']) > 0:
            linkpaths = '\n'.join(map(path_fixing,exe['linkpaths']))

        linklibs = ''
        if exe.has_key('linklibs') and len(exe['linklibs']) > 0:
            linklibs = '\n'.join(exe['linklibs'])

        extra_srcs = ''
        if exe.has_key('src_dirs') and len(exe['src_dirs']) > 0:
            extra_srcs = '\n'.join(map(src_files, exe['src_dirs']))

        copy_replace_file(exef, subf,
            {'<exe_name>': exe['name'],
             '<includes>': includes,
             '<linkpaths>': linkpaths,
             '<linklibs>': linklibs,
             '<extra_srcs>': extra_srcs})

def main(desc_file_path):
    #import desc_file
    path=os.path.dirname(desc_file_path)
    name=os.path.basename(desc_file_path)
    sys.path.append(path)
    desc=__import__(name)
    generate(desc, path)

def usage():
    print("./generate.py <description filepath>")


if __name__ == '__main__':
    if len(sys.argv) < 2:
        usage()
        sys.exit(-1)
    main(sys.argv[1])
