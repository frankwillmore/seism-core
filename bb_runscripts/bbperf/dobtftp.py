#!/usr/bin/env python

import sys, json
import os
import socket
import paramiko

def create_sftp_client(host, port, username, password, keyfilepath, keyfiletype):
    """
    create_sftp_client(host, port, username, password, keyfilepath, keyfiletype) -> SFTPClient

    Creates a SFTP client connected to the supplied host on the supplied port authenticating as the user with
    supplied username and supplied password or with the private key in a file with the supplied path.
    If a private key is used for authentication, the type of the keyfile needs to be specified as DSA or RSA.
    :rtype: SFTPClient object.
    """
    sftp = None
    key = None
    transport = None
    try:
        if keyfilepath is not None:
            # Get private key used to authenticate user.
            if keyfiletype == 'DSA':
                # The private key is a DSA type key.
                key = paramiko.DSSKey.from_private_key_file(keyfilepath)
            else:
                # The private key is a RSA type key.
                key = paramiko.RSAKey.from_private_key_file(keyfilepath)

        print 'retrieved key'
        # Create Transport object using supplied method of authentication.
        transport = paramiko.Transport((host, port))
        print 'transport created'
        transport.connect(None, username, password, key)

        print 'connected'
        sftp = paramiko.SFTPClient.from_transport(transport)

        return sftp
    except Exception as e:
        print('An error occurred creating SFTP client: %s: %s' % (e.__class__, e))
        if sftp is not None:
            sftp.close()
        if transport is not None:
            transport.close()
        pass


def create_sftp_client2(host, port, username, password, keyfilepath, keyfiletype):
    """
    create_sftp_client(host, port, username, password, keyfilepath, keyfiletype) -> SFTPClient

    Creates a SFTP client connected to the supplied host on the supplied port authenticating as the user with
    supplied username and supplied password or with the private key in a file with the supplied path.
    If a private key is used for authentication, the type of the keyfile needs to be specified as DSA or RSA.
    :rtype: SFTPClient object.
    """
    ssh = None
    sftp = None
    key = None
    try:
        if keyfilepath is not None:
            # Get private key used to authenticate user.
            if keyfiletype == 'DSA':
                # The private key is a DSA type key.
                key = paramiko.DSSKey.from_private_key_file(keyfilepath)
            else:
                # The private key is a RSA type key.
                key = paramiko.RSAKey.from_private_key_file(keyfilepath)

        print 'retrieved key'
        # Connect SSH client accepting all host keys.
        ssh = paramiko.SSHClient()
        print 'SSHClient created'
        ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
        ssh.connect(host, port, username, password, key)
        print 'connected'

        # Using the SSH client, create a SFTP client.
        sftp = ssh.open_sftp()
        # Keep a reference to the SSH client in the SFTP client as to prevent the former from
        # being garbage collected and the connection from being closed.
        sftp.sshclient = ssh

        return sftp
    except Exception as e:
        print('An error occurred creating SFTP client: %s: %s' % (e.__class__, e))
        if sftp is not None:
            sftp.close()
        if ssh is not None:
            ssh.close()
        pass

# arguments:
#    3. os of slave
#    4. platform of slave
#    5. configuration
#    6. ctparams
#    7. build system
#    8. compiler name
#    9. local folder for file
#    10. btconfig

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

def main(argv):
    print ('{}:{}:{}:{}:{}:{}:{}:{}:{}:{}'.format(sys.argv[1],sys.argv[2],sys.argv[3],sys.argv[4],sys.argv[5],sys.argv[6],sys.argv[7],sys.argv[8],sys.argv[9],sys.argv[10]))
    HOST = sys.argv[1]
    DIRN = sys.argv[2]
    slaveos = sys.argv[3]
    slaveplatform = sys.argv[4]
    buildconfig = sys.argv[5]
    ctparams = sys.argv[6]
    buildsys = sys.argv[7]
    compilername = VS_full_to_short(sys.argv[8])
    install_folder = sys.argv[9]
    configfile = sys.argv[10]

    with open(configfile) as json_file:
        json_data = json.load(json_file)
    #print(json_data)

    binary_fname = ''
    qatestdir = ''
    for btparam in json_data[buildconfig][ctparams]['buildsys'][buildsys]:
        if 'envparams' in btparam:
            envparams = json_data[buildconfig][ctparams]['buildsys'][buildsys]['envparams']
            #print ('tconf env == {}'.format(envparams))
            for envparam in envparams:
                if 'binary' == envparam:
                    for binfile in envparams['binary']:
                        for k,v in binfile.items():
                            qatestdir = k
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
                                qatestpath = qatestdir + 'windows'
                                binary_ext = '.zip'
                            else:
                                qatestpath = qatestdir + 'unix'
                                binary_ext = '.tar.gz'
                            binary_file = binary_fname + binary_ext

                            sftpclient = create_sftp_client2(HOST, 22, 'hdftest', None, os.path.expanduser(os.path.join("~", ".ssh", "bitbidkey")), 'RSA')
                            if sftpclient is not None:
                                try:
                                    sftpclient.chdir(DIRN + '%s' % qatestpath)
                                except Exception as e:
                                    print ('ERROR: cannot CD to "%s"' % DIRN)
                                    if sftpclient is not None:
                                        sftpclient.close()
                                print ('*** Changed to folder: "%s"' % DIRN)

                                currdir=os.getcwd() ####

                                try:
                                    if not os.path.exists('%s' % install_folder):
                                        os.mkdir('%s' % install_folder)
                                    os.chdir('%s' % install_folder)
                                    filename = '%s' % (binary_file)
                                    print ('Getting {}'.format(filename))
                                    sftpclient.get(filename, filename)
                                    print ('File {} downloaded'.format(filename))
                                except Exception as e:
                                    print ('ERROR: cannot read file "%s"' % filename)

                                os.chdir(currdir) ####
                                sftpclient.close()
    return

if __name__ == '__main__':
    main(sys.argv)

