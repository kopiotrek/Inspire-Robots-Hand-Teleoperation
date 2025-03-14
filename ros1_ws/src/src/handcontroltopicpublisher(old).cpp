#include <ros/ros.h>
#include <hand_control.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <iostream>
#include <std_msgs/String.h>
#include "std_msgs/MultiArrayLayout.h"
#include "std_msgs/MultiArrayDimension.h"
#include "std_msgs/Int32MultiArray.h"

using namespace std;

int hand_id_;
std::string port_name_;
int baudrate_;
int test_flags_;
int Serial_flags_;
serial::Serial *com_port_;
int hand_state_;
float curangle_[6];
float curforce_[6];

void getANGLE_ACT1(serial::Serial *port)
{
    ros::Time start_time = ros::Time::now(); 
    std::vector<uint8_t> output;
    //message from master to module
    output.push_back(0xEB);
    output.push_back(0x90);
    //module id
    output.push_back(hand_id_);
    //Data Length
    output.push_back(0x01);
    //Command get state
    output.push_back(0xD7);
    //Checksum calculation
    unsigned int check_num = 0;
    int len = output[3] + 5;
    for (int i = 2; i < len - 1; i++)
        check_num = check_num + output[i];
    //Add checksum to the output buffer
    output.push_back(check_num & 0xff);
    //Send message to the module
    port->write(output);

    ros::Duration(0.015).sleep();

    std::string s1;
    for (int i = 0; i < output.size(); ++i)
    {
        char str[16];
        sprintf(str, "%02X", output[i]);
        s1 = s1 + str + " ";
    }
    if (test_flags_ == 1)
        ROS_INFO_STREAM("Write: " << s1);

    //Read response
    std::vector<uint8_t> input;
    while (input.empty())
    {
        port->read(input, (size_t)64);
    }

    ros::Duration duration = ros::Time::now() - start_time;
    double frequency = 1.0 / duration.toSec();  // 计算读取频率

    std::string s2;
    for (int i = 0; i < input.size(); ++i)
    {
        char str[16];
        sprintf(str, "%02X", input[i]);
        s2 = s2 + str + " ";
    }
    if (test_flags_ == 1)
        ROS_INFO_STREAM("Read: " << s2);
        
    ROS_INFO_STREAM("ANGLE reading frequency: " << frequency << " Hz");
    
    float temp[10] = {0.0};
    for (int j = 0; j < 6; j++)
        temp[j] = ((input[6 + j * 2] << 8) & 0xff00) + input[5 + j * 2];
    curangle_[0] = temp[0];
    curangle_[1] = temp[1];
    curangle_[2] = temp[2];
    curangle_[3] = temp[3];
    curangle_[4] = temp[4];
    curangle_[5] = temp[5];
}

void getFORCE_ACT1(serial::Serial *port)
{
    ros::Time start_time = ros::Time::now();
    std::vector<uint8_t> output;
    //message from master to module
    output.push_back(0xEB);
    output.push_back(0x90);
    //module id
    output.push_back(hand_id_);
    //Data Length
    output.push_back(0x04);
    //Command get state
    output.push_back(0x11);
    output.push_back(0x2E);
    output.push_back(0x06);
    output.push_back(0x0C);
    //Checksum calculation
    unsigned int check_num = 0;
    int len = output[3] + 5;
    for (int i = 2; i < len - 1; i++)
        check_num = check_num + output[i];
    //Add checksum to the output buffer
    output.push_back(check_num & 0xff);
    //Send message to the module
    port->write(output);

    ros::Duration(0.015).sleep();

    std::string s1;
    for (int i = 0; i < output.size(); ++i)
    {
        char str[16];
        sprintf(str, "%02X", output[i]);
        s1 = s1 + str + " ";
    }
    if (test_flags_ == 1)
        ROS_INFO_STREAM("Write: " << s1);

    //Read response
    std::vector<uint8_t> input;
    while (input.empty())
    {
        port->read(input, (size_t)64);
    }
    
    ros::Duration duration = ros::Time::now() - start_time;
    double frequency = 1.0 / duration.toSec();  // 计算读取频率
    
    // Log the frequency
    ROS_INFO_STREAM("FORCE reading frequency: " << frequency << " Hz");
    
    std::string s2;
    for (int i = 0; i < input.size(); ++i)
    {
        char str[16];
        sprintf(str, "%02X", input[i]);
        s2 = s2 + str + " ";
    }
    if (test_flags_ == 1)
        ROS_INFO_STREAM("Read: " << s2);

    int temp[10] = {0};
    for (int j = 0; j < 6; j++)
    {
        temp[j] = ((input[8 + j * 2] << 8) & 0xff00) + input[7 + j * 2];
        if (temp[j] > 32768)
            temp[j] = temp[j] - 65536;
    }

    curforce_[0] = float(temp[0]);
    curforce_[1] = float(temp[1]);
    curforce_[2] = float(temp[2]);
    curforce_[3] = float(temp[3]);
    curforce_[4] = float(temp[4]);
    curforce_[5] = float(temp[5]);
}

int main(int argc, char *argv[])
{
    ros::init(argc, argv, "handcontroltopicpublisher");
    ros::NodeHandle nh;

    //topic
    ros::Publisher chatter_pub = nh.advertise<std_msgs::Int32MultiArray>("chatter", 1000);

    ros::Rate loop_rate(10);

    //Read launch file params
    nh.getParam("inspire_hand/hand_id", hand_id_);
    nh.getParam("inspire_hand/portname", port_name_);
    nh.getParam("inspire_hand/baudrate", baudrate_);
    nh.getParam("inspire_hand/test_flags", test_flags_);
    
    //Initialize and open serial port
    com_port_ = new serial::Serial(port_name_, (uint32_t)baudrate_, serial::Timeout::simpleTimeout(100));

    while (ros::ok())
    {
        //get the param Serial_flags
        nh.getParam("inspire_hand/Serial_flags", Serial_flags_);
        if(Serial_flags_ == 0)
        {
            ROS_INFO_STREAM("close the Serial");
        }
        else if (Serial_flags_ == 1)
        {
            std_msgs::Int32MultiArray array;

            //Clear array
            array.data.clear();
            getANGLE_ACT1(com_port_);
            getFORCE_ACT1(com_port_);

            for (int i = 0; i < 6; i++)
            {
                //assign array a random number between 0 and 255.
                array.data.push_back(curangle_[i]);
            }
            for (int i = 0; i < 6; i++)
            {
                //assign array a random number between 0 and 255.
                array.data.push_back(curforce_[i]);
            }
            //Publish array
            chatter_pub.publish(array);
            loop_rate.sleep();
        }
    }

    return (EXIT_SUCCESS);
}
