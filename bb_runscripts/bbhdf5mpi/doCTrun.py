#!/usr/bin/env python

import sys, json
import os
import subprocess
from shutil import copy2

def VS_full_to_VScompile(argument):
    #print(argument)
    switcher = {
        "Visual Studio 11 2012": "32_VS2012",
        "Visual Studio 11 2012 Win64": "64_VS2012",
        "Visual Studio 12 2013": "32_VS2013",
        "Visual Studio 12 2013 Win64": "64_VS2013",
        "Visual Studio 14 2015": "32_VS2015",
        "Visual Studio 14 2015 Win64": "64_VS2015",
    }
    return switcher.get(argument, "nothing")

# arguments:
#    1. configuration file name
#    2. ctconfig
#    3. ctparams
#    4. build system
#    5. theplatform
#    6. generator
def main(argv):
    print ('{}:{}:{}:{}:{}:{}'.format(sys.argv[1],sys.argv[2],sys.argv[3],sys.argv[4],sys.argv[5],sys.argv[6]))

    confilename = 'HDFoptions.cmake'
    configfile = sys.argv[1]
    buildconfig = sys.argv[2]
    ctparams = sys.argv[3]
    buildsys = sys.argv[4]
    theplatform = sys.argv[5]
    generator = sys.argv[6]
    buildtype = 'Release'

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
    print ('bb2 buildtype={}'.format(buildtype))
    shellState = False
    winver = ''
    if theplatform.lower() in ['vs11', 'vs12', 'vs14', '10vs12', '10vs14']:
        if 'bbparams' == ctparams:
            winver = '%s' % (VS_full_to_VScompile(generator))
        else:
            winver = '%s;%s' % (VS_full_to_VScompile(generator), ctparams)
        shellState = True
    else:
        if 'bbparams' == ctparams:
            winver = 'unix'
        else:
            winver = 'unix;%s' % (ctparams)

    my_env = os.environ.copy()
    my_env["CMAKE_CONFIG_TYPE"] = str('%s' % (buildtype))
    test = subprocess.Popen(['ctest', '-version'], shell=shellState, env=my_env)
    output = test.communicate()[0]
    test = subprocess.Popen(['ctest', '-S', 'HDF.cmake,'+ winver, '-C', buildtype, '-VV'], shell=shellState, env=my_env)
    output = test.communicate()[0]

    if os.path.exists('%s/FailedCTest.txt' % currdir):
        test.returncode = 255
        with open('%s/FailedCTest.txt' % currdir) as f:
            print ('ctest failure: {}'.format(f.read()))
        raise Exception('ctest failure: ', test.returncode)

    return test.returncode

if __name__ == '__main__':
    main(sys.argv)
