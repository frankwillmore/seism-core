#!/usr/bin/env python

import os.path, sys, json
import zipfile, tarfile

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

#    1. os of slave
#    2. platform of slave
#    3. configuration
#    4. ctparams
#    5. build system
#    6. compiler name
#    7. local folder for file
#    8. btconfig
def main(argv):
    print ('{}:{}:{}:{}:{}:{}:{}:{}'.format(sys.argv[1],sys.argv[2],sys.argv[3],sys.argv[4],sys.argv[5],sys.argv[6],sys.argv[7],sys.argv[8]))
    slaveos = sys.argv[1]
    slaveplatform = sys.argv[2]
    buildconfig = sys.argv[3]
    ctparams = sys.argv[4]
    buildsys = sys.argv[5]
    compilername = VS_full_to_short(sys.argv[6])
    install_folder = sys.argv[7]
    configfile = sys.argv[8]

    with open(configfile) as json_file:
        json_data = json.load(json_file)
    #print(json_data)

    binary_fname = ''
    for btparam in json_data[buildconfig][ctparams]['buildsys'][buildsys]:
        if 'envparams' in btparam:
            envparams = json_data[buildconfig][ctparams]['buildsys'][buildsys]['envparams']
            #print ('tconf env == {}'.format(envparams))
            for envparam in envparams:
                if 'binary' == envparam:
                    for binfile in envparams['binary']:
                        for k,v in binfile.items():
                            print ('binfile of {} is {}'.format(k,v))
                            if slaveplatform.lower() in ['vs11', 'vs12', 'vs14']:
                                binary_fname = v % '-win7_' + compilername
                            elif slaveplatform.lower() in ['10vs12', '10vs14']:
                                binary_fname = v % '-win10_' + compilername
                            else:
                                if buildsys in ['btctest', 'ctest', 'ant']:
                                    binary_fname = cmake_to_binary(v, slaveplatform.lower(), slaveos.lower())
                                else:
                                    binary_fname = linux_to_binary(v, slaveplatform.lower(), slaveos.lower())

                            if slaveplatform.lower() in ['vs11', 'vs12', 'vs14', '10vs12', '10vs14']:
                                binary_ext = '.zip'
                            else:
                                binary_ext = '.tar.gz'
                            binary_file = binary_fname + binary_ext

                            currdir=os.getcwd()
                            if not os.path.exists('%s' % install_folder):
                                os.mkdir('%s' % install_folder)
                            os.chdir('%s' % install_folder)

                            fileExtension = os.path.splitext(binary_file)[1][1:].strip().lower()
                            if 'zip' == fileExtension:
                                try:
                                    cf = zipfile.ZipFile(binary_file)
                                    cf.extractall('../'+install_folder)
                                    cf.close()
                                    print ('zip Done.')
                                except:
                                    e = sys.exc_info()[0]
                                    print ( "zip <p>Error: %s</p>" % e )
                            elif 'gz' == fileExtension:
                                try:
                                    cf = tarfile.open(binary_file, 'r:gz')
                                    cf.extractall()
                                    cf.close()
                                    print ('gz Done.')
                                except:
                                    e = sys.exc_info()[0]
                                    print ( "gz <p>Error: %s</p>" % e )
                            else:
                                try:
                                    cf = tarfile.open(binary_file, 'r')
                                    cf.extractall()
                                    cf.close()
                                    print 'other Done.'
                                except:
                                    e = sys.exc_info()[0]
                                    print ( "other <p>Error: %s</p>" % e )
                            os.chdir(currdir)
    return

if __name__ == '__main__':
    main(sys.argv)
