#include <iostream>
#include <algorithm>
#include <fstream>
#include <chrono>
#include <string>

#include <zmq.hpp>

#include <opencv2/core/core.hpp>

#include "../../include/System.h"
#include "json.hpp"

using json = nlohmann::json;
using namespace std;

void empty_free(void *data, void *hint) {return;}

int main(int argc, char **argv)
{
    if(argc != 4)
    {
        cerr << endl << "Usage: ./zmq_rgbd port path_to_vocabulary path_to_settings" << endl;
        return 1;
    }

    // Create SLAM system. It initializes all system threads and gets ready to process frames.
    ORB_SLAM2::System SLAM(argv[2],argv[3],ORB_SLAM2::System::RGBD,true);

    // Initialize zmq
    zmq::context_t ctx(1);
    zmq::socket_t socket (ctx, ZMQ_REP);

    std::string addr = "tcp://*:";
    addr.append(argv[1]);
    socket.bind(addr.c_str());
    std::cout << "Bound on " << addr << std::endl;

    cv::Mat rgb_img, d_image;
    double timer;
    for(;;) {
        // zmq recv
        zmq::message_t mr_json;
        zmq::message_t mr_data_rgb;
        zmq::message_t mr_data_d;
        socket.recv(&mr_json);
        std::string temp((const char*)mr_json.data(), mr_json.size());
        json metadata = json::parse(temp);
        socket.recv(&mr_data_rgb);
        socket.recv(&mr_data_d);

        // to cv::Mat
        std::cout << metadata["shape"][0] << "x" << metadata["shape"][1] << std::endl;
        cv::Mat rgb_img(metadata["shape"][0], metadata["shape"][1], CV_8UC3, mr_data_rgb.data());
        cv::Mat d_img(metadata["shape"][0], metadata["shape"][1], CV_32FC1, mr_data_d.data());

        cv::Mat pose = SLAM.TrackRGBD(rgb_img,d_img,metadata["timer"]);

        // zmq send
        json out_metadata = {
            {"shape", {pose.cols, pose.rows}},
            {"dtype", "float32"},
        };
        std::string out_metadata_buff = out_metadata.dump();
        zmq::message_t ms_json((void*)out_metadata_buff.c_str(), out_metadata_buff.size(), empty_free);
        zmq::message_t ms_data((void*)pose.data, pose.cols*pose.rows*4, empty_free); // 4=sizeof(float32)
        socket.send(ms_json, ZMQ_SNDMORE);
        socket.send(ms_data);
        printf("Answer sent, looping.\n");
    }

    // Stop all threads
    SLAM.Shutdown();

    // Save camera trajectory
    SLAM.SaveKeyFrameTrajectoryTUM("KeyFrameTrajectory.txt");

    return 0;
}
