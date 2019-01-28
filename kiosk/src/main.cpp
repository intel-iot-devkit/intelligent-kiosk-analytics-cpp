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

#include <fcntl.h>
#include "influxdb.hpp"
#include "main.hpp"


// Macros defining which address in "pCount" variable contains what data
// pCount variable contains the demographics data
#define NO_OF_PEOPLE 0
#define NO_OF_MALE 1
#define NO_OF_FEMALE 2
#define NO_OF_PEOPLE_INTERESTED 3

// Store the gender and age group based on which the ad will play
struct playAdForData 
{
    char gender;
    unsigned int ageGroup;
};



/*
* Send the total number of people, no. of male and no. of female to influxDB "Demographics"
*
* @param Number of people currently in front of digital signage
* @param Number of male
* @param Number of female
* @param Number of unique visitors
*/
void writeToDemographicsInfluxDB(int people, int male, int female, int uniqueCount)
{
    std::string resp;
    influxdb_cpp::server_info serverInfo("127.0.0.1", 8086, "Demographics");
    int status = influxdb_cpp::builder()
                  .meas("Demographics")
                  .field("Total people", people)
                  .field("Total female", female)
                  .field("Total male", male)
                  .field("Unique visitors", uniqueCount)
                  .post_http(serverInfo);
    if(status != 0)
    {
        std::cout<<"Error writing data to InfluxDB Demographics"<<std::endl;
    }
}



/*
* Send the advertisement name and number of people watched it to influxDB "AdData"
*
* @param Name of the previous ad which was played
* @param Name of ad currently playing
* @param Number of people who were interested in previous ad among the total number of people
* @param Number of people who were not interested in previous ad
*/
void  writeToAdDataInfluxDB(std::string previousAd, std::string currentAd ,int interested, int notInterested)
{
    influxdb_cpp::server_info serverInfo("127.0.0.1", 8086, "AdData");
    int status = influxdb_cpp::builder()
                     .meas("AdData")
                     .field("previousAd", previousAd)
                     .field("currentAd", currentAd)
                     .field("peopleInterested", interested)
                     .field("peopleNotInterested", notInterested)
                     .post_http(serverInfo);
    if(status != 0)
    {
        std::cout<<"Error writing data to InfluxDB AdData"<<std::endl;
    }
}



/*
* Select the Advertisement to be played
*
* @param Structure containing gender and age group based on which ad has to be selected
* @return Advertisement name on success, "NULL" in case of failure
*/
std::string getAd(struct playAdForData genderAgeData)
{ 
    // To avoid Advertisement repetition
    static unsigned int AdIndex[TOTAL_JSON_INSTANCES] = {0};

    // Iterate through the data in variable adsData
    for (int instance = 0; instance < TOTAL_JSON_INSTANCES; instance++)
    {
        if ((adsData[instance].ageGroup == genderAgeData.ageGroup) && (adsData[instance].gender == genderAgeData.gender))
        {
            if (AdIndex[instance] >= adsData[instance].Ads.size())
                AdIndex[instance] = 1;
            else
                AdIndex[instance]++;
            return adsData[instance].Ads[AdIndex[instance] - 1];
        }
    }
    return "NULL";
}



/*
* Find the total number of people, number of male, number of female.
* It gets the data from  circular array "demographics" defined in main.hpp and find the mean of the respective data to get the demographics
*
* @param Float array of size 4, which will be updated with the demographics data
*/
void getPeopleCount(float pCount[])
{
    pCount[NO_OF_PEOPLE] = 0.0f;
    pCount[NO_OF_MALE] = 0.0f;
    pCount[NO_OF_FEMALE] = 0.0f;
    pCount[NO_OF_PEOPLE_INTERESTED] = 0.0f;
    float femaleCount =0.0f;
    float maleCount = 0.0f;
    float totalCount = 0.0f;

    // Find the sum of the total people, number of the male, female and people interested in the ad count 
    for (int i = 0; i < 5; i++)
    {
        pCount[NO_OF_PEOPLE] = pCount[NO_OF_PEOPLE] + demographics[i].peopleCount;
        pCount[NO_OF_MALE] = pCount[NO_OF_MALE] + demographics[i].male.count;
        pCount[NO_OF_FEMALE] = pCount[NO_OF_FEMALE] + demographics[i].female.count;
        pCount[NO_OF_PEOPLE_INTERESTED] = pCount[NO_OF_PEOPLE_INTERESTED] + demographics[i].interestedCount;
    }

   // Find the mean to remove to the inconsistency in the data if any
    totalCount = pCount[NO_OF_PEOPLE]/5;
    maleCount = pCount[NO_OF_MALE]/5;
    femaleCount = pCount[NO_OF_FEMALE]/5;

    pCount[NO_OF_PEOPLE] = floor(totalCount + 0.5);
    pCount[NO_OF_MALE] = floor(maleCount + 0.5);
    pCount[NO_OF_FEMALE] = floor(femaleCount  + 0.5);
    pCount[NO_OF_PEOPLE_INTERESTED] = floor(pCount[NO_OF_PEOPLE_INTERESTED]/5 + 0.5);

    // Remove the error in the mismatch of total number of people and sum of male and female, if any
    if(pCount[NO_OF_PEOPLE] < pCount[NO_OF_MALE] + pCount[NO_OF_FEMALE])
    {
        pCount[NO_OF_PEOPLE] = ceil(totalCount);
    }
    else
    if(pCount[NO_OF_PEOPLE] > pCount[NO_OF_MALE] + pCount[NO_OF_FEMALE])
    {
        if( femaleCount > maleCount)
            pCount[NO_OF_FEMALE] = ceil(femaleCount);
        else
            pCount[NO_OF_MALE] = ceil(maleCount);
    }
   
}



