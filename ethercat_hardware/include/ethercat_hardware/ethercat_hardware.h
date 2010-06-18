/*********************************************************************
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2008, Willow Garage, Inc.
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
 *   * Neither the name of the Willow Garage nor the names of its
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
 *********************************************************************/

#ifndef ETHERCAT_HARDWARE_H
#define ETHERCAT_HARDWARE_H

#include <pr2_hardware_interface/hardware_interface.h>

#include <al/ethercat_AL.h>
#include <al/ethercat_master.h>
#include <al/ethercat_slave_handler.h>

#include "ethercat_hardware/ethercat_device.h"
#include "ethercat_hardware/ethercat_com.h"

#include <realtime_tools/realtime_publisher.h>

#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/max.hpp>
#include <boost/accumulators/statistics/mean.hpp>

#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>

#include <pluginlib/class_loader.h>

#include <std_msgs/Bool.h>

using namespace boost::accumulators;

class EthercatHardware
{
public:
  /*!
   * \brief Constructor
   */
  EthercatHardware(const std::string& name);

  /*!
   * \brief Destructor
   */
  ~EthercatHardware();

  /*!
   * \brief Send most recent motor commands and retrieve updates. This command must be run at a sufficient rate or else the motors will be disabled.
   * \param reset A boolean indicating if the motor controller boards should be reset
   * \param halt A boolean indicating if the motors should be halted
   */
  void update(bool reset, bool halt);

  /*!
   * \brief Initialize the EtherCAT Master Library.
   * \param interface The socket interface that is connected to the EtherCAT devices (e.g., eth0)
   * \param allow_unprogrammed A boolean indicating if the driver should treat the discovery of unprogrammed boards as a fatal error.  Set to 'true' during board configuration, and set to 'false' otherwise.
   */
  void init(char *interface, bool allow_unprogrammed);

  /*!
   * \brief Publish diagnostic messages
   */
  void publishDiagnostics();

  /*!
   * \brief Collects diagnotics from all devices.
   */
  void collectDiagnostics();

  void printCounters(std::ostream &os=std::cout); 

  /*!
   * \brief Send process data
   */
  bool txandrx_PD(unsigned buffer_size, unsigned char* buffer, unsigned tries);

  pr2_hardware_interface::HardwareInterface *hw_;

private:
  void diagnosticsThreadFunc();
  static void changeState(EtherCAT_SlaveHandler *sh, EC_State new_state);

  struct netif *ni_;
  string interface_;

  EtherCAT_AL *al_;
  EtherCAT_Master *em_;

  EthercatDevice *configSlave(EtherCAT_SlaveHandler *sh);
  EthercatDevice **slaves_;
  unsigned int num_slaves_;

  unsigned char *this_buffer_;
  unsigned char *prev_buffer_;
  unsigned char *buffers_;
  unsigned char *diagnostics_buffer_;
  unsigned int buffer_size_;

  bool halt_motors_;
  unsigned int reset_state_;

  realtime_tools::RealtimePublisher<std_msgs::Bool> motor_publisher_;
  realtime_tools::RealtimePublisher<diagnostic_msgs::DiagnosticArray> publisher_;
  struct EthercatHardwareDiagnostics {
    EthercatHardwareDiagnostics() : acc_() { }
    accumulator_set<double, stats<tag::max, tag::mean> > acc_;
    double max_roundtrip_;
    int txandrx_errors_;
    unsigned device_count_;
    bool pd_error_;
  } diagnostics_;
  boost::condition_variable diagnostics_cond_;
  boost::mutex diagnostics_mutex_;
  boost::thread diagnostics_thread_;
  bool diagnostics_ready_;

  ros::Time last_published_;
  ros::Time motor_last_published_;
  
  vector<diagnostic_msgs::DiagnosticStatus> statuses_;
  vector<diagnostic_msgs::KeyValue> values_;
  diagnostic_updater::DiagnosticStatusWrapper status_;


  EthercatOobCom *oob_com_;  

  pluginlib::ClassLoader<EthercatDevice> device_loader_;
};

#endif /* ETHERCAT_HARDWARE_H */
