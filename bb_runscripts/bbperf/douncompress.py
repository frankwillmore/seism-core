#!/usr/bin/env python

import os.path, sys
import zipfile, tarfile

# arguments:
#    1. install folder for file
#    2. filename target
def main(argv):
    print ('{}:{}'.format(sys.argv[1],sys.argv[2]))
    install_folder = sys.argv[1]
    binary_fname = sys.argv[2]

    currdir=os.getcwd()
    if not os.path.exists('%s' % install_folder):
        os.mkdir('%s' % install_folder)
    os.chdir('%s' % install_folder)

    fileExtension = os.path.splitext(binary_fname)[1][1:].strip().lower()
    if 'zip' == fileExtension:
        try:
            cf = zipfile.ZipFile(binary_fname)
            cf.extractall(install_folder)
            cf.close()
            print ('zip Done.')
        except:
            e = sys.exc_info()[0]
            print ('zip <p>Error: %s</p>' % e)
    elif 'gz' == fileExtension:
        try:
            cf = tarfile.open(binary_fname, 'r:gz')
            cf.extractall()
            cf.close()
            print ('gz Done.')
        except:
            e = sys.exc_info()[0]
            print ('gz <p>Error: %s</p>' % e)
    else:
        try:
            cf = tarfile.open(binary_fname, 'r')
            cf.extractall()
            cf.close()
            print ('other Done.')
        except:
            e = sys.exc_info()[0]
            print ('other <p>Error: %s</p>' % e)
    os.chdir(currdir)
    return

if __name__ == '__main__':
    main(sys.argv)
