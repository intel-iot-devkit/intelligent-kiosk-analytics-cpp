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


#include "main.hpp"

// "adsData" defined to store the list of ads based on demographics stored in json file
struct adsDataStructure adsData[TOTAL_JSON_INSTANCES];

// Function to parse json file containing the list of advertisement mapped according to demographics
bool parseJsonFile(std::string fileName) 
{
  std::string word, AdTemp;
  std::vector<std::string> words;
  unsigned int jsonInstanceCount = 0;
  unsigned int ageGroup;
  char gender;

  // Opening the file
  std::ifstream file(fileName); 

  if (!file) 
  {
    std::cout << "There was an error opening the file.\n";
    return false;
  }

  // Reading word by word from file and saving it in vector of string "word" 
  while (file >> word) 
    words.push_back(word);

  // Taking word-by-word from vector and storing it in structure
  for (unsigned int i = 0; i < words.size(); i+=3) 
  {
    if ((words[i] == "{" ))
    {
      if( (i > 0 ) )
        jsonInstanceCount++; 
      i++;
    }
  
    if (words[i] == "\"Age_Group\":")
    {
      ageGroup = stoi(words[i+1]);
      if(ageGroup < 1 || ageGroup > 4)
      {
        std::cout<<"Invalid Age Group found in json file at "<<jsonInstanceCount<< "th instance"<<std::endl;
        return false;
      }
      adsData[jsonInstanceCount].ageGroup = ageGroup;
      std::cout<<"ageGroup: "<<adsData[0].ageGroup<<std::endl;
      i += 2;
    }
    else
    {

      std::cout<<"Format error in json file at "<<jsonInstanceCount<< "th instance, ageGroup field"<<std::endl;
      return false;
    }
    
    if (words[i] == "\"Gender\":")
    {
      gender = words[i + 1][1];
      if( gender != 'M' && gender != 'm' &&  gender != 'F' && gender != 'f')
      {
        std::cout<<"Invalid gender found in json file at "<<jsonInstanceCount<< "th instance"<<std::endl;
        return false;
      }
      adsData[jsonInstanceCount].gender = gender;
      i += 2;
    }
    else
    {
        std::cout<<"Format error in json file at "<<jsonInstanceCount<< "th instance, Gender field"<<std::endl;;
        return false;
    }

    if (words[i++] == "\"Ads\":") 
    {
      if(words[i++] == "[")
      {
        while (words[i][words[i].length() - 1] == ',' ) 
        {
        
          AdTemp = words[i].substr(1, words[i].length()-3);
          adsData[jsonInstanceCount].Ads.push_back(AdTemp);
          AdTemp.clear();
          i++;
          
        }
        
        AdTemp = words[i].substr(1, words[i].length()-2);
        adsData[jsonInstanceCount].Ads.push_back(AdTemp); 
        AdTemp.clear();
      }
      else
      {
        std::cout<<"Format error in json file at "<<jsonInstanceCount<< "th instance, Ads field 1"<<std::endl;
          return false;
      }
    }
    else
    {
      std::cout<<"Format error in json file at "<<jsonInstanceCount<< "th instance, Ads field"<<std::endl;
        return false;
    }
  }
  return true;
}
