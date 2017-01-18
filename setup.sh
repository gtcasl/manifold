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
else
    echo "\nUsing ${bold}qsim${normal} from dir ${bold}$QSIM_PREFIX${normal}"
    echo "Press any key to continue..."
    read inp
fi

# install manifold-dep packages
if hash apt-get 2>/dev/null; then
    echo "Installing dependencies ..."
    echo "sudo apt-get -y install build-essentail libconfig++-dev openmpi-bin openmpi-common libopenmpi-dev"
    sudo apt-get -y install build-essential libconfig++-dev openmpi-bin openmpi-common libopenmpi-dev
else
    echo "please refer to manifold manual for installation dependencies"
    echo "Press any key to continue..."
    read dump
fi

# Build manifold simulator
echo "Building manifold components ..."
autoreconf -siv
if [  -z "$KITFOX_PREFIX" ]; then
    echo "\n${bold}kitfox library${normal} disabled "
    echo "Press any key to continue..."
    read inp
    ./configure QSIMINC=${QSIM_PREFIX}/include --without-kitfox
else
    echo "\nUsing ${bold}kitfox${normal} from dir ${bold}$KITFOX_PREFIX${normal}"
    echo "Press any key to continue..."
    read inp
    ./configure QSIMINC=${QSIM_PREFIX}/include KITFOXINC=${KITFOX_PREFIX}
fi
make -j4 

echo "Downloading the benchmark ..."
cd ${MANIFOLD_DIR}/simulator/smp
mkdir -p benchmark
cd benchmark
wget -c "https://github.com/gtcasl/qsim_prebuilt/releases/download/v0.1/graphBig_x86.tar.xz" -O graphbig_x86.tar.xz 
wget -c "https://github.com/gtcasl/qsim_prebuilt/releases/download/v0.1/graphBig_a64.tar.xz" -O graphbig_a64.tar.xz
echo "Uncompressing the benchmark ..."
tar -xf graphbig_x86.tar.xz
tar -xf graphbig_a64.tar.xz
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
make clean
if [  -z "$KITFOX_PREFIX" ]; then
    make -j4
else
    make -j4 -f Makefile.kitfox
fi


if [ $? -eq "0" ]; then
  echo "\n${bold}Manifold built successfully!${normal}\n\n"
  echo "Simulation Example:"
  echo "cd simulator/smp/QsimProxy"
  echo "smp_llp ../config/conf2x3_spx_torus_llp.cfg ../state/state.4 ../benchmark/graphbig_x86/bc.tar"
fi
