#!/usr/bin/env python

import sys
import os
import subprocess
from shutil import copy2
# arguments:
#    1. local folder

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
    print sys.argv[1]
    installocation = sys.argv[1]
    # following is needed for options to binary test script - currently unused
    configfile = sys.argv[2]

    # with open(configfile) as json_file:
    #     json_data = json.load(json_file)
    #print(json_data)

    builddir=os.getcwd() ####
    print ('current={}'.format(builddir))

    dir_list = get_filepaths(installocation)

    installname = ''
    deployname = ''

    parts = []
    for f in dir_list:
        if f.endswith("deploy"):
            parts = os.path.split(f)
            deployname = f
            print ('{} with {} - {}'.format(deployname,parts[0],parts[1]))
            break
    if deployname:
        os.chdir(parts[0])
        print ('execute current={}'.format(os.getcwd()))
        os.system('./'+parts[1]+' -force')
    else:
        raise ValueError('redeploy script failed.')

    os.chdir(builddir) ####

    print (' at {}'.format(installocation))
    return

if __name__ == '__main__':
    main(sys.argv)

