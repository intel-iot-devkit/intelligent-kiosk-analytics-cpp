// Copyright (C) 2018-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
//

/**
* \brief The entry point for the Inference Engine interactive_face_detection demo application
* \file interactive_face_detection_demo/main.cpp
* \example interactive_face_detection_demo/main.cpp
*/
#include <gflags/gflags.h>
#include <functional>
#include <iostream>
#include <fstream>
#include <random>
#include <memory>
#include <chrono>
#include <vector>
#include <string>
#include <utility>
#include <algorithm>
#include <iterator>
#include <map>
#include <list>

#include <inference_engine.hpp>

#include <samples/ocv_common.hpp>
#include <samples/slog.hpp>
#include "main.hpp"
#include "interactive_face_detection.hpp"
#include "detectors.hpp"
#include "face.hpp"
#include "visualizer.hpp"

#include <ie_iextension.h>
//#include <ext_list.hpp>



using namespace InferenceEngine;

struct DemographicsStructure demographics[5];

FaceDetection *faceDetector;
AgeGenderDetection *ageGenderDetector;
HeadPoseDetection *headPoseDetector;
 
//InferencePlugin plugin;

bool ParseAndCheckCommandLine(int argc, char *argv[]) {
    // ---------------------------Parsing and validating input arguments--------------------------------------
    gflags::ParseCommandLineNonHelpFlags(&argc, &argv, true);
    if (FLAGS_h) {
        showUsage();
        showAvailableDevices();    
        return false;
    }
    slog::info << "Parsing input parameters" << slog::endl;

    if (FLAGS_m.empty()) {
        throw std::logic_error("Parameter -m is not set");
    }

    if (FLAGS_n_ag < 1) {
        throw std::logic_error("Parameter -n_ag cannot be 0");
    }

    if (FLAGS_n_hp < 1) {
        throw std::logic_error("Parameter -n_hp cannot be 0");
    }

    // no need to wait for a key press from a user if an output image/video file is not shown.
    FLAGS_no_wait |= FLAGS_no_show;

    return true;
}




// Function to find the age Group in which the age of the person lies
int GetAgeGroup(int age)
{
    int ageGrp = 0;
    if(age > 0 && age < 14)
        ageGrp = CHILD;
    else   
    if(age >= 14 && age < 29)
        ageGrp = YOUNG_ADULT;
    else   
    if(age >=29 && age < 50)
        ageGrp = ADULT;
    else   
    if(age >= 50 )
        ageGrp = SENIOR;

    return ageGrp;
}


