#!/bin/bash

# Install the required dependencies:

sudo apt-get -y install git libssl-dev dh-autoreconf cmake libgl1-mesa-dev libpciaccess-dev libdrm-dev libx11-dev ffmpeg curl

# Create a directory and export the path:

mkdir -p $HOME/build-media-sdk
export WORKDIR=$HOME/build-media-sdk

# libVA
# Build and install the libVA library:

cd $WORKDIR
git clone https://github.com/intel/libva.git libva
cd libva
./autogen.sh --prefix=/usr --libdir=/usr/lib/x86_64-linux-gnu
make -j4
sudo make install

# libVA Utils
# Build and install libVA utils:

cd $WORKDIR
git clone https://github.com/intel/libva-utils.git
cd libva-utils
./autogen.sh --prefix=/usr --libdir=/usr/lib/x86_64-linux-gnu
make -j4
sudo make install

# Intel(R) Media Driver for VAAPI
# Build the Media Driver for VAAPI:

cd $WORKDIR
git clone https://github.com/intel/gmmlib.git
git clone https://github.com/intel/media-driver.git
mkdir -p $WORKDIR/build
cd $WORKDIR/build
cmake ../media-driver 
make -j8
sudo make install

