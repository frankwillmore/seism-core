#!/usr/bin/env python

import sys
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
#    1. subfolder of file in HOST:DIRN
#    2. local folder for file
#    3. filename target

def main(argv):
    print ('{}:{}:{}:{}:{}'.format(sys.argv[1],sys.argv[2],sys.argv[3],sys.argv[4],sys.argv[5]))
    HOST = sys.argv[1]
    DIRN = sys.argv[2]
    qatestpath = sys.argv[3]
    localdir = sys.argv[4]
    targetname = sys.argv[5]

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
            if not os.path.exists('%s' % localdir):
                os.mkdir('%s' % localdir)
            os.chdir('%s' % localdir)
            filename = '%s' % targetname
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

