
: '
  Copyright (c) 2018 Intel Corporation.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
'

sudo apt-get update
BASEDIR=$(pwd)

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

cd $BASEDIR

# InfluxDB
sudo curl -sL https://repos.influxdata.com/influxdb.key | sudo apt-key add -
source /etc/lsb-release
echo "deb https://repos.influxdata.com/${DISTRIB_ID,,} ${DISTRIB_CODENAME} stable" | sudo tee /etc/apt/sources.list.d/influxdb.list
sudo apt-get update && sudo apt-get install influxdb
sudo service influxdb start

# Grafana
wget https://s3-us-west-2.amazonaws.com/grafana-releases/release/grafana_5.3.2_amd64.deb
sudo apt-get install -y adduser libfontconfig
sudo dpkg -i grafana_5.3.2_amd64.deb
sudo /bin/systemctl start grafana-server

if [ -d "json" ]
then
   sudo rm -r json
fi

git clone https://github.com/nlohmann/json

sudo apt-get install libcurl4-openssl-dev        # Installing libcurl library

cd resources
wget -O face-demographics-walking-and-pause.mp4 https://raw.githubusercontent.com/intel-iot-devkit/sample-videos/master/face-demographics-walking-and-pause.mp4

cd /opt/intel/openvino/deployment_tools/tools/model_downloader

sudo ./downloader.py --name face-detection-retail-0004		# Downloading required models for the application
sudo ./downloader.py --name age-gender-recognition-retail-0013
sudo ./downloader.py --name head-pose-estimation-adas-0001
