#!/bin/bash

function usage()
{
    echo "
	Sony LK build script
	Usage: sh build.sh <platform> <releasetype> <outfile>

	platform: Device codename that you are building for [available: mint, yuga, pollux_windy, odin]
			mint: Sony Xperia T (LT30p)
			yuga: Sony Xperia Z (C6603)
			pollux_windy: Sony Xperia Tablet Z (SGP311)
			odin: Sony Xperia ZL (C6503)

	release type: What type of build indicates how it is packaged [available: debug]
			debug: Builds and packs the binary to be flashed with fastboot
			release: Builds binary and packs into an update.zip (not implemented yet) 

	outfile: Name of output and path from ./ (optional)
	"
    exit 1;
}

function test_gcc()
{
    if hash arm-eabi-gcc 2>/dev/null; then
        return;
    else
        echo "GCC not found, exiting (in future this will solve the problem for us)"
        exit 1;
    fi
}

function build_platform()
{

    if [ "mint" == "$1" ]; then
        uses_rpm=1
        build_elf=1
        rpm="RPM/mint.bin"
        rpm_loadaddr="0x20000"
        loadaddr="0x80208000"
        platform_type="$1"
        return
    fi

    if [ "yuga" == "$1" ]; then
        uses_rpm=0
        loadaddr="0x80208000"
        platform_type="$1"
        return
    fi

    if [ "odin" == "$1" ]; then
        uses_rpm=0
        loadaddr="0x80208000"
        platform_type="$1"
        return
    fi

    if [ "pollux_windy" == "$1" ]; then
        uses_rpm=0
        loadaddr="0x80208000"
        platform_type="$1"
        return
    fi

    echo "Platform $1 not supported"
    exit 1;

}

function output_build()
{
    if [ ! -z "$2" ]; then
        outfile=$2
    fi

    if [ -z $outfile ]; then
        if [ "debug" == "$1" ]; then
            outfile="lk.elf"
        else
            outfile="lk.zip"
        fi
    fi

    if [ "$build_elf" == "1" ]; then
        if [ $uses_rpm -eq 1 ]; then
            python scripts/mkelf.py -o $outfile build-$platform_type/lk.bin@$loadaddr $rpm@$rpm_loadaddr,rpm
        else
            python scripts/mkelf.py -o $outfile build-$platform_type/lk.bin@$loadaddr
        fi
    else
        abootimg --create $outfile -f bootimg/bootimg-$platform_type.cfg -k build-$platform_type/lk.bin -r bootimg/fakerd.img
        echo -n "!RECOVERY" >> $outfile
    fi

    if [ "release" == "$1" ]; then
        echo "Building release is not yet available"
        exit 1;
    fi
}

if [ -z "$1" ]; then
    usage
fi

if [ -z "$2" ]; then
    usage
fi

test_gcc

build_platform $1

make $platform_type -j16

if [ ! -f "build-$platform_type/lk.bin" ]; then
   echo "Build Failed"
   exit
fi

output_build $2 $3


# Delete the file used to build elf so that script can tell if
# the build failed on the next invocation.
rm build-$platform_type/lk.bin