int loadModel(int argc, char* argv[])
    try {
        std::cout << "InferenceEngine: " << GetInferenceEngineVersion() << std::endl;

        // ------------------------------ Parsing and validating of input arguments --------------------------
        if (!ParseAndCheckCommandLine(argc, argv)) {
            return 0;
        }
        
        // ---------------------------------------------------------------------------------------------------
        // --------------------------- 1. Loading plugin to the Inference Engine -----------------------------
        Core ie;

        std::set<std::string> loadedDevices;
        std::pair<std::string, std::string> cmdOptions[] = {
            {FLAGS_d, FLAGS_m},
            {FLAGS_d_ag, FLAGS_m_ag},
            {FLAGS_d_hp, FLAGS_m_hp}
        };
        
        faceDetector = new FaceDetection(FLAGS_m, FLAGS_d, 1, false, FLAGS_async, FLAGS_t, FLAGS_r,
                                   static_cast<float>(FLAGS_bb_enlarge_coef), static_cast<float>(FLAGS_dx_coef),
                                   static_cast<float>(FLAGS_dy_coef));
        ageGenderDetector = new AgeGenderDetection(FLAGS_m_ag, FLAGS_d_ag, FLAGS_n_ag, FLAGS_dyn_ag, FLAGS_async,
                                                    FLAGS_r);
        headPoseDetector = new HeadPoseDetection(FLAGS_m_hp, FLAGS_d_hp, FLAGS_n_hp, FLAGS_dyn_hp, FLAGS_async,
                                                    FLAGS_r);
       
        for (auto && option : cmdOptions) {
            auto deviceName = option.first;
            auto networkName = option.second;

            if (deviceName == "" || networkName == "") {
                continue;
            }

            if (loadedDevices.find(deviceName) != loadedDevices.end()) {
                continue;
            }
            slog::info << "Loading device " << deviceName << slog::endl;
            std::cout << ie.GetVersions(deviceName) << std::endl;
            
            /** Loading extensions for the CPU plugin **/
            /*
	    if ((deviceName.find("CPU") != std::string::npos)) {
                plugin.AddExtension(std::make_shared<Extensions::Cpu::CpuExtensions>());

                if (!FLAGS_l.empty()) {
                    // CPU(MKLDNN) extensions are loaded as a shared library and passed as a pointer to base extension
                    auto extension_ptr = make_so_pointer<IExtension>(FLAGS_l);
                    plugin.AddExtension(extension_ptr);
                    slog::info << "CPU Extension loaded: " << FLAGS_l << slog::endl;
                }
            }*/
            if (!FLAGS_c.empty()) {
                // Loading extensions for GPU
                 ie.SetConfig({{PluginConfigParams::KEY_CONFIG_FILE, FLAGS_c}}, "GPU");
            }
            loadedDevices.insert(deviceName);
        }

        /** Per-layer metrics **/
        if (FLAGS_pc) {
            ie.SetConfig({{PluginConfigParams::KEY_PERF_COUNT, PluginConfigParams::YES}});
        }
        // ---------------------------------------------------------------------------------------------------

        // --------------------------- 2. Reading IR models and loading them to plugins ----------------------
        // Disable dynamic batching for face detector as it processes one image at a time
        Load(*faceDetector).into(ie, FLAGS_d, false);
        Load(*ageGenderDetector).into(ie, FLAGS_d_ag, FLAGS_dyn_ag);
        Load(*headPoseDetector).into(ie, FLAGS_d_hp, FLAGS_dyn_hp);
        if(FLAGS_async == 0)
            std::cout<<"Application running in sync mode"<<std::endl;
        else
        {
            std::cout<<"Application running in Async mode"<<std::endl;
            faceDetector->isAsync = true;
            ageGenderDetector->isAsync = true;
            headPoseDetector->isAsync = true;
        }
        return 0;
        // ----------------------------------------------------------------------------------------------------
    }
    catch (const std::exception& error) {
        slog::err << error.what() << slog::endl;
        return 1;
    }
    catch (...) {
        slog::err << "Unknown/internal exception happened." << slog::endl;
        return 1;
    }


