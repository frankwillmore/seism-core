#!/usr/bin/env python

import sys, json
import os
import subprocess
from shutil import copy2, make_archive

def VS_full_to_VScompile(argument):
    #print(argument)
    switcher = {
        "Visual Studio 10 2010": "32-VS2010",
        "Visual Studio 10 2010 Win64": "64-VS2010",
        "Visual Studio 11 2012": "32_VS2012",
        "Visual Studio 11 2012 Win64": "64_VS2012",
        "Visual Studio 12 2013": "32_VS2013",
        "Visual Studio 12 2013 Win64": "64_VS2013",
        "Visual Studio 14 2015": "32_VS2015",
        "Visual Studio 14 2015 Win64": "64_VS2015",
    }
    return switcher.get(argument, "nothing")

def VS_full_to_short(argument):
    print(argument)
    switcher = {
        "Visual Studio 10 2010": "32-vs10",
        "Visual Studio 10 2010 Win64": "64-vs10",
        "Visual Studio 11 2012": "32-vs11",
        "Visual Studio 11 2012 Win64": "64-vs11",
        "Visual Studio 12 2013": "32-vs12",
        "Visual Studio 12 2013 Win64": "64-vs12",
        "Visual Studio 14 2015": "32-vs14",
        "Visual Studio 14 2015 Win64": "64-vs14",
    }
    return switcher.get(argument, "nothing")

def cmake_to_binary(basename, platform, osname):
    binname = basename % platform
    return binname

def linux_to_binary(basename, platform, osname):
    switcher = {
        "fedora": "fedora",
        "debian": "debian",
        "ubuntu": "ubuntu",
        "opensuse": "opensuse",
        "centos6": "centos6",
        "centos7": "centos7",
        "ppc64": "ppc64",
        "osx1010": "10.10",
        "osx1011": "10.11",
    }
    binname = basename % switcher.get(osname, "linux")
    return binname

# arguments:
#    1. configuration file name
#    2. ctconfig
#    3. ctparams
#    4. build system
#    5. slaveplatform
#    6. generator
def main(argv):
    print ('{}:{}:{}:{}:{}:{}:{}:{}'.format(sys.argv[1],sys.argv[2],sys.argv[3],sys.argv[4],sys.argv[5],sys.argv[6],sys.argv[7],sys.argv[8]))

    configfile = sys.argv[1]
    buildconfig = sys.argv[2]
    ctparams = sys.argv[3]
    buildsys = sys.argv[4]
    theos = sys.argv[5]
    slaveplatform = sys.argv[6]
    generator = sys.argv[7]
    compilername = VS_full_to_short(sys.argv[7])
    slavename = sys.argv[8]
    buildtype = 'Release'

    if 'bbparams' == ctparams:
        hdfsrc = '../hdfsrc'
        hdfbld = './hdfbld'
        hdfpack = 'hdf'
    else:
        hdfsrc = '../hdfsrc/%s'  % (ctparams)
        hdfbld = './hdfbld/%s'  % (ctparams)
        hdfpack = '%s'  % (ctparams)

    currdir=os.getcwd() ####
    print ('current={}'.format(currdir))

    with open(configfile) as json_file:
        json_data = json.load(json_file)
    #print(json_data)
    #  get config buildtype
    for testparam in json_data[buildconfig]['bbparams']['testparams']:
        if 'buildtype' in testparam:
            buildtype = testparam['buildtype']
    print ('bb buildtype={}'.format(buildtype))
    # check if config buildtype is overridden by buildsys buildtype
    for testparam in json_data[buildconfig][ctparams]['buildsys'][buildsys]['testparams']:
        if 'buildtype' in testparam:
            buildtype = testparam['buildtype']

    if not os.path.exists('%s' % (hdfpack)):
        os.mkdir('%s' % (hdfpack))

    with open(configfile) as json_file:
        json_data = json.load(json_file)
    #print(json_data)

    binary_fname = ''
    for btparam in json_data[buildconfig][ctparams]['buildsys'][buildsys]:
        if 'envparams' in btparam:
            envparams = json_data[buildconfig][ctparams]['buildsys'][buildsys]['envparams']
            #print ('tconf env == {}'.format(envparams))
            for envparam in envparams:
                if 'srcbinary' == envparam:
                    for binfile in envparams['srcbinary']:
                        for k,v in binfile.items():
                            if slaveplatform.lower() in ['vs11', 'vs12', 'vs14']:
                                binary_fname = v % '-win7_' + compilername
                            elif slaveplatform.lower() in ['10vs12', '10vs14']:
                                binary_fname = v % '-win10_' + compilername
                            else:
                                if 'ctest' == buildsys:
                                    binary_fname = cmake_to_binary(v, slaveplatform.lower(), theos.lower())
                                else:
                                    binary_fname = linux_to_binary(v, slaveplatform.lower(), theos.lower())

    if slaveplatform.lower() in ['vs11', 'vs12', 'vs14', '10vs12', '10vs14']:
        binary_ext = '.zip'
    else:
        binary_ext = '.tar.gz'
    binary_file = binary_fname + binary_ext
    print ('binary_name={}'.format(binary_fname))

    for file in os.listdir('%s' % (hdfbld)):
        if slaveplatform.lower() in ['vs11', 'vs12', 'vs14', '10vs12', '10vs14']:
            if file.endswith('.zip'):
                copy2 (os.path.join('%s' % (hdfbld), file), '%s' % (binary_file))
                print ('copied: {}'.format(file))
                break;
        else:
            if file.endswith('.tar.gz'):
                copy2 (os.path.join('%s' % (hdfbld), file), '%s' % (binary_file))
                print ('copied: {}'.format(file))
                break;

    os.chmod('%s' % (binary_file), 0o777)

    print ('copied to {} : done'.format(binary_file))
    return

if __name__ == '__main__':
    main(sys.argv)
