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

        print ('retrieved key')
        # Create Transport object using supplied method of authentication.
        transport = paramiko.Transport((host, port))
        print ('transport created')
        transport.connect(None, username, password, key)

        print ('connected')
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

        print ('retrieved key')
        # Connect SSH client accepting all host keys.
        ssh = paramiko.SSHClient()
        print ('SSHClient created')
        ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
        ssh.connect(host, port, username, password, key)
        print ('connected')

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
#    8. subfolder of file in HOST:DIRN
#    9. tconf file

def main(argv):
    print ('{}:{}:{}:{}:{}:{}:{}:{}:{}'.format(sys.argv[1],sys.argv[2],sys.argv[3],sys.argv[4],sys.argv[5],sys.argv[6],sys.argv[7],sys.argv[8],sys.argv[9]))
    HOST = sys.argv[1]
    DIRN = sys.argv[2]
    slaveos = sys.argv[3]
    slaveplatform = sys.argv[4]
    buildconfig = sys.argv[5]
    ctparams = sys.argv[6]
    buildsys = sys.argv[7]
    qatestpath = sys.argv[8]
    configfile = sys.argv[9]

    sftpclient = create_sftp_client2(HOST, 22, 'hdftest', None, os.path.expanduser(os.path.join("~", ".ssh", "bitbidkey")), 'RSA')
    if sftpclient is not None:
        try:
            sftpclient.chdir(DIRN + '%s' % qatestpath)
        except Exception as e:
            print ('ERROR: cannot CD to "%s"' % DIRN)
            if sftpclient is not None:
                sftpclient.close()
        print ('*** Changed to folder: "%s"' % DIRN)

        with open(configfile) as json_file:
            json_data = json.load(json_file)
        #print(json_data)

        currdir=os.getcwd() ####

        files_fname = ''
        qatestdir = ''
        for btparam in json_data[buildconfig][ctparams]['buildsys'][buildsys]:
            if 'envparams' in btparam:
                envparams = json_data[buildconfig][ctparams]['buildsys'][buildsys]['envparams']
                #print ('tconf env == {}'.format(envparams))
                for envparam in envparams:
                    if 'files' == envparam:
                        for files in envparams['files']:
                            for k,v in files.items():
                                #print ('{}-{} in {}'.format(k,v,args['ctconfig']))
                                try:
                                    filename = '%s' % (v)
                                    print ('Getting {}'.format(filename))
                                    sftpclient.get(filename, filename)
                                        #s = s + 1
                                except Exception as e:
                                    print ('ERROR: cannot read file "%s"' % filename)

                                if os.path.exists(k) and not k == v:
                                    os.remove(k)
                                os.rename(v, k)
                                print ('File {} downloaded as {}'.format(v,k))

        os.chdir(currdir) ####
        sftpclient.close()
    return

if __name__ == '__main__':
    main(sys.argv)

