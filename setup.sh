#!/bin/sh
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
echo "Installing dependencies..."
echo "sudo apt-get -y install build-essentail libconfig++-dev openmpi-bin openmpi-common libopenmpi-dev"
sudo apt-get -y install build-essential libconfig++-dev openmpi-bin openmpi-common libopenmpi-dev

# Build manifold simulator
echo "Building manifold components ..."
./configure QSIMINC=${QSIM_PREFIX}/include
make -j4 

echo "Building simulator ..."
cd ${MANIFOLD_DIR}/simulator/smp/QsimProxy
make -j4


if [ $? -eq "0" ]; then
  echo "\n${bold}Manifold built successfully!${normal}"
fi
