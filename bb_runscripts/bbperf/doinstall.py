#!/usr/bin/env python

import sys
import os, json
import subprocess

# arguments:
#    1. os of slave
#    2. local folder for file
#    3. install or uninstall

def get_directory_structure(rootdir):
    """
    Creates a nested dictionary that represents the folder structure of rootdir
    """
    thedir = {}
    rootdir = rootdir.rstrip(os.sep)
    start = rootdir.rfind(os.sep) + 1
    for path, dirs, files in os.walk(rootdir):
        folders = path[start:].split(os.sep)
        subdir = dict.fromkeys(files)
        parent = reduce(dict.get, folders[:-1], thedir)
        parent[folders[-1]] = subdir
    return thedir

def get_filepaths(directory):
    """
    This function will generate the file names in a directory
    tree by walking the tree either top-down or bottom-up. For each
    directory in the tree rooted at directory top (including top itself),
    it yields a 3-tuple (dirpath, dirnames, filenames).
    """
    file_paths = []  # List which will store all of the full filepaths.

    # Walk the tree.
    for root, directories, files in os.walk(directory):
        for filename in files:
            # Join the two strings in order to form the full filepath.
            filepath = os.path.join(os.path.normpath(root), filename)
            file_paths.append(filepath)  # Add it to the list.

    return file_paths  # Self-explanatory.

def main(argv):
    print ('{}:{}:{}:{}:{}:{}:{}'.format(sys.argv[1],sys.argv[2],sys.argv[3],sys.argv[4],sys.argv[5],sys.argv[6],sys.argv[7]))
    installos = sys.argv[1].strip()
    buildconfig = sys.argv[2]
    ctparams = sys.argv[3]
    configfile = sys.argv[4]
    buildsys = sys.argv[5]
    installfolder = sys.argv[6].strip()
    installform = sys.argv[7].strip()

    with open(configfile) as json_file:
        json_data = json.load(json_file)
    #print(json_data)

    if ctparams == 'bbparams':
        for btparam in json_data[buildconfig][ctparams]['buildsys'][buildsys]:
            if 'envparams' in btparam:
                envparams = json_data[buildconfig][ctparams]['buildsys'][buildsys]['envparams']
                #print ('tconf env == {}'.format(envparams))
                for envparam in envparams:
                    if 'binaryfolder' == envparam:
                        installfolder += '/' + envparams['binaryfolder']
    else:
        installfolder += '/' + ctparams

    currdir=os.getcwd() ####

    dir_list = get_filepaths(installfolder)

    returncode = 0
    installname = ''
    output = ''
    if installos.lower() in ['vs11', 'vs12', 'vs14', '10vs12', '10vs14']:
        for f in dir_list:
            #print ('f = {}'.format(f))
            if f.endswith(".msi"):
                installname = f
                print ('installing {}'.format(installname))
                if 'install' == installform.lower():
                    test = subprocess.Popen(['msiexec.exe', '/c', '/package', installname, '/quiet'], shell=True)
                    output = test.communicate()[0]
                else:
                    test = subprocess.Popen(['msiexec.exe', '/c', '/uninstall', installname, '/quiet'], shell=True)
                    output = test.communicate()[0]

        if len(installname) == 0:
            raise ValueError('cannot find *.msi file')
    else:
        for f in dir_list:
            #print ('f = {}'.format(f))
            if f.endswith(".sh"):
                installname = f
                if 'install' == installform.lower():
                    test = subprocess.Popen([installname, '--exclude-subdir', '--skip-license'], shell=False)
                    output = test.communicate()[0]
                    returncode = test.returncode

        if len(installname) == 0:
            raise ValueError('cannot find *.sh file')
    os.chdir(currdir) ####

    print ('File {} : {}'.format(installname,output))
    return returncode

if __name__ == '__main__':
    main(sys.argv)

