/**
Software License Agreement (BSD)

\file      camera.cpp
\authors   Michael Hosmar <mhosmar@clearpathrobotics.com>
\copyright Copyright (c) 2018, Clearpath Robotics, Inc., All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that
the following conditions are met:
 * Redistributions of source code must retain the above copyright notice, this list of conditions and the
   following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the
   following disclaimer in the documentation and/or other materials provided with the distribution.
 * Neither the name of Clearpath Robotics nor the names of its contributors may be used to endorse or promote
   products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WAR-
RANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, IN-
DIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include "spinnaker_camera_driver/camera.h"

#include <string>

namespace spinnaker_camera_driver
{
void Camera::init()
{
  Spinnaker::GenApi::CIntegerPtr height_max_ptr = node_map_->GetNode("HeightMax");
  if (!IsAvailable(height_max_ptr) || !IsReadable(height_max_ptr))
  {
    throw std::runtime_error("[Camera::init] Unable to read HeightMax");
  }
  height_max_ = height_max_ptr->GetValue();
  roi_height_ = height_max_;
  roi_y_offset_ = 0;
  Spinnaker::GenApi::CIntegerPtr width_max_ptr = node_map_->GetNode("WidthMax");
  if (!IsAvailable(width_max_ptr) || !IsReadable(width_max_ptr))
  {
    throw std::runtime_error("[Camera::init] Unable to read WidthMax");
  }
  width_max_ = width_max_ptr->GetValue();
  roi_width_ = width_max_;
  roi_x_offset_ = 0;
  // Set Throughput to maximum
  //=====================================
  setMaxInt(node_map_, "DeviceLinkThroughputLimit");
}
void Camera::setFrameRate(const float frame_rate)
{
  // This enables the "AcquisitionFrameRateEnabled"
  //======================================
  setProperty(node_map_, "AcquisitionFrameRateEnable", true);

  // This sets the "AcquisitionFrameRate" to X FPS
  // ========================================

  Spinnaker::GenApi::CFloatPtr ptrAcquisitionFrameRate = node_map_->GetNode("AcquisitionFrameRate");
  ROS_DEBUG_STREAM("Minimum Frame Rate: \t " << ptrAcquisitionFrameRate->GetMin());
  ROS_DEBUG_STREAM("Maximum Frame rate: \t " << ptrAcquisitionFrameRate->GetMax());

  // Finally Set the Frame Rate
  setProperty(node_map_, "AcquisitionFrameRate", frame_rate);

  ROS_DEBUG_STREAM("Current Frame rate: \t " << ptrAcquisitionFrameRate->GetValue());
}

void Camera::setNewConfiguration(const SpinnakerConfig& config, const uint32_t& level)
{
  try
  {
    if (level >= LEVEL_RECONFIGURE_STOP)
      setImageControlFormats(config);

    setFrameRate(static_cast<float>(config.acquisition_frame_rate));
    // Set enable after frame rate encase its false
    setProperty(node_map_, "AcquisitionFrameRateEnable", config.acquisition_frame_rate_enable);

    // Set Trigger and Strobe Settings
    // NOTE: The trigger must be disabled (i.e. TriggerMode = "Off") in order to configure whether the source is
    // software or hardware.
    setProperty(node_map_, "TriggerMode", std::string("Off"));
    setProperty(node_map_, "TriggerSource", config.trigger_source);
    setProperty(node_map_, "TriggerSelector", config.trigger_selector);
    setProperty(node_map_, "TriggerActivation", config.trigger_activation_mode);
    setProperty(node_map_, "TriggerMode", config.enable_trigger);

    setProperty(node_map_, "LineSelector", config.line_selector);
    setProperty(node_map_, "LineMode", config.line_mode);
    setProperty(node_map_, "LineSource", config.line_source);

    // Set auto exposure
    setProperty(node_map_, "ExposureMode", config.exposure_mode);
    setProperty(node_map_, "ExposureAuto", config.exposure_auto);

    // Set sharpness
    if (IsAvailable(node_map_->GetNode("SharpeningEnable")))
    {
      setProperty(node_map_, "SharpeningEnable", config.sharpening_enable);
      if (config.sharpening_enable)
      {
        setProperty(node_map_, "SharpeningAuto", config.auto_sharpness);
        setProperty(node_map_, "Sharpening", static_cast<float>(config.sharpness));
        setProperty(node_map_, "SharpeningThreshold", static_cast<float>(config.sharpening_threshold));
      }
    }

    // Set saturation
    if (IsAvailable(node_map_->GetNode("SaturationEnable")))
    {
      setProperty(node_map_, "SaturationEnable", config.saturation_enable);
      if (config.saturation_enable)
      {
        setProperty(node_map_, "Saturation", static_cast<float>(config.saturation));
      }
    }

    // Set shutter time/speed
    if (config.exposure_auto.compare(std::string("Off")) == 0)
    {
      setProperty(node_map_, "ExposureTime", static_cast<float>(config.exposure_time));
    }
    else
    {
      setProperty(node_map_, "AutoExposureExposureTimeUpperLimit",
                  static_cast<float>(config.auto_exposure_time_upper_limit));
    }

    // Set gain
    setProperty(node_map_, "GainSelector", config.gain_selector);
    setProperty(node_map_, "GainAuto", config.auto_gain);
    if (config.auto_gain.compare(std::string("Off")) == 0)
    {
      setProperty(node_map_, "Gain", static_cast<float>(config.gain));
    }

    // Set brightness
    setProperty(node_map_, "BlackLevel", static_cast<float>(config.brightness));

    // Set gamma
    if (config.gamma_enable)
    {
      setProperty(node_map_, "GammaEnable", config.gamma_enable);
      setProperty(node_map_, "Gamma", static_cast<float>(config.gamma));
    }

    // Set white balance
    if (IsAvailable(node_map_->GetNode("BalanceWhiteAuto")))
    {
      setProperty(node_map_, "BalanceWhiteAuto", config.auto_white_balance);
      if (config.auto_white_balance.compare(std::string("Off")) == 0)
      {
        setProperty(node_map_, "BalanceRatioSelector", "Blue");
        setProperty(node_map_, "BalanceRatio", static_cast<float>(config.white_balance_blue_ratio));
        setProperty(node_map_, "BalanceRatioSelector", "Red");
        setProperty(node_map_, "BalanceRatio", static_cast<float>(config.white_balance_red_ratio));
      }
    }

    setProperty(node_map_, "ReverseX", config.reverse_x);
    setProperty(node_map_, "ReverseY", config.reverse_y);
  }
  catch (const Spinnaker::Exception& e)
  {
    throw std::runtime_error("[Camera::setNewConfiguration] Failed to set configuration: " + std::string(e.what()));
  }
}

// Image Size and Pixel Format
void Camera::setImageControlFormats(const spinnaker_camera_driver::SpinnakerConfig& config)
{
  // Set Binning and Decimation
  setProperty(node_map_, "BinningHorizontal", config.image_format_x_binning);
  setProperty(node_map_, "BinningVertical", config.image_format_y_binning);
  setProperty(node_map_, "DecimationHorizontal", config.image_format_x_decimation);
  setProperty(node_map_, "DecimationVertical", config.image_format_y_decimation);

  // Grab the Max values after decimation
  Spinnaker::GenApi::CIntegerPtr height_max_ptr = node_map_->GetNode("HeightMax");
  if (!IsAvailable(height_max_ptr) || !IsReadable(height_max_ptr))
  {
    throw std::runtime_error("[Camera::setImageControlFormats] Unable to read HeightMax");
  }
  height_max_ = height_max_ptr->GetValue();
  Spinnaker::GenApi::CIntegerPtr width_max_ptr = node_map_->GetNode("WidthMax");
  if (!IsAvailable(width_max_ptr) || !IsReadable(width_max_ptr))
  {
    throw std::runtime_error("[Camera::setImageControlFormats] Unable to read WidthMax");
  }
  width_max_ = width_max_ptr->GetValue();

  // Offset first encase expanding ROI
  // Apply offset X
  setProperty(node_map_, "OffsetX", 0);
  // Apply offset Y
  setProperty(node_map_, "OffsetY", 0);

  setROI(config.image_format_x_offset, config.image_format_y_offset,
         config.image_format_roi_width, config.image_format_roi_height);

  // Set Pixel Format
  setProperty(node_map_, "PixelFormat", config.image_format_color_coding);
}

void Camera::setROI(const int x_offset, const int y_offset, const int roi_width, const int roi_height)
{
  // Set Width/Height
  if (roi_width != roi_width_)
  {
    if (roi_width <= 0 || roi_width > width_max_)
    {
      setProperty(node_map_, "Width", width_max_);
      roi_width_ = width_max_;
    }
    else
    {
      setProperty(node_map_, "Width", roi_width);
      roi_width_ = roi_width;
    }
  }
  if (roi_height != roi_height_)
  {
    if (roi_height <= 0 || roi_height > height_max_)
    {
      setProperty(node_map_, "Height", height_max_);
      roi_height_ = height_max_;
    }
    else
    {
      setProperty(node_map_, "Height", roi_height);
      roi_height_ = roi_height;
    }
  }

  // Apply offset X
  setProperty(node_map_, "OffsetX", x_offset);
  roi_x_offset_ = x_offset;
  // Apply offset Y
  setProperty(node_map_, "OffsetY", y_offset);
  roi_y_offset_ = y_offset;
}

void Camera::setGain(const float& gain)
{
  setProperty(node_map_, "GainAuto", "Off");
  setProperty(node_map_, "Gain", static_cast<float>(gain));
}

/*
void Camera::setGigEParameters(bool auto_packet_size, unsigned int packet_size, unsigned int packet_delay)
{
}

void Camera::setupGigEPacketSize(PGRGuid & guid)
{
}

void Camera::setupGigEPacketSize(PGRGuid & guid, unsigned int packet_size)
{

}

void Camera::setupGigEPacketDelay(PGRGuid & guid, unsigned int packet_delay)
{
}

*/

