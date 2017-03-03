#!/usr/bin/env python

import sys, json
import pickle
import os
import subprocess
from shutil import copy2

def copytree(src, dst):
    if not os.path.exists(dst):
        os.makedirs(dst)
    for item in os.listdir(src):
        s = os.path.join(src, item)
        d = os.path.join(dst, item)
        if os.path.isdir(s):
            copytree(s, d)
        else:
            if not os.path.exists(d) or os.stat(s).st_mtime - os.stat(d).st_mtime > 1:
                copy2(s, d)

def autotools_options(conffile, slaveos, confsteps, HAVE_FORTRAN):
    project_configureFlags = ['../hdfsrc/configure']
    conffile.write('###################################################################\n')
    for confs in confsteps:
        for k,v in confs.items():
            if 'fortran' == k.lower() and 'enable' == v.lower() and 'OFF' == HAVE_FORTRAN:
                project_configureFlags.append('--disable-%s' % k)
                conffile.write(' --disable-fortran')
            elif k.startswith('with'):
                for vos in v:
                    if slaveos in vos:
                        project_configureFlags.append('--%s=%s' % (k,vos[slaveos]))
                        conffile.write(' --%s=%s' % (k,vos[slaveos]))
                    else:
                        project_configureFlags.append('--%s' % (k))
                        conffile.write(' --%s' % (k))
            elif 'libdir' == k.lower():
                project_configureFlags.append('--%s=%s' % (k,v))
                conffile.write(' --%s=%s' % (k,v))
            else:
                project_configureFlags.append('--%s-%s' % (v,k))
                conffile.write(' --%s-%s' % (v,k))
    conffile.write('\n###################################################################\n')
    return project_configureFlags


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

    project_configureFlags = []
    confilename = 'HDFoptions.txt'
    conffile = open(confilename,'w')
    conffile.write('###################################################################\n')
    conffile.write('#########       Following is for autotools config      ############\n')
    conffile.write('###################################################################\n')

    if 'Fortran' in compilers:
        HAVE_FORTRAN = 'ON'
    else:
        HAVE_FORTRAN = 'OFF'

    with open(configfile) as json_file:
        json_data = json.load(json_file)
    #print(json_data)
    for autotoolsparam in json_data[atconfig][atparams]['buildsys']['autotools']:
        if 'confparams' in autotoolsparam:
            project_configureFlags = autotools_options(conffile, slaveos, json_data[atconfig][atparams]['buildsys']['autotools']['confparams'], HAVE_FORTRAN)
        if 'copyparams' in autotoolsparam:
            for copyconfs in json_data[atconfig][atparams]['buildsys']['autotools']['copyparams']:
                for k,v in copyconfs.items():
                    if not os.path.exists('%s' % v):
                        os.mkdir('%s' % v)
                    copytree (k, v)

    conffile.write('\n')
    conffile.close()

    currdir=os.getcwd() ####
    print ('current={}'.format(currdir))

    if 'testoptions' in json_data[atconfig][atparams]['buildsys']['autotools']:
        testoptions = json_data[atconfig][atparams]['buildsys']['autotools']['testoptions']
        print ('testoptions={}'.format(testoptions))

    if 'envparams' in json_data[atconfig][atparams]['buildsys']['autotools']:
        envparams = json_data[atconfig][atparams]['buildsys']['autotools']['envparams']
        for envitem in envparams:
            if 'autogen' in envitem:
                my_env = os.environ.copy()

                for atenv in envparams['autogen']:
                    if slaveos in atenv:
                        for atos in atenv[slaveos]:
                            for k,v in atos.items():
                                if k == 'PATH':
                                    my_env["PATH"] = str('%s:%s' % (v, my_env["PATH"]))
                                else:
                                    my_env[k] = str('%s' % (v))

                os.chdir('../hdfsrc') ####
                autogencommand=['sh','./autogen.sh']
                test = subprocess.Popen(autogencommand, shell=False, env=my_env)
                output = test.communicate()[0]
                os.chdir(currdir) ####

    if not 'bbparams' == atparams:
        if not os.path.exists(atparams):
            os.mkdir(atparams)
        os.chdir(atparams)

    thereturncode = 0
    print(project_configureFlags)
    if (len(project_configureFlags) > 0):
        test = subprocess.Popen(project_configureFlags, shell=False)
        test.wait()
        output = test.communicate()[0]
        thereturncode = test.returncode

    os.chdir(currdir) ####

    if not thereturncode == 0:
        raise Exception('config failure: ', thereturncode)

    print ('configure with options:{} : {}'.format(project_configureFlags,thereturncode))
    return thereturncode

if __name__ == '__main__':
    main(sys.argv)
