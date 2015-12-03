#!/bin/sh
#
# Build the dependencies for the Travis-CI build of the BES and all of
# its modules that are distributed with Hyrax.
#
# Two things about this script: 1. It builds both the dependencies for
# a complete set of Hyrax modules and that is quite taxing for the Travis
# system since Travis allows logs of 4MB or less. When the log hits the
# 4MB size, Travis stops the build. To get around that limitation, I send
# stdout output of the hyrax deps build to /dev/null. The output to stderr
# still shows up in the log, however, and that turns out to be important
# since output has to appear once every X minutes (5, 10?) or the build 
# will be stopped.
# 2. We have tired building using Ubuntu packages, but the Ubuntu 12 pkgs
# are just not current enough for Hyrax. We could drop a handful of the
# deps built here and get them from packages, but it's not enough to make
# a big difference. Also, building this way mimics what we will do when it's
# time to make the release RPMs.

set -e # enable exit on error

# hyrax-dependencies appends '/deps' to 'prefix'
export prefix=$HOME
export PATH=$HOME/deps/bin:$PATH

echo prefix: $prefix


#------------------------------------------------------------------------------
# Build hyrax-dependencies project
#
#
newClone=false

if test ! -d "$HOME/hyrax-dependencies" -o ! -f "$HOME/hyrax-dependencies/Makefile"
then
    echo "Cloning hyrax-dependencies."
    (cd $HOME && git clone https://github.com/opendap/hyrax-dependencies)
    echo "Cloned hyrax-dependencies"
    newClone=true  
fi     
echo newClone: $newClone

echo "Using 'git pull' to update hyrax-dependencies..."
set +e # disable  exit on error
pull_status="up-to-date"
(cd $HOME/hyrax-dependencies; git pull) | grep "$pull_status" 
if [ $? -ne 0 ]; then
    pull_status="needs_update"
fi
set -e # (re)enable exit on error

echo up_to_date: $up_to_date 
echo pull_status: $pull_status 

if test "$newClone" = "true" -o ! $pull_status = "up-to-date"  -o ! -x "$HOME/deps/bin/bison" 
then
    echo "(Re)Building hyrax-dependencies"
    (cd $HOME/hyrax-dependencies && make for-travis -j7) > /dev/null
    echo "Completed hyrax-dependencies build - stdout to /dev/null to save space"
else
    echo "Using cached hyrax-dependencies."
fi


#------------------------------------------------------------------------------
# Build libdap project
#
#
newClone=false

if test ! -f "$HOME/libdap4/Makefile.am"
then
    echo "Cloning libdap4..."
    (cd $HOME && git clone https://github.com/opendap/libdap4)
    echo "Cloned libdap4"
    newClone=true  
fi     
echo newClone: $newClone

echo "Using 'git pull' to update libdap4..."
pull_status="up-to-date"
set +e # disable  exit on error
(cd $HOME/libdap4; git pull) | grep "$pull_status" 
if [ $? -ne 0 ]; then
    pull_status="needs_update"
fi
set -e # (re)enable exit on error

echo pull_status: $pull_status 

if test "$newClone" = "true" -o ! $pull_status = "up-to-date"  -o ! -x "$HOME/deps/bin/dap-config" 
then
    echo "(Re)Building libdap4"
    (cd $HOME/libdap4 && autoreconf -vif && ./configure --prefix=$prefix/deps/ && make -j7 && make install)
    echo "Completed libdap4 build"
else
    echo "Using cached libdap4."
fi




exit




# unlike hyrax-dependencies, the libdap tar needs --prefix to be the
# complete dir name. The hyrax-deps... project is a bit of a hack...

#if test ! -x "$HOME/deps/bin/dap-config" -o ! "`dap-config --version`" = "libdap 3.16.0"
#then
#    wget http://www.opendap.org/pub/tmp/travis/libdap-3.16.0.tar.gz
#    tar -xzf libdap-3.16.0.tar.gz
#    (cd libdap-3.16.0 && ./configure --prefix=$prefix/deps/ && make -j7 && make install)
#else
#    echo "Using cached libdap."
#fi