/*
* Find the unique count of people who visited kiosk
*
* @param Number of people currently in front of digital signage
* @return Count of unique visitors
*/
int getUniqueVisitorCount(int peopleCount)
{
    static int previousPeopleCount = 0;
    static int uniqueCount = 0;
    if(previousPeopleCount == 0 )
    {
        uniqueCount = peopleCount + uniqueCount; 
    }
    else if(previousPeopleCount < peopleCount)
    {
        uniqueCount = uniqueCount + peopleCount - previousPeopleCount;
    }
    previousPeopleCount = peopleCount;
    return uniqueCount;
}



/*
* Find the dominant age among the dominant gender
*
* @param Array of type float containing the demographics data
* @return Structure containing age and gender for which the ad has to be played
*/
struct playAdForData getGenderAgeGroup(float pCount[])
{
    struct playAdForData data;
    char gender;
    int ageGroup, maxAgeGroup, max;
    float meanAge[5] = {0};

    // Check people of which gender is more in number
    if (pCount[NO_OF_MALE] > pCount[NO_OF_FEMALE])
    {
        gender = 'M';
        for (int i = 0; i < 5; i++)
        {
            for (int ageGroup = 1; ageGroup < 5; ageGroup++)
            {
               meanAge[ageGroup] = meanAge[ageGroup] + demographics[i].male.ageGroup[ageGroup];
            }
        }
    }
    else
    {
        gender = 'F';
        for (int i = 0; i < 5; i++)
        {
            for (int ageGroup = 1; ageGroup < 5; ageGroup++)
            {
                meanAge[ageGroup] = meanAge[ageGroup] + demographics[i].female.ageGroup[ageGroup];
            }
        }
    }

    // Find the mean of the number of people on different ageGroup of the dominant gender
    for (int ageGroup = 1; ageGroup < 5; ageGroup++)
    {
        meanAge[ageGroup] = meanAge[ageGroup] / 5;
    }

    max = meanAge[1], maxAgeGroup = 1;
    // Find the dominant age group (Age group having maximum number of people)
    for (int ageGroup = 1; ageGroup < 5; ageGroup++)
    {
        if (max < meanAge[ageGroup])
        {
            maxAgeGroup = ageGroup;
            max = meanAge[ageGroup];
        }
    }    
    data.gender = gender;
    data.ageGroup = maxAgeGroup;
    return data;
} 



