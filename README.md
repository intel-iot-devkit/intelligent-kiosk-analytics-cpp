# 4K Advertisement Display on Digital Kiosk Based on Demographics

| Details          |                    |
|-----------------------|---------------|
| Target OS:            |  Ubuntu* 16.04 LTS|
| Programming Language: |  C++ |
| Time to Complete:    |  45 min     |


# ---This reference implementation is under construction, please check back in a couple of weeks for an update.---

## Introduction

This reference implementation is developed using the Intel® Distributon of OpenVINO™ toolkit for audience analytics and Intel® Media SDK for the display of 4K advertisements. The application identifies the age and gender of the audience standing in front of digital signage, and based on the identification, it selects a suitable 4K advertisement. Real-time data visualization,  which enables developers to monitor trends over time, occurs on Grafana* This reference implementation aims to improve real-world marketing and advertising.

## Requirements

### Hardware

* [IEI Tank* AIoT Developer Kit](https://software.intel.com/en-us/iot/hardware/iei-tank-dev-kit)

### Software
* Ubuntu* 16.04 LTS
* Intel® Distributon of OpenVINO™ toolkit 2018 (R3)
* Intel® Media SDK
* Grafana* v5.3.2
* InfluxDB* v1.6.2

## Setup

### Install Intel® Distributon of OpenVINO™ toolkit
Refer to [Install the Intel® Distributon of OpenVINO™ toolkit for Linux*](https://software.intel.com/en-us/articles/OpenVINO-Install-Linux) for more information about how to install and setup the Intel® Distributon of OpenVINO™ toolkit.

### Install Dependencies for Intel® Media SDK

Run *install_mediasdk_dependencies.sh* script to install mediaSDK dependencies:
```
./install_mediasdk_dependencies.sh
```
**Note**: Configure the proxy setting for *git*, if behind a proxy server:
```
git config --global http.proxy http://<proxyUsername>:<proxyPassword>@<proxy.server.com>:<port>
```

### Install InfluxDB* and Grafana

Run *install__influx_grafana.sh* script to install the InfluxDB* and Grafana:
```
./install_influx_grafana.sh
```

## How It Works

The application uses a video source, such as a camera to grab frames, and Deep Neural Networks (DNNs) to process data. The first network detects faces. The second neural network determines age and gender for each detected face. The third neural network detects the head pose of the person.

The application observes the age, gender, and head pose of the person standing in front of the digital signage camera and collects data for five frames. Data are normalized to avoid any fluctuations in the observation. Based on normalized data, the application selects a gender-appropriate advertisement.

A JSON file provides a list of advertisements for different age and gender groups. This JSON file is parsed, and data present in the file such as age group, gender, and a list of advertisements, are stored in a C++ structure. Once the software determines the dominant age-group and gender from the audience analytics, it selects an ad from JSON data, which is decoded using the HEVC plugin and played using Intel® Media SDK.

This application has two processes, the audience analytics process (AAP) and the advertisement display process (ADP). These processes use pipes for inter-process communication.

The AAP analyzes the audience, selects the appropriate ad, and writes the selected ad to the pipe, which the ADP reads from the other end of the pipe. The ADP reads the file name from the pipe, decodes the video which is in H.265 format, and renders it. Once the advertisement finishes playing, the ADP sends the acknowledgement of ad completion through the pipe to AAP. On receiving the acknowledgement, AAP selects the next ad based on the audience present at that time and sends it to ADP. The new advertisement plays only when the previous advertisement has completed.

The audience analysis happens even when an ad is playing as the two processes run in parallel. For every 20 frames, the application sends data to the InfluxDB database which is then visualized on Grafana to display the trends over time. Grafana visualizes the number of people who were interested in ads, the number of people not interested in ads, the ad currently playing, and the total number of unique visitors who were in front of the digital signage.  

![Code organization](./images/arch.png)

The DNN models are optimized for Intel® architecture and are included with Intel® Distributon of OpenVINO™ toolkit. The models can be be found in these locations:

- /opt/intel/computer_vision_sdk/deployment_tools/intel_models/face-detection-retail-0004
- /opt/intel/computer_vision_sdk/deployment_tools/intel_models/age-gender-recognition-retail-0013
- /opt/intel/computer_vision_sdk/deployment_tools/intel_models/head-pose-estimation-adas-0001

## Set the Build Environment
 
Configure the environment to use the Intel® Distributon of OpenVINO™ toolkit and Intel® Media SDK one time per session by exporting environment variables:
```
source /opt/intel/computer_vision_sdk/bin/setupvars.sh
export LIBVA_DRIVERS_PATH=/opt/intel/mediasdk/lib64
export LIBVA_DRIVER_NAME=iHD
export MFX_HOME=/opt/intel/mediasdk
```

### Build the Code

Go to the project directory:

```
cd <path-to-kiosk-analytics>
```

Create a **build** directory:

```
mkdir build
```

Build the project:

```
cd build
cmake ..
make -j8
export LD_LIBRARY_PATH=$PWD/intel64/Release/lib:$LD_LIBRARY_PATH
```

### Run the Code

Run the application with a video file video file as input:

```
__bin/Release/kiosk -i ../data/demoPeople.mp4 -m /opt/intel/computer_vision_sdk/deployment_tools/intel_models/face-detection-retail-0004/FP32/face-detection-retail-0004.xml -m_ag /opt/intel/computer_vision_sdk/deployment_tools/intel_models/age-gender-recognition-retail-0013/FP32/age-gender-recognition-retail-0013.xml -m_hp /opt/intel/computer_vision_sdk/deployment_tools/intel_models/head-pose-estimation-adas-0001/FP32/head-pose-estimation-adas-0001.xml
```
Run the application with a camera for audience analytics:
```
__bin/Release/kiosk -i cam -m /opt/intel/computer_vision_sdk/deployment_tools/intel_models/face-detection-retail-0004/FP32/face-detection-retail-0004.xml -m_ag /opt/intel/computer_vision_sdk/deployment_tools/intel_models/age-gender-recognition-retail-0013/FP32/age-gender-recognition-retail-0013.xml -m_hp /opt/intel/computer_vision_sdk/deployment_tools/intel_models/head-pose-estimation-adas-0001/FP32/head-pose-estimation-adas-0001.xml
```

In addition to the model provided by the Intel® Distributon of OpenVINO™ toolkit, the user can include models trained on Caffe*, TensorFlow*, or Apache MXNet* machine learning frameworks for face-detection, age-gender recognition, or head pose. To do this, first configure the model optimizer for the framework used and then convert the trained model to produce an optimized Intermediate Representation (IR) of the model. 

Learn more about the model optimizer in the [Model Optimizer Developer Guide](https://software.intel.com/en-us/articles/OpenVINO-ModelOptimizer)

Once the IR is obtained, run the application:
```
__bin/Release/kiosk -i ../data/demoPeople.mp4 -m <path-to-face-detection-IR>/face-detection.xml -m_ag <path-to-age-gender-IR>/age-gender-recognition.xml -m_hp <path-to-head-pose-estimation-IR>/head-pose-estimation.xml
```


### Troubleshooting

The application may display an error concerning regarding shared library loading: *__bin/Release/kiosk: error while loading shared libraries: libcpu_extension.so: cannot open shared object file: No such file or directory*.

If this error occurs, go to *kiosk-analytics* directory and export the environment LD_LIBRARY_PATH variable:
```
cd build
export LD_LIBRARY_PATH=$PWD/intel64/Release/lib:$LD_LIBRARY_PATH
```

### Add New Advertisements to the List of Ads

To add the new ads in JSON file, follow this series of instructions:

1. Video is first converted to H.265 format using FFmpeg* as the decoding process takes the input videos in this format for decoding. To convert an MP4 video to required format, follow these steps:
    * Install FFmpeg:
    ```
    git clone https://github.com/FFmpeg/FFmpeg.git ffmpeg-master
    cd ffmpeg-master
    sudo apt install yasm
    ./configure
    make
    sudo make install
    ```
    **Troubleshooting:** 
    The application may display an error after the **sudo make install** instruction:  "*mkdir: cannot create directory ‘/usr/local/share/man/man1’: File exists*".
    
    If this problem occurs, remove symbolic links to these directories:
    ```
    sudo unlink /usr/local/share/man/man1
    sudo unlink /usr/local/share/man/man3
    ```
    * Convert video from .mp4 to .h265 format using FFmpeg:
    ```
    ffmpeg -i <input_video>.mp4  -c:v copy -bsf hevc_mp4toannexb <output_video>.h265
    ```
	
2. Copy the video to the **data** directory found in the *kiosk-analytics* directory.
3. Add a video file name for an ad. For example, to add a new ad file called  *intel.h265*, which is meant to be played for males between the age group 28 to 50 years, modify the adlist.json file:
    * Open **adList.json** in an editor.
    * Add the file name *intel.h265* at the end of the list of the ads for "Age_Group": 3, "Gender": "M".
    
    ```
        {
            "Age_Group": 3,
            "Gender": "M",
            "Ads": [
                "maleAd.h265",
                "maleAd.h265",
                "maleAd.h265",
                "intel.h265"
            ]
        }
    ```

    **Note**:
    * There are four age groups considered in the application:
        * Child: 1 to 13 yrs belongs to age group 1
        * Young Adult: 14 to 28 yrs belongs to age group 2
        * Adult: 28 to 50 yrs belongs to age group 3
        * Senior: 50+ yrs belongs to age group 4



### InfluxDB Database / Grafana

* Check time series demographics data on InfluxDB databases:
 
    ```
    influx
    show databases
    use Demographics
    select * from Demographics
    ```
  
* To visualize data on Grafana:

  * On the terminal, start the Grafana server:
    
      ```
      sudo service grafana-server start
      ```

  * Open the browser and go to **localhost:3000**.

  * Sign in with user as **admin** and password as **admin**.

  * Select the **Configuration** icon and then choose **Data Sources**.

       ![Grafana](./images/Grafana1.png)

  * Select **+ Add data source** and add these settings:
     * *Name*: Demographics
     * *Type*: InfluxDB
     * *URL*: http://localhost:8086
     * *Database*: Demographics
     * Click on “Save and Test”

   * Click the **Back** button at the bottom of the page. Select **+ Add data source** and enter these settings:
     * *Name*: AdData
     * *Type*: InfluxDB
     * *URL*: http://localhost:8086
     * *Database*: AdData
     * Click on “Save and Test”

    * Click the **Back** button at the bottom of the page. AdData and Demographics are added to the data sources.

       ![Grafana](./images/Grafana2.png)

  * To create a new dashboard:

     1. Select **+** icon from the side menu bar which is under Grafana icon and select **Dashboard**.  

        ![Grafana](./images/Grafana3.png)

     2. Select **Singlestat**. Click on the **Panel Title** and select **Edit**. 
        * Click on **General** in Singlestat menu tab and change the Panel Title to *Number of people*.
        * Click on **Metrics** in Singlestat menu tab. From **Data Source** choose **Demographics**. 
        * Click on **Add Query**. Click on **select measurement** and select **Demographics**.  
        * In **field (value)** click on **value** and select **Total people**.
        * Click on **+** beside *mean* in **SELECT** field. From **Selectors**, select **last()**. It will override *mean()*.
        
            After the series of steps #2., query A will be: 
               
              FROM default Demographics SELECT field(Total people) last() GROUP BY time($__interval) fill(null) FORMAT AS Time Series

        ![Grafana](./images/Grafana4.png)

        * Click on **Options** in Singlestat menu tab. Select **Current** from **stat** field and change the **Font size** to *120*, in the first row.
        * Select **show** under **Gauge** option present at the bottom.
        * Save the dashboard with name **Kiosk**. Save button is provided in the top menu.

        ![Grafana](./images/Grafana5.png)
    
     3. Repeat step 2 to add *Singlestat panel* for *Number of male* to the dashboard, with these modifications:
        * For the panel title, change *Number of people* to *Number of male*.
        * Select **Total male** in the **field (value)** of **Metrics**  Singlestat menu option.

            Query A will be: 

              FROM default Demographics SELECT field(Total male) last() GROUP BY time($__interval) fill(null) FORMAT AS Time Series

        ![Grafana](./images/Grafana6.png)
        
        * Save the dashboard and click on the **Back to dashboard** icon on right corner of the top menu.

     4. Similarly, to add *Singlestat panel* for *Number of female* to the dashboard, repeat step 2 with these modifications:
        * For the panel title, change *Number of people* to *Number of female*.
        * Select **Total female** in the **field (value)** of **Metrics**  Singlestat menu option.

            Query A will be: 

              FROM default Demographics SELECT field(Total female) last() GROUP BY time($__interval) fill(null) FORMAT AS Time Series
        
        * Save the dashboard and click on the **Back to dashboard** icon.
      

     5. Add a graph for visualizing demographics time series data:
        * Click on the **add panel** icon on the top menu and click on **Graph**. This will add the Graph to the dashboard.
        * Click on the **Panel Title** of the Graph and select **Edit**. 
        * Click on **General** in Graph menu tab and change the Panel Title to *People viewing Ad*.
        * Click on **Metrics** in Graph menu tab. From **Data Source** choose **Demographics**. 
        * Click on **Add Query**. Click on **select measurement** and select **Demographics**. 
        * Click on **field (value)** and select **Total people**. 
        * Click on **+** in **SELECT** field. From **Selectors**, select **last()**.
        * In GROUP BY field, click on **time($__interval)** and select **10s**. 
        * Name the query as **Number of people** in the **ALIAS BY** row.

          Query A will be: 
               
              FROM default Demographics SELECT field(Total people) last() GROUP BY time(10s) fill(null) FORMAT AS Time Series ALIAS BY Number of People

        * Similarly add queries for displaying no. of male and female by clicking on **+**. For male and female, queries will be:

          Query B: 
               
              FROM default Demographics SELECT field(Total male) last() GROUP BY time(10s) fill(null) FORMAT AS Time Series ALIAS BY Number of male

          Query C: 
               
              FROM default Demographics SELECT field(Total female) last() GROUP BY time(10s) fill(null) FORMAT AS Time Series ALIAS BY Number of female

        ![Grafana](./images/Grafana7.png)

        * Click on **Display** in Graph menu tab and change the values of **Fill** and **Line Width** under **Mode Options** to **2**.
        * Click on **Time Range** present on the menu tab and write **500s** in the text box provided beside **Override relative time** **Last**.   
        * Save the dashboard and click on **Back to dashboard** icon.

     6. Add the table for displaying the Demographics data:
         * Click on the **add panel** icon on the top menu and click on **Table**.
         * Click on the **Panel Title** of the Table and select **Edit**. 
         * Click on **General** in Table menu tab and change the Panel Title to *Demographics*.
         * Click on **Metrics** in Table menu tab. From **Data Source** choose **Demographics**.  
         * Configure the following queries under **Metrics**:
          
           Query A: 
              
               FROM default Demographics SELECT field(Total people) last() GROUP BY time(1s) fill(none) FORMAT AS Time Series ALIAS BY Number of People
          
           Query B:

               FROM default Demographics SELECT field(Total male) last() GROUP BY time(1s) fill(none) FORMAT AS Time Series  ALIAS BY Number of male
            
           Query C:

               FROM default Demographics SELECT field(Total female) last() GROUP BY time(1s) fill(none) FORMAT AS Time Series ALIAS BY Number of female

         * From the **Column Styles** on the menu tab, click on **+Add** and in the **Apply to columns named** give the name as **No.of people** and under *Type*, select **Number** in **Type** and write **0** in the **Decimals** field as shown in the image below.

        ![Grafana](./images/Grafana8.png)

         * Similarly in **Column Styles** add the rules for *No. of male* and *No. of female* and select **Number** in **Type** and write **0** in the **Decimals** field.
         * Save the dashboard and click on **Back to dashboard** icon.
        
     7. Add the graph for displaying the people interested in viewing the ad:
         * Click on the **add panel** icon on the top menu and click on **Graph**. This will add the Graph to the dashboard.
         * Click on the **Panel Title** of the Graph and select **Edit**. 
         * Click on **General** in menu tab and change the Panel Title to *Audience interested in viewing the Ad*. 
         * Click on **Metrics** in Graph menu tab. From **Data Source** choose **AdData**.
         * Configure the following queries under **Metrics**:

            Query A:

                FROM default AdData SELECT field(peopleInterested) last() GROUP BY time(10s) fill(null) FORMAT AS Time Series ALIAS BY People Interested

            Query B:

                FROM default AdData SELECT field(peopleNotInterested) last() GROUP BY time(10s) fill(null) FORMAT AS Time Series ALIAS BY People Not Interested

         * Click on **Display** in Graph menu tab and change the values of **Fill** and **Line Width** under **Mode Options** to **2**. 
         * Click on **Time Range** present on the menu tab and write **500s** in the text box provided beside **Override relative time** **Last**.   
         * Save the dashboard and click on **Back to dashboard** icon.        
      
     8. Add the table for displaying the people interested in viewing the ad:
         * Click on the **add panel** icon on the top menu and click on **Table**.
         * Click on the **Panel Title** of the Table and select **Edit**. 
         * Click on **General** in Table menu tab and change the Panel Title to *Ad Data*.
         * Click on **Metrics** in Table menu tab. From **Data Source** choose **AdData**.  
         * Configure the following queries under **Metrics**:

            Query A:

                FROM default AdData SELECT field(previousAd) last() GROUP BY time(1s) fill(none) FORMAT AS Time Series ALIAS BY Ad name           

            Query B:

                FROM default AdData SELECT field(peopleInterested) last() GROUP BY time(1s) fill(none) FORMAT AS Time Series ALIAS BY People Interested

            Query C:

                FROM default AdData SELECT field(peopleNotInterested) last() GROUP BY time(1s) fill(none) FORMAT AS Time Series ALIAS BY People Not Interested      

          * In **Column Styles** add the rules for *People interested* and *People not interested* and select **Number** in **Type** and write **0** in the **Decimals** field.   
          * Save the dashboard and click on **Back to dashboard** icon.

     9. Display the ad currently playing on the dashboard:
          * Select **Singlestat**, Click on the **Panel Title** and select **Edit**.
          * Click on **General** in Singlestat menu tab and change the Panel Title to *Ad currently playing*.
          * Click on **Metrics** in Singlestat menu tab. From **Data Source** choose **AdData**. 
          * Configure the following queries under **Metrics**:

            Query A:

                FROM default AdData SELECT field(currentAd) last() GROUP BY time($__interval) fill(null) FORMAT AS Time Series

          * Save the dashboard and click on **Back to dashboard** icon. 

     10. To add *Singlestat panel* for *Number of unique visitors* to the dashboard, repeat step 2 with this modifications:
          * Instead of *Number of people*, set the panel title to *Number of unique visitors*.
          * Select **Unique visitors** in the **field (value)** of **Metrics**  Singlestat menu option.

            Query A in this case will be: 

              FROM default Demographics SELECT field((Unique visitors)) last() GROUP BY time(1m) fill(null) FORMAT AS Time Series

          * Click on **Options** in Singlestat menu tab. Select **Current** from **stat** field under *Value* and change the **Font size** to *120*, in the first row.        
          * Save the dashboard and click on **Back to dashboard** icon.

     11.  Select **Time picker** from the top menu of dashboard. Under **Custom range** change the **From** value to **now-10m** , **To** to **now** and **Refreshing every:** to **5s**, click on **Apply** and save the dashboard.

       ![Grafana](./images/Grafana9.png)

  * Run the C++ code to visualize data on grafana.

  * Panels can be arranged on the dashboard by resizing and dragging. 

       ![Grafana](./images/Grafana10.png)
       

    ####  Alternatively, visualization on Grafana can be done by following these steps:

    1. In your browser, go to localhost:3000.
    2. Log in with user as **admin** and password as **admin**.
    3. Click on **Configuration**.
    4. Select **“Data Sources”**.
    5. Click on **“+ Add data source”** and provide these settings: 
        * *Name*: Demographics
        * *Type*: InfluxDB
        * *URL*: http://localhost:8086
        * *Database*: Demographics
        * Click on “Save and Test”

        Click on **Back** button at the bottom of the page. Click on **“+ Add data source”** and provide these settings:
        * *Name*: AdData
        * *Type*: InfluxDB
        * *URL*: http://localhost:8086
        * *Database*: AdData
        * Click on “Save and Test”

    6. Click on **+**  icon present on the left side of the browser, select **import**.
    7. Click on **Upload.json File**.
    8. Select the file name "kiosk-analytics.json" from kiosk-analytics directory.
    9. Select "Demographics" in **Select a influxDB data source** beside *Demographics* and "AdData" data source in **AdData** field.
   10. Click on import.  
   11. Run the application to see the data on Grafana.




        
        


                







      

















