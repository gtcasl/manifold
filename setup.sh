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
    echo -e "\n${bold}qsim${normal} is not installed "
    echo -e "Press any key to exit..."
    read inp
    exit 1
else
    echo -e "\nUsing ${bold}qsim${normal} from dir ${bold}$QSIM_PREFIX${normal}"
    echo -e "Press any key to continue..."
    read inp
fi

# install manifold-dep packages
if hash apt-get 2>/dev/null; then
    echo -e "Installing dependencies ..."
    echo -e "sudo apt-get -y install build-essentail libconfig++-dev openmpi-bin openmpi-common libopenmpi-dev"
    sudo apt-get -y install build-essential libconfig++-dev openmpi-bin openmpi-common libopenmpi-dev
else
    echo -e "please refer to manifold manual for installation dependencies"
    echo -e "Press any key to continue..."
    read dump
fi

# Build manifold simulator
echo -e "Building manifold components ..."
autoreconf -siv
if [  -z "$KITFOX_PREFIX" ]; then
    echo -e "\n${bold}kitfox library${normal} disabled "
    echo -e "Press any key to continue..."
    read inp
    ./configure QSIMINC=${QSIM_PREFIX}/include --without-kitfox
else
    echo -e "\nUsing ${bold}kitfox${normal} from dir ${bold}$KITFOX_PREFIX${normal}"
    echo -e "Press any key to continue..."
    read inp
    ./configure QSIMINC=${QSIM_PREFIX}/include KITFOXINC=${KITFOX_PREFIX}
fi
make -j4 

echo -e "Downloading the benchmark ..."
cd ${MANIFOLD_DIR}/simulator/smp
mkdir -p benchmark
cd benchmark
wget -c "https://github.com/gtcasl/qsim_prebuilt/releases/download/v0.1/graphBig_x86.tar.xz" -O graphbig_x86.tar.xz 
wget -c "https://github.com/gtcasl/qsim_prebuilt/releases/download/v0.1/graphBig_a64.tar.xz" -O graphbig_a64.tar.xz
echo -e "Uncompressing the benchmark ..."
tar -xf graphbig_x86.tar.xz
tar -xf graphbig_a64.tar.xz
cd ..

if [ ! -f $QSIM_PREFIX/state.64 ]; then
    echo -e "QSim state files are not found in $QSIM_PREFIX"
    echo -e "Please run ${bold}mkstate.sh${normal} in $QSIM_PREFIX"
    exit 1
else
    echo -e "Linking QSim state files to manifold directory smp/state ..."
    ln -s $QSIM_PREFIX state
fi

echo -e "Building simulator ..."
cd QsimProxy
make clean
if [  -z "$KITFOX_PREFIX" ]; then
    make -j4
else
    make -j4 -f Makefile.kitfox
fi


if [ $? -eq "0" ]; then
  echo -e "\n${bold}Manifold built successfully!${normal}\n\n"
  echo -e "Simulation Example:"
  echo -e "QsimProxy/smp_llp ../config/conf2x3_spx_torus_llp.cfg ../state/state.4 ../benchmark/graphbig_x86/bc.tar"
fi
