#!/usr/bin/env python

import sys, json
import pickle
import os
import subprocess
import multiprocessing

def autotools_options(conffile, confsteps):
    conffile.write('###################################################################\n')
    for confs in confsteps:
        for k,v in confs.items():
                project_make.append('-%s' % (v))
                conffile.write(' -%s' % (v))
    conffile.write('\n###################################################################\n')
    return project_make



# arguments:
#    1. configuration file name
#    2. atconfig
#    3. atparams
#    4. os
#    5. slave name
#    6... slave compiler types
def main(argv):
    print ('{}:{}:{}:{}:{}:{}'.format(sys.argv[1],sys.argv[2],sys.argv[3],sys.argv[4],sys.argv[5],sys.argv[6]))
    configfile = sys.argv[1]
    atconfig = sys.argv[2]
    atparams = sys.argv[3]
    slaveos = sys.argv[4]
    slavename = sys.argv[5]
    compilers = sys.argv[6:]

    project_make = ['make']
    confilename = 'HDFmakecheck.txt'
    conffile = open(confilename,'w')
    conffile.write('###################################################################\n')
    conffile.write('#########   Following is for autotools make check      ############\n')
    conffile.write('###################################################################\n')

    with open(configfile) as json_file:
        json_data = json.load(json_file)
    #print(json_data)
    for autotoolsparam in json_data[atconfig][atparams]['buildsys']['autotools']:
        if 'checkparams' in autotoolsparam:
            project_make = autotools_options(conffile, json_data[atconfig][atparams]['buildsys']['autotools']['checkparams'])

    conffile.write('\n')
    conffile.close()

    currdir=os.getcwd() ####
    print ('current={}'.format(currdir))
    checkname = 'check'
    ldpath = ''

    if 'checkoptions' in json_data[atconfig][atparams]['buildsys']['autotools']:
        checkoptions = json_data[atconfig][atparams]['buildsys']['autotools']['checkoptions']
        print ('checkoptions={}'.format(checkoptions))

    if 'envparams' in json_data[atconfig][atparams]['buildsys']['autotools']:
        envparams = json_data[atconfig][atparams]['buildsys']['autotools']['envparams']
        for envitem in envparams:
            if 'MAKEFILE' in envitem:
                project_make.append('-f')
                project_make.append('%s' % envitem['MAKEFILE'])
            if 'parallelgen' in envitem:
                count = multiprocessing.cpu_count()
                project_make.append('-j%d' % count)
            if 'check-vfd' in envitem:
                checkname = 'check-vfd'
            if 'librarypath' in envitem:
                for atenv in envparams['librarypath']:
                    if slaveos in atenv:
                        for atos in atenv[slaveos]:
                            ldpath = '%s' % (atenv[slaveos])

    project_make.append('%s' % (checkname))

    if not 'bbparams' == atparams:
        if not os.path.exists(atparams):
            os.mkdir(atparams)
        os.chdir(atparams)
        
    thereturncode = 0
    print('make cmd={}'.format(project_make))
    if (len(project_make) > 0):
        my_env = os.environ.copy()
        if len(ldpath) > 0:
            my_env["LD_LIBRARY_PATH"] = str('%s:%s' % (ldpath, my_env["LD_LIBRARY_PATH"]))
        test = subprocess.Popen(project_make, shell=False, env=my_env)
        test.wait()
        output = test.communicate()[0]
        thereturncode = test.returncode

    os.chdir(currdir) ####

    if not thereturncode == 0:
        raise Exception('make failure: ', thereturncode)

    print ('make with check:{} : {}'.format(project_make,thereturncode))
    return thereturncode

if __name__ == '__main__':
    main(sys.argv)