int Camera::getHeightMax() const
{
  return height_max_;
}

int Camera::getWidthMax() const
{
  return width_max_;
}

int Camera::getROIXOffset() const
{
  return roi_x_offset_;
}

int Camera::getROIYOffset() const
{
  return roi_y_offset_;
}

int Camera::getROIWidth() const
{
  return roi_width_;
}

int Camera::getROIHeight() const
{
  return roi_height_;
}

// uint SpinnakerCamera::getGain()
// {
//   return metadata_.embeddedGain >> 20;
// }

// uint Camera::getShutter()
// {
//   return metadata_.embeddedShutter >> 20;
// }

// uint Camera::getBrightness()
// {
//   return metadata_.embeddedTimeStamp >> 20;
// }

// uint Camera::getExposure()
// {
//   return metadata_.embeddedBrightness >> 20;
// }

// uint Camera::getWhiteBalance()
// {
//   return metadata_.embeddedExposure >> 8;
// }

// uint Camera::getROIPosition()
// {
//   return metadata_.embeddedROIPosition >> 24;
// }

// float Camera::getCameraTemperature()
//{
//}

// float Camera::getCameraFrameRate()
//{
//}
Spinnaker::GenApi::CNodePtr Camera::readProperty(const Spinnaker::GenICam::gcstring property_name)
{
  Spinnaker::GenApi::CNodePtr ptr = node_map_->GetNode(property_name);
  if (!Spinnaker::GenApi::IsAvailable(ptr) || !Spinnaker::GenApi::IsReadable(ptr))
  {
    throw std::runtime_error("Unable to get parmeter " + property_name);
  }
  return ptr;
}

Camera::Camera(Spinnaker::GenApi::INodeMap* node_map)
{
  node_map_ = node_map;
  init();
}
}  // namespace spinnaker_camera_driver
