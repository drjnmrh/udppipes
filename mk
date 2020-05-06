#!/bin/bash


MYDIR="$(cd "$(dirname "$0")" && pwd)"

OLDDIR=${PWD}
PLATFORM=auto
NEEDCLEANUP=0
NEEDTEST=0


mk::read_local_properties() {
    local _root=$1

    if [[ ! -e $_root/local.properties ]]; then
        echo "$_root doesn't contain local properties - skipping\n"
        return 1
    fi

    local _content=$(cat $_root/local.properties)

    local _title="local.properties"

    while IFS= read -r line
    do
        local _beforecomment=$(echo $line | sed 's/\(.*\)\([#].*\)$/\1/g')
        local _propname=$(echo $_beforecomment | sed 's/^\(\([a-zA-Z0-9.][a-zA-Z0-9.]*[=]\)\(.*\)\)$/\2/g')
        if [[ ! "$_propname" == "" ]]; then
            _propname=${_propname:0:${#_propname}-1} # remove '=' character
            local _propvalue=$(echo $_beforecomment | sed 's/^\(.*[=]["]\(.*\)["]\)$/\2/g')

            local _varname=$(echo $_propname | sed 's/[.]/_/g' | tr a-z A-Z)

            export $_varname="$_propvalue"
        fi
    done < $_root/local.properties

    return 0
}


mk::read_local_properties ${MYDIR}



while [[ "$#" > 0 ]]; do case $1 in
--platform) PLATFORM=$2; shift;;
--clean) NEEDCLEANUP=1;;
--test) NEEDTEST=1;;
*) echo "Unknown parameter passed: $1" >&2; exit 1;;
esac; shift; done


if [[ "$PLATFORM" == "auto" ]]; then
    _uname=$(uname)
    if [[ "$_uname" == "Darwin" ]]; then
        PLATFORM=macos
    elif [[ "$_uname" == "Linux" ]]; then
        PLATFORM=linux
    fi
fi


if [[ $NEEDCLEANUP -eq 1 ]]; then
    if [[ -d ".build.${PLATFORM}" ]]; then
        rm -rf .build.${PLATFORM}
    fi
fi

if [[ ! -d ".build.${PLATFORM}" ]]; then
    mkdir .build.${PLATFORM}
fi

cd .build.${PLATFORM}

if [[ "$PLATFORM" == "macos" ]]; then
    cmake .. -G "Xcode"
elif [[ "$PLATFORM" == "iphone" ]]; then
    cmake .. -DCMAKE_TOOLCHAIN_FILE=${MYDIR}/3rdparty/cmake/iOS.cmake \
             -G "Xcode" \
			 -DIOS_DEPLOYMENT_TARGET=9.3 -DCMAKE_OSX_DEPLOYMENT_TARGET=9.3 \
			 -DIOS_PLATFORM=OS \
             -DIPHONE_BUNDLEID="com.musescore.player.tester" -DIPHONE_DEVTEAM="$IPHONE_TEAM" \
             -DIPHONE_SIGNID="$IPHONE_SIGNID" -DIPHONE_SIGNIDSHA="$IPHONE_SIGNIDSHA"
fi

if [[ $NEEDTEST -eq 1 ]]; then
    cmake --build . --config Release -- -jobs 8
    if [[ $? -ne 0 ]]; then
        echo "FAILED!!!"
        exit 1
    fi

    ctest -C Release --verbose
    if [[ $? -ne 0 ]]; then
        echo "FAILED!!!"
        exit 1
    fi

    cmake --build . --config Debug -- -jobs 8
    if [[ $? -ne 0 ]]; then
        echo "FAILED!!!"
        exit 1
    fi

    ctest -C Debug --verbose
    if [[ $? -ne 0 ]]; then
        echo "FAILED!!!"
        exit 1
    fi
fi

cd $OLDDIR
