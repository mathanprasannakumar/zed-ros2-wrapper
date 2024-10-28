// Copyright 2024 Stereolabs
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef ZED_CAMERA_ONE_COMPONENT_HPP_
#define ZED_CAMERA_ONE_COMPONENT_HPP_

#include <atomic>
#include <sl/CameraOne.hpp>

#include "sl_tools.hpp"
#include "sl_types.hpp"
#include "visibility_control.hpp"

namespace stereolabs
{

class ZedCameraOne : public rclcpp::Node
{
public:
  ZED_COMPONENTS_PUBLIC
  explicit ZedCameraOne(const rclcpp::NodeOptions & options);

  virtual ~ZedCameraOne();

protected:
  // ----> Initialization functions
  void init();
  void initParameters();
  void initServices();
  void initThreadsAndTimers();
  void initTFCoordFrameNames();
  void initPublishers();

  void getDebugParams();

  bool startCamera();
  void startTempPubTimer();
  // <---- Initialization functions

  // ----> Utility functions
  template<typename T>
  void getParam(
    std::string paramName, T defValue, T & outVal,
    std::string log_info = std::string(), bool dynamic = false);
  void fillCamInfo(
    const std::shared_ptr<sensor_msgs::msg::CameraInfo> & camInfoMsg,
    const std::string & frameId,
    bool rawParam = false);
  // <---- Utility functions

  // ----> Callbacks
  rcl_interfaces::msg::SetParametersResult callback_paramChange(
    std::vector<rclcpp::Parameter> parameters);
  void callback_updateDiagnostic(
    diagnostic_updater::DiagnosticStatusWrapper & stat);
  void callback_pubTemp();
  // <---- Callbacks

private:
  // ----> ZED SDK
  std::shared_ptr<sl::CameraOne> _zed;
  sl::InitParametersOne _initParams;
  // <---- ZED SDK

  // ----> Threads and Timers
  std::thread _grabThread;        // Main grab thread
  std::thread _videoThread;       // RGB data publish thread
  std::thread _sensThread;        // Sensors data publish thread

  std::atomic<bool> _threadStop;
  rclcpp::TimerBase::SharedPtr _initTimer;
  rclcpp::TimerBase::SharedPtr _tempPubTimer;    // Timer to retrieve and publish camera temperature
  // <---- Threads and Timers

  // ----> Debug variables
  bool _debugCommon = false;
  bool _debugVideoDepth = false;
  bool _debugSensors = false;
  bool _debugCamCtrl = false;
  bool _debugStreaming = false;
  // <---- Debug variables

  // ----> QoS
  // https://github.com/ros2/ros2/wiki/About-Quality-of-Service-Settings
  rclcpp::QoS _qos;
  rclcpp::PublisherOptions _pubOpt;
  rclcpp::SubscriptionOptions _subOpt;
  // <---- QoS

  // ----> Topics
  std::string _topicRoot = "~/";
  std::string _imgTopic;
  std::string _imgRawTopic;
  std::string _imgGrayTopic;
  std::string _imgRawGrayTopic;

  std::string _tempTopic;
  // <---- Topics

  // ----> Publishers
  image_transport::CameraPublisher _pubColorImg;
  image_transport::CameraPublisher _pubColorRawImg;
  image_transport::CameraPublisher _pubGrayImg;
  image_transport::CameraPublisher _pubGrayRawImg;

  imuPub _pubImu;
  imuPub _pubImuRaw;
  tempPub _pubTemp;
  // <---- Publishers

  // ----> Parameters
  std::string _cameraName = "zed_one";  // Name of the camera
  int _camGrabFrameRate = 30; // Grab frame rate
  sl::RESOLUTION _camResol = sl::RESOLUTION::HD1080; // Default resolution: RESOLUTION_HD1080
  PubRes _pubResolution = PubRes::NATIVE; // Use native grab resolution by default
  double _customDownscaleFactor = 1.0;  // Used to rescale data with user factor
  bool _cameraFlip = false; // Camera flipped?
  bool _enableHDR = false; // Enable HDR if supported?
  float _openTimeout_sec = 5; // Camera open timeout
  std::string _opencvCalibFile; // Custom OpenCV calibration file
  int _sdkVerbose = 0; // SDK verbose level
  int _gpuId = -1; // GPU ID

  int _camSerialNumber = 0; // Camera serial number

  sl::MODEL _camUserModel = sl::MODEL::ZED_XONE_GS;  // Default camera model

  std::string _svoFilepath = ""; // SVO input
  bool _svoRealtime = true; // SVO playback with real time

  std::string _streamAddr = ""; // Address for local streaming input
  int _streamPort = 10000;
  // <---- Parameters

  // ----> Dynamic params
  OnSetParametersCallbackHandle::SharedPtr _paramChangeCallbackHandle;
  // <---- Dynamic params

  // ----> Diagnostic
  diagnostic_updater::Updater _diagUpdater;
  // <---- Diagnostic

  // ----> Running status
  bool _debugMode = false;  // Debug mode active?
  bool _svoMode = false;        // Input from SVO?
  bool _streamMode = false;     // Expecting local streaming data?
  sl::ERROR_CODE _connStatus = sl::ERROR_CODE::LAST; // Connection status
  sl::ERROR_CODE _grabStatus = sl::ERROR_CODE::LAST; // Grab status
  float _tempImu = NOT_VALID_TEMP;
  // <---- Running status

  // ----> Timestamps
  rclcpp::Time _frameTimestamp;
  // <---- Timestamps

  // Camera IMU transform
  sl::Transform _slCamImuTransf;

  // ----> Camera info
  sl::MODEL _camRealModel;                   // Camera model requested to SDK
  unsigned int _camFwVersion;                // Camera FW version
  unsigned int _sensFwVersion;               // Sensors FW version
  // <---- Camera info

  // ----> Stereolabs Mat Info
  int _camWidth;   // Camera frame width
  int _camHeight;  // Camera frame height
  sl::Resolution _matResol;
  // <---- Stereolabs Mat Info

  // ----> Camera infos
  camInfoMsgPtr _camInfoMsg;
  camInfoMsgPtr _camInfoRawMsg;
  // <---- Camera infos

  // ----> Frame IDs
  std::string _cameraLinkFrameId;
  std::string _cameraCenterFrameId;
  std::string _camImgFrameId;
  std::string _camOptFrameId;
  std::string _imuFrameId;
  // <---- Frame IDs

};

// ----> Template Function definitions
template<typename T>
void ZedCameraOne::getParam(
  std::string paramName, T defValue, T & outVal,
  std::string log_info, bool dynamic)
{
  rcl_interfaces::msg::ParameterDescriptor descriptor;
  descriptor.read_only = !dynamic;

  declare_parameter(paramName, rclcpp::ParameterValue(defValue), descriptor);

  if (!get_parameter(paramName, outVal)) {
    RCLCPP_WARN_STREAM(
      get_logger(),
      "The parameter '"
        << paramName
        << "' is not available or is not valid, using the default value: "
        << defValue);
  }

  if (!log_info.empty()) {
    RCLCPP_INFO_STREAM(get_logger(), log_info << outVal);
  }
}

}  // namespace stereolabs

#endif  // ZED_CAMERA_ONE_COMPONENT_HPP_