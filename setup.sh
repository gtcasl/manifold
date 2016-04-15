#!/bin/bash
#
# Run this script to set up the qsim environment for the first time.
# You can read the following steps to see what each is doing.
#
# Author: He Xiao
# Date: 02/05/2016
# Usage: ./setup.sh

bold=$(tput bold)
normal=$(tput sgr0)

MANIFOLD_DIR=$PWD

# test qsim installation
if [  -z "$QSIM_PREFIX" ]; then
    echo "\n${bold}qsim${normal} is not installed "
    echo "Press any key to exit..."
    read inp
    exit 1
fi

# install manifold-dep packages
echo "Installing dependencies ..."
echo "sudo apt-get -y install build-essentail libconfig++-dev openmpi-bin openmpi-common libopenmpi-dev"
sudo apt-get -y install build-essential libconfig++-dev openmpi-bin openmpi-common libopenmpi-dev

# Build manifold simulator
echo "Building manifold components ..."
./configure QSIMINC=${QSIM_PREFIX}/include
make -j4 

echo "Downloading the benchmark ..."
cd ${MANIFOLD_DIR}/simulator/smp
mkdir -p benchmark
cd benchmark
wget -c "https://www.dropbox.com/s/8551vnwhzqt9fk6/graphbig_x64.tar.gz?dl=0" -O graphbig_x64.tgz 
wget -c "https://www.dropbox.com/s/rpxgwhyr41m2oa0/graphbig_a64.tar.gz?dl=0" -O graphbig_a64.tgz 
wget -c "https://www.dropbox.com/s/jslv1g1v77ndq8m/splash-2.tar.gz?dl=0" -O splash_x86.tgz 
echo "Uncompressing the benchmark ..."
tar -xzvf graphbig_a64.tgz
tar -xzvf graphbig_x64.tgz
tar -xzvf splash_x86.tgz
cd ..

if [ ! -f $QSIM_PREFIX/state.64 ]; then
    echo "QSim state files are not found in $QSIM_PREFIX"
    echo "Please run ${bold}mkstate.sh${normal} in $QSIM_PREFIX"
    exit 1
else
    echo "Linking QSim state files to manifold directory smp/state ..."
    ln -s $QSIM_PREFIX state
fi

echo "Building simulator ..."
cd QsimProxy
make -j4


if [ $? -eq "0" ]; then
  echo "\n${bold}Manifold built successfully!${normal}\n\n"
  echo "Simulation Example:"
  echo "QsimProxy/smp_llp ../config/conf2x3_spx_torus_llp.cfg ../state/state.4 ../benchmark/graphbig_x86/bc.tar"
fi
