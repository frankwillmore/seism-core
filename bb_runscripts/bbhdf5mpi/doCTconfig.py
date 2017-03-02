#!/usr/bin/env python

import sys, json, os

def ctest_sets(conffile, confsteps):
    conffile.write('###################################################################\n')
    for confs in confsteps:
        for k,v in confs.items():
            conffile.write('set(%s %s)\n' % (k,v))
    conffile.write('###################################################################\n')

def ctest_options(conffile, confsteps, HAVE_FORTRAN):
    conffile.write('###################################################################\n')
    for confs in confsteps:
        for o,t in confs.items():
            for k,v in t.items():
                if 'FORTRAN' in o:
                    if HAVE_FORTRAN == 'ON':
                        conffile.write('set(ADD_BUILD_OPTIONS "${ADD_BUILD_OPTIONS} -D%s:%s=%s")\n' % (o,k,v))
                    else:
                        conffile.write('set(ADD_BUILD_OPTIONS "${ADD_BUILD_OPTIONS} -D%s:%s=%s")\n' % (o,k,HAVE_FORTRAN))
                else:
                    conffile.write('set(ADD_BUILD_OPTIONS "${ADD_BUILD_OPTIONS} -D%s:%s=%s")\n' % (o,k,v))
    conffile.write('###################################################################\n')


# arguments:
#    1. configuration file name
#    2. ctconfig
#    3. ctparams
#    4. build system
#    5. theplatform
#    6. compiler type
#    7. compiler name
def main(argv):
    print ('{}:{}:{}:{}:{}:{}:{}'.format(sys.argv[1],sys.argv[2],sys.argv[3],sys.argv[4],sys.argv[5],sys.argv[6],sys.argv[7]))
    configfile = sys.argv[1]
    buildconfig = sys.argv[2]
    ctparams = sys.argv[3]
    buildsys = sys.argv[4]
    theplatform = sys.argv[5]
    schedule = sys.argv[6]

    if 'Fortran' in argv[7:]:
        HAVE_FORTRAN = 'ON'
    else:
        HAVE_FORTRAN = 'OFF'

    with open(configfile) as json_file:
        json_data = json.load(json_file)
    #print(json_data)

    if 'nightly' in schedule.lower() and not 'ec2' in schedule.lower():
        cdashmodel = 'Nightly'
    else:
        cdashmodel = 'BuildBot'

    for ctparam in json_data[buildconfig][ctparams]['buildsys'][buildsys]:
        if 'envparams' in ctparam:
            envparams = json_data[buildconfig][ctparams]['buildsys'][buildsys]['envparams']
            #print ('tconf env == {}'.format(envparams))
            for envparam in envparams:
                if 'cdash_model' == envparam:
                    if 'ec2' in schedule.lower():
                        cdashmodel = 'BuildBot'
                    else:
                        cdashmodel = envparams['cdash_model']

    confilename = 'HDFoptions.cmake'
    conffile = open(confilename,'w')
    conffile.write('###################################################################\n')
    conffile.write('#########       Following is for submission to CDash   ############\n')
    conffile.write('###################################################################\n')
    conffile.write('set(MODEL "%s")\n' % cdashmodel)
    conffile.write('#####       Following controls CDash submission               #####\n')
    conffile.write('set(LOCAL_SUBMIT "TRUE")\n')
    conffile.write('###################################################################\n')

    for testparam in json_data[buildconfig][ctparams]['buildsys'][buildsys]['testparams']:
        if 'ctest_opts' in testparam:
            ctest_sets(conffile, testparam['ctest_opts'])
        elif 'ctest_conf' in testparam:
            ctest_options(conffile, testparam['ctest_conf'], HAVE_FORTRAN)

    if theplatform.startswith("10VS"):
        conffile.write('set(SITE_OS_VERSION "10")\n')
    elif theplatform.startswith("VS"):
        conffile.write('set(SITE_OS_VERSION "7")\n')

    conffile.write('set(SITE_BUILDNAME_SUFFIX "' + schedule + '-${SITE_BUILDNAME_SUFFIX}")\n')
    conffile.close()

    if not os.path.exists('%s' % confilename):
        raise Exception('ctest config failure: file not found')

if __name__ == '__main__':
    main(sys.argv)
