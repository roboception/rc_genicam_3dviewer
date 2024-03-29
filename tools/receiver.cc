/*
 * This file is part of the rc_genicam_3dviewer package.
 *
 * Copyright (c) 2018 Roboception GmbH
 * All rights reserved
 *
 * Author: Heiko Hirschmueller
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "receiver.h"
#include "selectionwindow.h"

#include <rc_genicam_api/system.h>
#include <rc_genicam_api/interface.h>
#include <rc_genicam_api/stream.h>
#include <rc_genicam_api/buffer.h>
#include <rc_genicam_api/image.h>
#include <rc_genicam_api/imagelist.h>
#include <rc_genicam_api/config.h>

#include <rc_genicam_api/pixel_formats.h>

#include <Base/GCException.h>

#include <gutil/proctime.h>
#include <gutil/exception.h>

#include <vector>
#include <sstream>

namespace rcgv
{

namespace
{

std::vector<std::shared_ptr<rcg::Device> > getDevices(std::vector<std::string> &label)
{
  std::vector<std::shared_ptr<rcg::Device> > ret;

  std::vector<std::shared_ptr<rcg::System> > system=rcg::System::getSystems();

  for (size_t i=0; i<system.size(); i++)
  {
    system[i]->open();

    std::vector<std::shared_ptr<rcg::Interface> > interf=system[i]->getInterfaces();

    for (size_t k=0; k<interf.size(); k++)
    {
      interf[k]->open();

      std::vector<std::shared_ptr<rcg::Device> > device=interf[k]->getDevices();

      for (size_t j=0; j<device.size(); j++)
      {
        // create label

        std::ostringstream out;

        std::string s=device[j]->getDisplayName();

        out << s;
        for (size_t l=s.size(); l<=16; l++)
        {
          out << ' ';
        }

        s=device[j]->getParent()->getID();
        if (s.size() > 20)
        {
          s=s.substr(0, 20);
        }

        out << s << ":";

        s=device[j]->getSerialNumber();
        out << s;

        // insert sorted by serial number

        size_t l=0;
        while (l < ret.size() && s.compare(ret[l]->getSerialNumber()) >= 0)
        {
          l++;
        }

        label.insert(label.begin()+l, out.str());
        ret.insert(ret.begin()+l, device[j]);
      }

      interf[k]->close();
    }

    system[i]->close();
  }

  return ret;
}

}

Receiver::Receiver(std::shared_ptr<Modeler> _modeler, const char *device,
  double _timeout, const std::vector<std::string> &genicam_param)
{
  sem_nodemap.increment();

  modeler=_modeler;

  timeout=_timeout;

  // find specific device accross all systems and interfaces and open it

  if (device)
  {
    dev=rcg::getDevice(device);

    if (!dev)
    {
      throw gutil::IOException(std::string("Device not found: ")+device);
    }
  }
  else
  {
    std::vector<std::string> label;
    std::vector<std::shared_ptr<rcg::Device> > list=getDevices(label);

    if (list.size() == 1)
    {
      dev=list[0];
    }
    else if (list.size() == 0)
    {
      throw gutil::IOException(std::string("No device found"));
    }
    else
    {
      SelectionWindow sel(label);

      int i=sel.getSelection();

      if (i >= 0)
      {
        dev=list[i];
      }
      else
      {
        throw gutil::IOException(std::string("No device selected"));
      }
    }
  }

  dev->open(rcg::Device::CONTROL);
  nodemap=dev->getRemoteNodeMap();

  rcg::setBoolean(nodemap, "ChunkModeActive", true);

  // apply all given genicam parameters

  for (size_t i=0; i<genicam_param.size(); i++)
  {
    // split argument in key and value

    std::string key=genicam_param[i];
    std::string value;

    size_t k=key.find('=');
    if (k != std::string::npos)
    {
      value=key.substr(k+1);
      key=key.substr(0, k);
    }

    if (value.size() > 0)
    {
      rcg::setString(nodemap, key.c_str(), value.c_str(), true);
    }
    else
    {
      rcg::callCommand(nodemap, key.c_str(), true);
    }
  }

  // get focal length, baseline and disparity scale factor

  f=rcg::getFloat(nodemap, "FocalLengthFactor", 0, 0, false);
  t=rcg::getFloat(nodemap, "Baseline", 0, 0, true);
  scale=rcg::getFloat(nodemap, "Scan3dCoordinateScale", 0, 0, true);
  offset=rcg::getFloat(nodemap, "Scan3dCoordinateOffset", 0, 0, true);

  // check for special exposure alternate mode of rc_visard and
  // corresponding filter and set tolerance accordingly

  // (The exposure alternate mode is typically used with a random dot
  // projector connected to Out1. Alternate means that Out1 is high for
  // every second image. The rc_visard calculates disparities from images
  // with Out1=High. However, if the alternate filter is set to OnlyLow,
  // then it is gueranteed that Out1=Low (i.e. projector off) for all
  // rectified images. Thus, rectified images and disparity images are
  // always around 40 ms appart, which must be taken into account for
  // synchronization.)

  tol=0;

  try
  {
    rcg::setEnum(nodemap, "LineSelector", "Out1", true);
    std::string linesource=rcg::getEnum(nodemap, "LineSource", true);

    if (linesource == "ExposureAlternateActive")
    {
      tol=250*1000*1000; // set tolerance to 250 ms
    }
  }
  catch (const std::exception &)
  {
    // ignore possible errors
  }

  // sanity check of some parameters

  rcg::checkFeature(nodemap, "Scan3dOutputMode", "DisparityC");
  rcg::checkFeature(nodemap, "Scan3dInvalidDataFlag", "1");
  inv=rcg::getFloat(nodemap, "Scan3dInvalidDataValue", 0, 0, true);

  // set to color format if available

  rcg::setEnum(nodemap, "PixelFormat", "YCbCr411_8", false);
  rcg::setEnum(nodemap, "PixelFormat", "RGB8", false);

  // enable left image and disparity disparity

  {
    std::vector<std::string> component;

    rcg::getEnum(nodemap, "ComponentSelector", component, true);

    for (size_t k=0; k<component.size(); k++)
    {
      rcg::setEnum(nodemap, "ComponentSelector", component[k].c_str(), true);

      bool enable=(component[k] == "Intensity" || component[k] == "Disparity");
      rcg::setBoolean(nodemap, "ComponentEnable", enable, true);
    }
  }

  // only get images without projection in exposure alternate mode (this does
  // not have any effect in other out1_modes)

  rcg::setString(nodemap, "AcquisitionAlternateFilter", "OnlyLow");

  // try getting synchronized data (which only has an effect if the device
  // and GenTL producer support multipart)

  rcg::setString(nodemap, "AcquisitionMultiPartMode", "SingleComponent");

  // set depth acquisition mode to continuous

  rcg::setString(nodemap, "DepthAcquisitionMode", "Continuous");

  // start background thread for streaming images

  running=true;
  thread.create(*this);
}

Receiver::~Receiver()
{
  close();
}

void Receiver::close()
{
  if (running)
  {
    // stop grabbing thread

    running=false;
    thread.join();
  }

  // closing the communication to the device

  if (dev)
  {
    dev->close();
  }

  modeler.reset();
  dev.reset();
  nodemap.reset();

  // clear systems

  rcg::System::clearSystems();
}

bool Receiver::getWritable(bool &writable, const char *name)
{
  gutil::Lock lock(sem_nodemap);
  bool ret=false;

  if (nodemap)
  {
    try
    {
      GenApi::INode *node=nodemap->_GetNode(name);

      if (node != 0)
      {
        if (GenApi::IsReadable(node))
        {
          writable=GenApi::IsWritable(node);
          ret=true;
        }
      }
    }
    catch (const GENICAM_NAMESPACE::GenericException &)
    { }
  }

  return ret;
}

bool Receiver::getBoolean(const char *name)
{
  gutil::Lock lock(sem_nodemap);

  if (nodemap)
  {
    return rcg::getBoolean(nodemap, name);
  }

  return false;
}

void Receiver::setBoolean(const char *name, bool value)
{
  gutil::Lock lock(sem_nodemap);

  if (nodemap)
  {
    rcg::setBoolean(nodemap, name, value);
  }
}

std::string Receiver::getEnum(const char *name, std::vector<std::string> &list)
{
  gutil::Lock lock(sem_nodemap);

  if (nodemap)
  {
    return rcg::getEnum(nodemap, name, list);
  }

  return std::string();
}

void Receiver::setEnum(const char *name, const std::string &value)
{
  gutil::Lock lock(sem_nodemap);

  if (nodemap)
  {
    rcg::setEnum(nodemap, name, value.c_str());

    // switch timestamp tolerance if switching between exposure alternate and
    // other modes

    if (std::string(name) == "LineSource")
    {
      if (value == "ExposureAlternateActive")
      {
        tol=250*1000*1000; // set maximum tolerance to 250 ms
      }
      else
      {
        tol=0;
      }
    }
  }
}

void Receiver::run()
{
  try
  {
    // open image stream

    std::vector<std::shared_ptr<rcg::Stream> > stream=dev->getStreams();

    if (stream.size() > 0)
    {
      // opening first stream

      stream[0]->open();
      stream[0]->attachBuffers(true);
      stream[0]->startStreaming();

      // prepare buffers for time synchronization of images

      rcg::ImageList left_list(100);
      rcg::ImageList disp_list(25);

      double last_grabbed=gutil::ProcTime::monotonic();
      double last_heartbeat=last_grabbed;
      double heartbeat_timeout=rcg::getInteger(nodemap, "GevHeartbeatTimeout", 0, 0, false)/2000.0;

      while (running && (timeout == 0 || last_grabbed+timeout > gutil::ProcTime::monotonic()))
      {
        // grab next image with timeout

        const rcg::Buffer *buffer=stream[0]->grab(500);
        if (buffer != 0)
        {
          // ensure heartbeat for GEV devices

          if (heartbeat_timeout > 0 && last_heartbeat+heartbeat_timeout < gutil::ProcTime::monotonic())
          {
            gutil::Lock lock(sem_nodemap);
            rcg::setEnum(nodemap, "LineSelector", "Out1", true);
            rcg::getString(nodemap, "LineSource", true, true);

            last_heartbeat=gutil::ProcTime::monotonic();
          }

          // check for a complete image in the buffer

          if (!buffer->getIsIncomplete())
          {
            gutil::Lock lock(sem_nodemap);

            // go through all parts in case of multi-part buffer

            size_t partn=buffer->getNumberOfParts();
            for (uint32_t part=0; part<partn; part++)
            {
              if (buffer->getImagePresent(part))
              {
                // get current out1 mode from chunk data (allowed to fail to support
                // rc_visard / rc_cube < 22.07.0)

                uint64_t ltol=tol;

                rcg::setEnum(nodemap, "ChunkLineSelector", "Out1", false);
                std::string out1_mode=rcg::getEnum(nodemap, "ChunkLineSource", false);

                if (out1_mode.size() > 0)
                {
                  if (out1_mode == "ExposureAlternateActive")
                  {
                    ltol=250*1000*1000; // set maximum tolerance to 250 ms
                  }
                  else
                  {
                    ltol=0;
                  }
                }

                // store image in the corresponding list

                uint64_t left_tol=0;
                uint64_t disp_tol=0;

                std::string component=rcg::getComponetOfPart(nodemap, buffer, part);

                if (component == "Intensity")
                {
                  left_list.add(buffer, part);
                  disp_tol=ltol;
                }
                else if (component == "Disparity")
                {
                  disp_list.add(buffer, part);
                  left_tol=ltol;
                }

                // get corresponding left and disparity images

                uint64_t timestamp=buffer->getTimestampNS();
                std::shared_ptr<const rcg::Image> left=left_list.find(timestamp, left_tol);
                std::shared_ptr<const rcg::Image> disp=disp_list.find(timestamp, disp_tol);

                if (left && disp)
                {
                  // hand the data over to the modeler

                  modeler->process(f, t, inv, scale, offset, left, disp);

                  // remove all images from the buffer with the current or an
                  // older time stamp

                  left_list.removeOld(timestamp);
                  disp_list.removeOld(timestamp);

                  last_grabbed=gutil::ProcTime::monotonic();
                }
              }
            }
          }
          else
          {
            std::cerr << "Incomplete buffer received!" << std::endl;
          }
        }
        else
        {
          gutil::Lock lock(sem_nodemap);

          // check if connection is still there

          rcg::setEnum(nodemap, "LineSelector", "Out1", true);
          rcg::getString(nodemap, "LineSource", true, true);
        }
      }

      // report if synchronization failed

      if (last_grabbed+timeout < gutil::ProcTime::monotonic())
      {
        std::cerr << "Cannot grab synchronized left and disparity image" << std::endl;
      }

      // stopping and closing image stream

      stream[0]->stopStreaming();
      stream[0]->close();
    }
    else
    {
      std::cerr << "No streams available" << std::endl;
    }
  }
  catch (const std::exception &ex)
  {
    std::cerr << ex.what() << std::endl;
  }
  catch (const GENICAM_NAMESPACE::GenericException &ex)
  {
    std::cerr << "Exception: " << ex.what() << std::endl;
  }
  catch (...)
  {
    std::cerr << "Unknown exception!" << std::endl;
  }

  running=false;
}

}
