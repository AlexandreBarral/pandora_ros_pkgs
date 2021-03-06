/*********************************************************************
*
* Software License Agreement (BSD License)
*
*  Copyright (c) 2014, P.A.N.D.O.R.A. Team.
*  All rights reserved.
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
*
*   * Redistributions of source code must retain the above copyright
*     notice, this list of conditions and the following disclaimer.
*   * Redistributions in binary form must reproduce the above
*     copyright notice, this list of conditions and the following
*     disclaimer in the documentation and/or other materials provided
*     with the distribution.
*   * Neither the name of the P.A.N.D.O.R.A. Team nor the names of its
*     contributors may be used to endorse or promote products derived
*     from this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
*  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
*  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
*  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
*  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
*  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
*  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
*  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
*  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
*  POSSIBILITY OF SUCH DAMAGE.
*
* Author:  Evangelos Apostolidis
* Author:  George Kouros 
*********************************************************************/
#ifndef ARM_HARDWARE_INTERFACE_ARM_HARDWARE_INTERFACE_H
#define ARM_HARDWARE_INTERFACE_ARM_HARDWARE_INTERFACE_H


#include <boost/math/constants/constants.hpp>
#include "ros/ros.h"
#include "tf/tf.h"
#include <hardware_interface/robot_hw.h>
#include <controller_manager/controller_manager.h>
#include <arm_hardware_interface/co2_sensor_interface.h>
#include <arm_hardware_interface/thermal_sensor_interface.h>
#include <arm_hardware_interface/range_sensor_interface.h>
#include <arm_hardware_interface/battery_interface.h>
#include <hardware_interface/joint_state_interface.h>
#include <arm_usb_interface/arm_usb_interface.h>

namespace pandora_hardware_interface
{
namespace arm
{
  /**
   @class ArmHardwareInterface
   @brief Allows the controller manager to communicate with the arm board
  **/
  class ArmHardwareInterface : public hardware_interface::RobotHW
  {
    public:
      /**
       @brief Default Constructor
       @details Establishes communication and registers thermal and CO2 
       interfaces
       @param nodeHandle [ros::NodeHandle] : handle of 
       arm_hardware_interface_node
      **/
      explicit ArmHardwareInterface(
        ros::NodeHandle nodeHandle);

      /**
       @brief Default Constructor
       @details Frees ArmUSBInterface instance
      **/
      ~ArmHardwareInterface();

      /**
       @brief Reads CO2 percentage and grideyes' values
       @return void
      **/
      void read();

    private:
     /**
      @brief Registers CO2 sensors interface
      @return void
     **/
     void registerCo2SensorInterface();

     /**
      @brief Registers CO2 sensors interface
      @return void
     **/
     void registerThermalSensorInterface();

     /**
      @brief Registers CO2 sensors interface
      @return void
     **/
     void registerRangeSensorInterface();

     /**
      @brief Registers CO2 sensors interface
      @return void
     **/
     void registerJointStateInterface();

     /**
      @brief Registers CO2 sensors interface
      @return void
     **/
     void registerBatteryInterface();

    private:
     ros::NodeHandle nodeHandle_;
     ArmUsbInterface* arm_;

     /* CO2 sensor */
     Co2SensorInterface co2SensorInterface_;
     std::vector<Co2SensorHandle::Data>
       co2SensorData_;
     std::vector<std::string> co2SensorName_;
     std::vector<std::string> co2SensorFrameId_;
     float* co2Percentage_;

     /* thermal sensors */
     ThermalSensorInterface thermalSensorInterface_;
     std::vector<ThermalSensorHandle::Data>
       thermalSensorData_;
     std::vector<std::string> thermalSensorName_;
     std::vector<std::string> thermalFrameId_;
     int* height_;
     int* width_;
     int* step_;
     uint8_t** thermalData_;
     char* thermalSensorCode_;  // not stored in handle

     /* range sensors */
     RangeSensorInterface rangeSensorInterface_;
     std::vector<RangeSensorHandle::Data> rangeSensorData_;
     std::vector<std::string> rangeSensorName_;
     std::vector<std::string> rangeSensorFrameId_;
     int* radiationType_;
     double* fieldOfView_;
     double* minRange_;
     double* maxRange_;
     double* range_;
     int* bufferCounter_;
     char* rangeSensorCode_;  // not stored in handle

     /* battery interface */
     BatteryInterface batteryInterface_;
     std::vector<BatteryHandle::Data> batteryData_;
     std::vector<std::string> batteryName_;
     double* voltage_;
     char* batteryCode_;  // not stored in handle

     /* encoder interface */
     hardware_interface::JointStateInterface jointStateInterface_;
     std::vector<std::string> jointNames_;
     double position_[2];
     double velocity_[2];
     double effort_[2];
     double encoder_offset_;
  };
}  // namespace arm
}  // namespace pandora_hardware_interface
#endif  // ARM_HARDWARE_INTERFACE_ARM_HARDWARE_INTERFACE_H
