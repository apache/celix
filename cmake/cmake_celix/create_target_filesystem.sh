# Licensed to the Apache Software Foundation (ASF) under one or more
# contributor license agreements.  See the NOTICE file distributed with
# this work for additional information regarding copyright ownership.
# The ASF licenses this file to You under the Apache License, Version 2.0
# (the "License"); you may not use this file except in compliance with
# the License.  You may obtain a copy of the License at
#    
#     http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

#!/bin/bash

declare -A EXE_ARGUMENTS
EXE_INDEX=0
declare -A LIB_ARGUMENTS
LIB_INDEX=0
declare -A BUNDLE_ARGUMENTS
BUNDLE_INDEX=0

while [ -n "${1}" ]
do
	ARG="${1}"
	case $ARG in
	    -h|--help)
	    echo "Usage ${0} [-e executable].. [-l library].. [-b bundle].."
	    exit 0
	    ;;
	    -e|--executable)
	    EXE_ARGUMENTS[${EXE_INDEX}]="${2}"
	    EXE_INDEX=$((EXE_INDEX+1))
	    shift
	    ;;
	    -l|--lib)
	    LIB_ARGUMENTS[${LIB_INDEX}]="${2}"
	    LIB_INDEX=$((LIB_INDEX+1))
	    shift
	    ;;
	    -b|--bundle)
	    BUNDLE_ARGUMENTS[${BUNDLE_INDEX}]="${2}"
	    BUNDLE_INDEX=$((BUNDLE_INDEX+1))
	    shift
	    ;;
	    *)
	    echo "unknown option '${ARG}'"
	    ;;
	esac
	shift
done

celix_add_file() {
	FILE=$1
	TO_DIR=$2
	if [ -f ${FILE} ]
	then #is regular file and not symlink
		if [ -n "${TO_DIR}" ]; then
			DIR=${TO_DIR}
		elif [[ "${FILE}" =~ ^/.* ]]; then #absolute path
			DIR=.$(dirname ${FILE})
		else
			DIR=$(dirname ${FILE})
		fi

		mkdir -p ${DIR} 2> /dev/null
		cp -vu ${FILE} ${DIR}/ 
	else
		echo "Skipping file ${FILE}"
	fi
}

celix_add_bundle() {
	BUNDLE=$1
	if [ -e ${BUNDLE} ] 
	then
		BUNDLE_DIR=bundles
		mkdir -p ${BUNDLES_DIR} 2> /dev/null
		cp -vu ${BUNDLE} ${BUNDLES_DIR}/
	else
		echo "Bundle '${BUNDLE}' does not exists!"
	fi
}

celix_add_lib() {
	LIB=$1
	for DEP in $(ldd ${LIB} | grep lib | awk '{print $3}')
	do
		celix_add_file ${DEP} lib64
	done

	#the ld-linux library is handled separately
	LDLIB=$(ldd ${EXE} | grep ld-linux | awk '{print $1}')
	if [ -n ${LDLIB} ] 
	then
		celix_add_file ${LDLIB} lib64
	fi
}

celix_add_libs_for_exe() {
	EXE=$1
	for LIB in $(ldd ${EXE} | grep lib | awk '{print $3}')
	do
		celix_add_file ${LIB} lib64
	done

	#the ld-linux library is handled separately
	LDLIB=$(ldd ${EXE} | grep ld-linux | awk '{print $1}')
	if [ -n ${LDLIB} ] 
	then
		celix_add_file ${LDLIB} lib64
	fi
}

celix_add_exe() {
	EXE=$1
	celix_add_libs_for_exe ${EXE}

    #if [[ "${EXE}" =~ ^/.* ]]; then
    #    DIR=.$(dirname ${EXE})
    #else
    #   #for relative paths use usr/bin
    #    DIR=usr/bin
    #fi

    #Always put executables in /bin
	DIR=bin

	mkdir -p ${DIR} 2> /dev/null
	cp -vu ${EXE} ${DIR}/
}

celix_add_required_libs() {
	if [ ! -d /lib64 ]
	then
		echo "ERROR: Assuming 64 bit architecture for docker creating. Created filesystem will not be valid"
	fi
	#LIBS=$(ls -1 /lib64/ld-linux* /lib64/libnss_dns* /lib64/libnss_files*)
	LIBS=$(ls -1 /lib64/ld-linux* /lib64/libnss_dns* /lib64/libnss_files* /lib/x86_64-linux-gnu/libnss_dns* /lib/x86_64-linux-gnu/libnss_files* 2> /dev/null)
	for LIB in ${LIBS}
	do
		celix_add_file ${LIB} 
	done
}

for INDEX in "${!EXE_ARGUMENTS[@]}"
do
	EXE=${EXE_ARGUMENTS[${INDEX}]}
	echo "Adding exe ${EXE}"
	celix_add_exe ${EXE}
done

for INDEX in "${!LIB_ARGUMENTS[@]}"
do
	LIB=${LIB_ARGUMENTS[${INDEX}]}
	echo "Adding lib ${LIB}"
	celix_add_lib ${LIB}
done

for INDEX in "${!BUNDLE_ARGUMENTS[@]}"
do
	BUNDLE=${BUNDLE_ARGUMENTS[${INDEX}]}
	echo "Adding bundle ${BUNDLE}"
	celix_add_bundle ${BUNDLE}
done

		
mkdir root 2> /dev/null

echo "Creating minimal etc dir"

mkdir etc 2> /dev/null
if [ ! -f etc/group ]
then
    echo """root:x:0:
bin:x:1:
daemon:x:2:
adm:x:4:
lp:x:7:
mail:x:12:
games:x:20:
ftp:x:50:
nobody:x:99:
users:x:100:
nfsnobody:x:65534:""" > etc/group
fi

if [ ! -f etc/nsswitch.conf ]
then
    echo """passwd:     files
shadow:     files
group:      files
hosts:      files dns""" > etc/nsswitch.conf
fi

if [ ! -f etc/passwd ]
then
    echo """root:x:0:0:root:/root:/bin/bash
bin:x:1:1:bin:/bin:/sbin/nologin
daemon:x:2:2:daemon:/sbin:/sbin/nologin
adm:x:3:4:adm:/var/adm:/sbin/nologin
lp:x:4:7:lp:/var/spool/lpd:/sbin/nologin
sync:x:5:0:sync:/sbin:/bin/sync
shutdown:x:6:0:shutdown:/sbin:/sbin/shutdown
halt:x:7:0:halt:/sbin:/sbin/halt
mail:x:8:12:mail:/var/spool/mail:/sbin/nologin
operator:x:11:0:operator:/root:/sbin/nologin
games:x:12:100:games:/usr/games:/sbin/nologin
ftp:x:14:50:FTP User:/var/ftp:/sbin/nologin
nobody:x:99:99:Nobody:/:/sbin/nologin""" > etc/passwd
fi

celix_add_required_libs
