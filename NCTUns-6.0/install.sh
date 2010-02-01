#!/bin/bash
#
# NCTUns 6.0 Network Simulator installation script
# Last updated: 09/01/2009
#
# Network and System Laboratory 
# Department of Computer Science 
# College of Computer Science
# National Chiao Tung University, Taiwan
# All Rights Reserved.
#

# change current path to package directory
pkgdir=`echo $0 | sed 's#\(.*\)/[^/]\+$#\1#'`
if [ -d "$pkgdir" ]; then
    pushd $pkgdir >/dev/null
fi
pkgdir=`pwd`

# include library
for i in $pkgdir/install.d/E*; do
    if [ -r $i ]; then
        . $i
    fi
done

args=`getopt c:ehi:l:mqsv $*`
if [ $? = 1 ]; then
    show_help
fi
set -- $args
for i; do
    case "$i"
    in
        -c) # read configure file
            dot_config=$2
            if [ -r "$dot_config" ]; then
                . $dot_config
            fi
            shift; shift;
            ;;
        -e) # set strip mode of all execute
            strip_symbol=1
            shift;
            ;;
        -h) # display help information
            show_help
            ;;
        -i) # set install prefix path
            installdir=$2
            shift; shift;
            ;;
        -l) # set install progress logfile
            logfile=$2
            shift; shift;
            ;;
        -m) # set mono mode
            color=0
            shift;
            ;;
        -q) # set quiet mode
            quiet=1
            out_null='>/dev/null'
            shift;
            ;;
        -s) # set step by step parameter
            stepbystep=1
            shift;
            ;;
        -v) # set verbose mode
            verbose=1
            shift;
            ;;
        --) # stop
            shift;
            ;;
    esac
done

# set environment variable
for i in $pkgdir/install.d/S*; do
    if [ -r $i ]; then
        . $i
    fi
done

# convert logfile path
if [ `echo $logfile | cut -c 1` != '/' ]; then
    logfile="$pkgdir/$logfile"
fi

# if verbose mode is turn on, then disable quiet mode
if [ $verbose = 1 ]; then
    quiet=0
    out_null=''
    info "log install message to $logfile"
fi

# execute install procedure
for i in $pkgdir/install.d/I*; do
    if [ -r $i ]; then
        . $i
    fi
done

stop_log
