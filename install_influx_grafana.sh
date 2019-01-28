#!/bin/bash

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

