/*
* Copyright (c) 2018 Intel Corporation.
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
*/

#include <iostream>
#include <fstream>
#include <opencv2/opencv.hpp>
#include <signal.h>


// Number of instances in json file. 
#define TOTAL_JSON_INSTANCES  8

// Pipes for communication between process 
#define P1_READ     0
#define P2_WRITE    1
#define P2_READ     2
#define P1_WRITE    3

/*
* Age Range:
* Child: 1 to 13 yrs -> 1
* Young Adult: 14 to 28 yrs -> 2
* Adult: 28 to 50 yrs -> 3
* Senior: 50+ -> 4
*/

#define CHILD 1
#define YOUNG_ADULT 2
#define ADULT 3
#define SENIOR 4

/*
* Structure to store demographics
*/
struct DemographicsStructure {
    int peopleCount;           /* Total number of people in the frame */
    int interestedCount;       /* Number of people looking straight at the screen i.e they are interested in the ad*/
    struct Male {
        int count;             /* Number of male in the frame */ 
        int ageGroup[5];       /* array containing count of male falling in age range 1 to 4 stored in address from 1 to 4 */
    } male;
    struct Female {
        int count;             /* Number of female in the frame */     
        int ageGroup[5];       /* array containing count of female falling in age range 1 to 4 stored in address from 1 to 4 */
    } female;
};

/* 
* Structure to store values from json file containing the list of ad 
*/
struct adsDataStructure{			
  unsigned int ageGroup;       
  char gender;
  std::vector<std::string> Ads; /* list of ad for age group "ageGroup" and gender "gender" member of this structure */
};

/*
* Circular array variable of type DemographicsStructure of size 5 to store the demographics of 5 frames
* Every 5th frame of the input video is processed for audience analytics, output of which is stored in "demographics"\
* Declared in interactive_face_detection.cpp
*/
extern struct DemographicsStructure demographics[5];

/*
* Structure in which the data parsed from the json file is stored. Declared in json_parser.cpp
*/
extern struct adsDataStructure adsData[TOTAL_JSON_INSTANCES];  

/*
* Parse json file that contains the list of ads for particular age and gender and store it in "adsData" variable
*
* @param  Json file name containing ad list in a particular format
* @return  "false" if any error occurs while parsing the file else "true" 
*/
bool parseJsonFile(std::string );

/*
* Load the model, which is in the form of Intermediate Representation, in the memory
*
* @param All command line arguments which contains path to IR model for face detection, age-gender detection, head pose estimation
* @return 0 on success, 1 on failure
*/
int loadModel(int argc, char* argv[]);

/*
* Detects faces, age, gender and head pose for each face and store the data in demographics variable every 5th frame
*
* @param frame of type Mat on which the detection has to be made
* @return 0 on success, 1 on failure
*/
int analysePeople(cv::Mat frame);

/*
* Takes the h265 input video, decodes it using hevc plugin and renders it
*
* @param h265 video file name
* @return 0 on success, 1 on failure
*/
int media_sdk(const char*);