/*
* main()
*/
int main(int argc, char *argv[])
{
    cv::VideoCapture capture;
    cv::Mat frame;
    float pCount[4] ={0.0f};
    int fd[4];
    int flag = 0; 
    int frameCount = 1;
    int status = 0;
    int uniqueVisitors = 0;
    pid_t PID;
    std::string adToPlay = "\0";
    std::string previousAd = "\0";
    std::string adName = "\0";
    std::string response = "\0";
    std::string fileName = "../AdList.json";    
    std::string pathToAds = "../data/";


    // Default gender and age group for which ad needs to be played if any error occurres
    // or if their is no person in front of digital signage 
    struct playAdForData genderAgeData = {'M',2};
    
    // Parse the json file contaning the list of ads and store in "adsDataStructure" structure
    if (parseJsonFile(fileName) == false)
    {
        std::cout<<"Error occurred while parsing the json file!"<<std::endl;
        return EXIT_FAILURE;
    }


    // Read the Intermediate representation (read network model and load its weights)
    if (loadModel(argc, argv) == 1)
    {
        std::cout << "Error occurred while reading Intermediate Representation" << std::endl;
        return EXIT_FAILURE;
    }

    // Create databases Demographics and AdData in InfluxDB to store data
    influxdb_cpp::server_info serverInfo("127.0.0.1", 8086);
    influxdb_cpp::create_db(response, "Demographics", serverInfo);
    influxdb_cpp::create_db(response, "AdData", serverInfo);

    // Create pipes for interprocess communication between Audience Analytics process and MediaSDK process
    for (int i = 0; i < 4; ++i)
    {
        if (pipe(fd + (i * 2)) < 0)
        {
            perror("Failed to allocate pipes");
            exit(EXIT_FAILURE);
        }
    }

    // Make the P1_READ pipe as non blocking    
    if (fcntl(fd[P1_READ], F_SETFL, O_NONBLOCK) < 0)
    {
        std::cout<<"Error occurred while configuring pipe"<<std::endl;
        exit(EXIT_FAILURE);
    }

    // Create a process which will decode Ad using H265 codec and play it
    PID = fork();
    
    // MediaSDK process for video decoding
    if (PID == 0)
    {
        // Close the pipe ends not required by child process
        close(fd[P1_READ]);
        close(fd[P1_WRITE]);
        int result = 0;
        std::string ad = "\0";
        char ch = '\0';
        while (read(fd[P2_READ], &ch, 1) > 0)
        {
            if (ch != 0)
                ad.push_back(ch);
            else
            {
                // Pass ad name to media_sdk() function to decode and play the ad 
                result = media_sdk(ad.c_str());
                if(result != 0)
                {
                    flag = 1;
                    // Send negative acknowledge to the audience analytics process if any error occurred while decoding
                    if(write(fd[P2_WRITE], &flag, sizeof(flag)) == -1)
                    {
                        std::cout<<"Error occurred while writing to the pipe!"<<std::endl;
                        exit(EXIT_FAILURE);
                    }
                    break;
                }
                else
                {
                    flag = 0;
                    // Send the acknowledge to the audience analytics process on success
                    if(write(fd[P2_WRITE], &flag, sizeof(flag)) == -1)
                    {
                        std::cout<<"Error occurred while writing to the pipe!"<<std::endl;
                        exit(EXIT_FAILURE);
                    }
                }
                ad.clear();
            }
        }
        close(fd[P2_READ]);
        close(fd[P2_WRITE]);
        exit(EXIT_SUCCESS);
    }
    // Process for audience analytics and sending data to influxDB 
    else
    {
        // Close the pipes not required by parent process
        close(fd[P2_READ]);
        close(fd[P2_WRITE]);
        if (strcmp(argv[2], "cam") == 0)
        {
            std::cout << "Input from camera " << std::endl;
            if(capture.open(0) == false)
            {
                std::cout<<"\nError opening the camera!\n"<<std::endl;
                kill(PID, SIGKILL);
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            std::cout << "Loading the video " << argv[2] << std::endl;
            if(capture.open(argv[2]) == false)
            {
                std::cout<<"\nError loading the video!\n"<<std::endl;
                kill(PID, SIGKILL);
                exit(EXIT_FAILURE);
            }
        }
        while (1)
        {
            capture >> frame;

            // If frame is empty, exit the application
            if (frame.empty())
            {
                // Kill the video decoding process
                kill(PID, SIGKILL);
                exit(EXIT_SUCCESS);
            }

            // Analyse the data till 30th frame and then play the ad along with publishing the data to Grafana
            if (frameCount == 30)
            {
                memset(pCount, 0, sizeof(pCount));

                // Find the total number people of people, number of male and female
                getPeopleCount(pCount);

                // Get the unique count of visitors
                uniqueVisitors = getUniqueVisitorCount(pCount[NO_OF_PEOPLE]);

                // Write the demographics data to InfluxDB 
                writeToDemographicsInfluxDB(pCount[NO_OF_PEOPLE], pCount[NO_OF_MALE], pCount[NO_OF_FEMALE], uniqueVisitors);
                
                // Check if there are people in front of digital signage
                if ((pCount[NO_OF_PEOPLE]) != 0)
                {
                    // Get dominant gender and Age Group and store it in genderAgeData
                    genderAgeData = getGenderAgeGroup(pCount);
                }

                // Select the ad to be played based on demographics
                adToPlay = getAd(genderAgeData);

                if(adToPlay == "NULL")
                {
                    std::cout<<"Error occurred while selecting the ad!"<<std::endl;
                    break;
                }
                std::cout<<"\n\n\n*********** Playing Add for Gender : "<<genderAgeData.gender<<", Age Group : "<<genderAgeData.ageGroup<<" ***********\n";
                std::cout<<"*********** Playing Ad: "<<adToPlay<<"***********\n\n\n";
                adName = pathToAds + adToPlay;

                // Send the ad name to video decoding process
                if(write(fd[P1_WRITE], adName.c_str(), adName.length() + 1) == -1)
                {
                    std::cout<<"Error occurred while writing to the pipe!"<<std::endl;
                    kill(PID, SIGKILL);
                    exit(EXIT_FAILURE);
                }
            }

            // Send the demographics data every 30th frame (approx 1 sec)
            if(frameCount % 30 == 0)
            {
                getPeopleCount(pCount);
                uniqueVisitors = getUniqueVisitorCount(pCount[NO_OF_PEOPLE]);
                std::cout<<"\nUnique visitors count : "<<uniqueVisitors<<std::endl; 
                writeToDemographicsInfluxDB(pCount[NO_OF_PEOPLE], pCount[NO_OF_MALE], pCount[NO_OF_FEMALE], uniqueVisitors);
            }

            // Get the acknowledgment of ad completion from video decoding process 
            if (read(fd[P1_READ], &flag, sizeof(flag)) != -1)
            {   
                if (flag == 1)
                {
                    std::cout<<"Error occurred while playing the ad!"<<std::endl;
                    break;
                }
                if (flag == 0)
                {
                    // Find the total number people of people, number of male and female
                    getPeopleCount(pCount);
                    previousAd = adToPlay;

                    // Check if there are people in front of digital signage
                    if ((pCount[NO_OF_PEOPLE]) != 0)
                    {
                        std::cout<<"\nPeople interested: "<<pCount[NO_OF_PEOPLE_INTERESTED]<<std::endl;
                        // Get dominant gender and Age Group and store it in genderAgeData
                        genderAgeData = getGenderAgeGroup(pCount);
                    }

                    // Select the ad to be played based on demographics
                    adToPlay = getAd( genderAgeData );
                    int notInterested  = pCount[NO_OF_PEOPLE] - pCount[NO_OF_PEOPLE_INTERESTED];
                    writeToAdDataInfluxDB(previousAd, adToPlay, pCount[NO_OF_PEOPLE_INTERESTED], notInterested);
                    if(adToPlay == "NULL")            
                    {
                        // Default gender and age group for which ad needs to be played if any error occurres 
                        // or if their is no person in front of digital signage  
                        genderAgeData = {'M',2};
                        std::cout<<"Error occurred while selecting the ad!"<<std::endl;
                        adToPlay = getAd( genderAgeData );
                        adName = pathToAds + adToPlay;
                        if(write(fd[P1_WRITE], adName.c_str(), adName.length() + 1) == -1)
                        {
                            std::cout<<"Error occurred while writing to the pipe!"<<std::endl;
                            kill(PID, SIGKILL);
                            exit(EXIT_FAILURE);
                        }
 
                    }
                    else
                    {
                        std::cout<<"\n\n\n*********** Playing Ad for Gender : "<<genderAgeData.gender<<", Age Group : "<<genderAgeData.ageGroup<<" ***********\n";
                        std::cout<<"*********** Playing Ad: "<<adToPlay<<" ***********\n\n\n";
                        adName = pathToAds + adToPlay;
                        if(write(fd[P1_WRITE], adName.c_str(), adName.length() + 1) == -1)
                        {
                            std::cout<<"Error occurred while writing to the pipe!"<<std::endl;
                            kill(PID, SIGKILL);
                            exit(EXIT_FAILURE);
                        }
                    }
                }
            }

            /*
            * analysePeople function is defined in interactive_face_detection.cpp file.
            * It analysis the audience in front of digital signage and store the age and gender of the people in 
            * the "demographics" circular array defined in main.hpp
            */  
            status = analysePeople(frame);
            if (status != 0)
            {
                std::cout<<"Error occurred while analysing the audience"<<std::endl;
                break;
            }
            frameCount++;

        }
    }
    // finished with write-side
    close(fd[P1_READ]);
    close(fd[P1_WRITE]);
}