int analysePeople(cv::Mat frame) {
        Timer timer;
        static int frameCount = 0, dataCount = 0;
        if(dataCount == 5 && frameCount % 5 == 0)
            dataCount = 0;
        
        if(frameCount % 5 == 0)
        {
            demographics[dataCount] = {0};
        }

        cv::namedWindow("Detection results", cv::WINDOW_NORMAL );
        
        const size_t width  = static_cast<size_t>(frame.cols);
        const size_t height = static_cast<size_t>(frame.rows);

                // --------------------------- 3. Doing inference -----------------------------------------------------
        // Starting inference & calculating performance
        bool isFaceAnalyticsEnabled = ageGenderDetector->enabled() || headPoseDetector->enabled();

        std::ostringstream out;
        int delay = 1;
        double msrate = -1;
        static std::list<Face::Ptr> faces;
        size_t id = 0;
        Visualizer::Ptr visualizer;
        
        if (!FLAGS_no_show) {
            visualizer = std::make_shared<Visualizer>(cv::Size(width, height));
        }
        // Detecting all faces on the first frame and reading the next one
        faceDetector->enqueue(frame);
        faceDetector->submitRequest();
        
        if(faceDetector->isAsync)
            faceDetector->request.swap(faceDetector->nxtrequest);
        timer.start("total");
        faceDetector->enqueue(frame);
        faceDetector->submitRequest();
        faceDetector->wait();
        faceDetector->fetchResults();
        auto prev_detection_results = faceDetector->results;
        
        // Filling inputs of face analytics networks
        for (auto &&face : prev_detection_results) {
            if (isFaceAnalyticsEnabled) {
                auto clippedRect = face.location & cv::Rect(0, 0, width, height);
                cv::Mat face = frame(clippedRect);
                ageGenderDetector->enqueue(face);
                headPoseDetector->enqueue(face);
            }
        }

        // Running Age/Gender Recognition, Head Pose Estimation, Emotions Recognition, and Facial Landmarks Estimation
        // networks simultaneously
        if (isFaceAnalyticsEnabled) {
            ageGenderDetector->submitRequest();
            headPoseDetector->submitRequest();
            if(ageGenderDetector->isAsync)
                ageGenderDetector->request.swap(ageGenderDetector->nxtrequest);
            if(headPoseDetector->isAsync)
                headPoseDetector->request.swap(headPoseDetector->nxtrequest);
        }

        // Reading the next frame if the current one is not the last

        if (isFaceAnalyticsEnabled) {
            ageGenderDetector->wait();
            headPoseDetector->wait();
        }

        //  Postprocessing
        std::list<Face::Ptr> prev_faces;

        if (!FLAGS_no_smooth) {
            prev_faces.insert(prev_faces.begin(), faces.begin(), faces.end());
        }

        faces.clear();

        // For every detected face
        for (size_t i = 0; i < prev_detection_results.size(); i++) {
            auto& result = prev_detection_results[i];
            cv::Rect rect = result.location & cv::Rect(0, 0, width, height);

            Face::Ptr face;
            if (!FLAGS_no_smooth) {
                face = matchFace(rect, prev_faces);
                float intensity_mean = calcMean(frame(rect));

                if ((face == nullptr) ||
                    ((face != nullptr) && ((std::abs(intensity_mean - face->_intensity_mean) / face->_intensity_mean)
                     > 0.07f))) {
                    face = std::make_shared<Face>(id++, rect);
                } else {
                    prev_faces.remove(face);
                }

                face->_intensity_mean = intensity_mean;
                face->_location = rect;
            } else {
                face = std::make_shared<Face>(id++, rect);
            }

            face->ageGenderEnable((ageGenderDetector->enabled() &&
                                   i < ageGenderDetector->maxBatch));
            if (face->isAgeGenderEnabled()) {
                AgeGenderDetection::Result ageGenderResult = (*ageGenderDetector)[i];
                face->updateGender(ageGenderResult.maleProb);
                face->updateAge(ageGenderResult.age);
                if(frameCount % 5 == 0) {
                    if (ageGenderResult.maleProb > 0.5) {
                        demographics[dataCount].male.count++;
                        int ageRange = GetAgeGroup(ageGenderResult.age);
                        demographics[dataCount].male.ageGroup[ageRange]++;
                    } else {
                        demographics[dataCount].female.count++;
                        int ageRange = GetAgeGroup(ageGenderResult.age);
                        demographics[dataCount].female.ageGroup[ageRange]++;
                    }
                }
                
            }

            face->headPoseEnable((headPoseDetector->enabled() &&
                                  i < headPoseDetector->maxBatch));
            if (face->isHeadPoseEnabled()) {
                face->updateHeadPose((*headPoseDetector)[i]);
                if((*headPoseDetector)[i].angle_y > -30 && (*headPoseDetector)[i].angle_y < 30)
                {
                    demographics[dataCount].interestedCount++;
                }
                else
                {
                    out<<" not interested ";
                }
            }

            faces.push_back(face);
            cv::rectangle(frame, result.location, cv::Scalar(0, 0, 255), 1);
        }
        if (!FLAGS_no_show) {
            out.str("");
            out << "Total image throughput: " << std::fixed << std::setprecision(2)
                << 1000.f / (timer["total"].getSmoothedDuration()) << " fps";
            cv::putText(frame, out.str(), cv::Point2f(10, 45), cv::FONT_HERSHEY_TRIPLEX, 1.2,
                        cv::Scalar(255, 0, 0), 2);

            // drawing faces
            visualizer->draw(frame, faces);

            if (!FLAGS_no_show) {
                cv::imshow("Detection results", frame);
                cv::waitKey(1);
            }
        }

        timer.finish("total");

        if(frameCount % 5 == 0)
        {
            demographics[dataCount].peopleCount = prev_detection_results.size();
            dataCount++;
        }
        
        // Showing performance results
        if (FLAGS_pc) {
            //faceDetector->printPerformanceCounts(getFullDeviceName(ie, FLAGS_d));
            //ageGenderDetector->printPerformanceCounts(getFullDeviceName(ie, FLAGS_d_ag));
            //headPoseDetector->printPerformanceCounts(getFullDeviceName(ie, FLAGS_d_hp));
        }
        // ---------------------------------------------------------------------------------------------------
        frameCount++;
    return 0;
}
