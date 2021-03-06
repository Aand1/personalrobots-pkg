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

#ifndef ETHERCAT_DEVICE_H
#define ETHERCAT_DEVICE_H

#include <vector>

#include <tinyxml/tinyxml.h>

#include <ethercat/ethercat_defs.h>
#include <al/ethercat_slave_handler.h>

#include <pr2_hardware_interface/hardware_interface.h>

#include <diagnostic_updater/DiagnosticStatusWrapper.h>

#include <diagnostic_msgs/DiagnosticArray.h>

#include <ethercat_hardware/ethercat_com.h>

#include <loki/Factory.h>
#include <loki/Sequence.h>

using namespace std;
using namespace pr2_mechanism;


struct et1x00_error_counters
{
  struct {
    uint8_t invalid_frame;
    uint8_t rx_error;
  } __attribute__((__packed__)) port[4];
  uint8_t forwarded_rx_error[4];
  uint8_t epu_error;
  uint8_t pdi_error;
  uint8_t res[2];
  uint8_t lost_link[4];
  static const EC_UINT BASE_ADDR=0x300;
  bool isGreaterThan(unsigned value) const;
  bool isGreaterThan(const et1x00_error_counters &value) const;
  void zero();
} __attribute__((__packed__));


struct EthercatPortDiagnostics
{
  EthercatPortDiagnostics();
  void zeroTotals();
  bool physicalLink;
  bool loop;
  bool communication;
  uint64_t rxErrorTotal;
  uint64_t invalidFrameTotal; 
  uint64_t forwardedRxErrorTotal;
  uint64_t lostLinkTotal; 
};

struct EthercatDeviceDiagnostics
{
public:
  EthercatDeviceDiagnostics();

  // Collects diagnostic data from specific ethercat slave, and updates object state
  //
  // com  EtherCAT communication object is used send/recv packets to/from ethercat chain.
  // sh   slaveHandler of device to collect Diagnostics from 
  // prev previously collected diagnostics (can be pointer to this object)
  //
  // collectDiagnotics will send/recieve multiple packets, and may considerable amount of time complete.
  // 
  void collect(EthercatCom *com, EtherCAT_SlaveHandler *sh);

  // Puts reviously diagnostic collected diagnostic state to DiagnosticStatus object 
  // 
  // d         DiagnositcState to add diagnostics to.
  // numPorts  Number of ports device is supposed to have.  4 is max, 1 is min.
  void publish(diagnostic_updater::DiagnosticStatusWrapper &d, unsigned numPorts=4) const;

protected:
  void zeroTotals();
  void accumulate(const et1x00_error_counters &next, const et1x00_error_counters &prev);
  uint64_t pdiErrorTotal_;
  uint64_t epuErrorTotal_;
  EthercatPortDiagnostics portDiagnostics_[4];
  unsigned nodeAddress_;
  et1x00_error_counters errorCountersPrev_;
  bool errorCountersMayBeCleared_;

  bool diagnosticsValid_;
  bool resetDetected_;
};


class EthercatDevice
{
public:
  EthercatDevice(EtherCAT_SlaveHandler *sh, bool has_actuator = false, int command_size = 0, int status_size = 0);

  virtual ~EthercatDevice();

  virtual int initialize(Actuator *, bool allow_unprogrammed=0) = 0;

  virtual void convertCommand(ActuatorCommand &command, unsigned char *buffer) = 0;
  virtual void convertState(ActuatorState &state, unsigned char *current_buffer, unsigned char *last_buffer) = 0;

  virtual void computeCurrent(ActuatorCommand &command) = 0;
  virtual void truncateCurrent(ActuatorCommand &command) = 0;
  virtual bool verifyState(ActuatorState &state, unsigned char *this_buffer, unsigned char *prev_buffer) = 0;

  virtual void diagnostics(diagnostic_updater::DiagnosticStatusWrapper &d, unsigned char *);

  void ethercatDiagnostics(diagnostic_updater::DiagnosticStatusWrapper &d, unsigned numPorts);

  void collectDiagnostics(EthercatCom *com);

  enum AddrMode {FIXED_ADDR=0, POSITIONAL_ADDR=1};

  /*!
   * \brief Write data to device ESC.
   */
  static int writeData(EthercatCom *com, EtherCAT_SlaveHandler *sh,  EC_UINT address, void const* buffer, EC_UINT length, AddrMode addrMode);
  inline int writeData(EthercatCom *com, EC_UINT address, void const* buffer, EC_UINT length, AddrMode addrMode) {
    return writeData(com, sh_, address, buffer, length, addrMode);
  }
  
  /*!
   * \brief Read data from device ESC.
   */
  static int readData(EthercatCom *com, EtherCAT_SlaveHandler *sh,  EC_UINT address, void *buffer, EC_UINT length, AddrMode addrMode);
  inline int readData(EthercatCom *com, EC_UINT address, void *buffer, EC_UINT length, AddrMode addrMode) {
    return readData(com, sh_, address, buffer, length, addrMode);
  }  

  /*!
   * \brief Read then write data to ESC.
   */  
  static int readWriteData(EthercatCom *com, EtherCAT_SlaveHandler *sh,  EC_UINT address, void *buffer, EC_UINT length, AddrMode addrMode);
  inline int readWriteData(EthercatCom *com, EC_UINT address, void *buffer, EC_UINT length, AddrMode addrMode) {
    return readWriteData(com, sh_, address, buffer, length, addrMode);
  }

  EtherCAT_SlaveHandler *sh_;
  bool has_actuator_;
  unsigned int command_size_;
  unsigned int status_size_;
  
  // The device diagnostics are collected with a non-readtime thread that calls collectDiagnostics()
  // The device published from the realtime loop by indirectly invoking ethercatDiagnostics()
  // To avoid blocking of the realtime thread (for long) a double buffer is used the 
  // The publisher thread will lock newDiagnosticsIndex when publishing data.
  // The collection thread will lock deviceDiagnostics when updating deviceDiagnostics
  // The collection thread will also lock newDiagnosticsIndex at end of update, just before swapping buffers.
  unsigned newDiagnosticsIndex_;
  pthread_mutex_t newDiagnosticsIndexLock_;  
  EthercatDeviceDiagnostics deviceDiagnostics[2];
  pthread_mutex_t diagnosticsLock_;  
};


typedef Loki::SingletonHolder
<
  Loki::Factory< EthercatDevice, EC_UDINT, Loki::Seq<EtherCAT_SlaveHandler *, int&> >,
  Loki::CreateUsingNew,
  Loki::LongevityLifetime::DieAsSmallObjectChild
> DeviceFactory;

template< class T> T* deviceCreator(EtherCAT_SlaveHandler *sh, int &addr) {return new T(sh, addr);}

#endif /* ETHERCAT_DEVICE_H */
