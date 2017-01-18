manifold[![Build Status](https://travis-ci.org/gtcasl/manifold.svg?branch=master)](https://travis-ci.org/gtcasl/manifold)
=============
A Parallel Simulation Framework For Multicore Systems


**Installation Instructions**

If you are on Ubuntu (>= 14.04), just run setup.sh script in the root directory 
after QSim is installed. This will link the manifold to the QSim library specifi
ed by $QSIM_PREFIX variable, download multi-thread benchmarks supported by manif
old, and build the manifold components and simulators.

Manifold now integrates [KitFox multi-physics library](http://manifold.gatech.edu/projects/kitfox) to simulate physical phenomena including power, thermal and lifetime reliability models. After KitFox framework is installed and exported to system variable $KITFOX_PREFIX, the setup.sh script will configure manifold to enable KitfoxProxy that interacts with the given KitFox framekwork. The lastest KitFox source code tarball can be found [here](http://manifold.gatech.edu/wp-content/uploads/2017/01/kitfox-v1.1.tar.gz). Please refer to [KitFox user manual](http://manifold.gatech.edu/wp-content/uploads/2015/04/kitfox-v1.0.0.pdf) for details.

Users under other Linux distributions, please consult instructions in [manifold document](http://manifold.gatech.edu/documentation).
