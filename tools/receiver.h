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

#ifndef RC_GENICAM_VIEWER_RECEIVER
#define RC_GENICAM_VIEWER_RECEIVER

#include "modeler.h"

#include <rc_genicam_api/device.h>
#include <rc_genicam_api/config.h>

#include <gutil/thread.h>
#include <gutil/semaphore.h>
#include <atomic>
#include <memory>

namespace rcgv
{

/**
  Receiver object that grabs synchronized intensity and disparity images from
  the device in a background thread and hands them over to the modeler.
*/

class Receiver: public gutil::ThreadFunction
{
  public:

    Receiver(std::shared_ptr<Modeler> modeler, const char *device,
      double timeout, const std::vector<std::string> &genicam_param);
    ~Receiver();

    /**
      Returns true if the background thread is running.
    */

    bool isRunning() { return running; }

    /**
      Close device and free all resources.
    */

    void close();

    /**
      Tests if parameter is writable.

      @param writable Return value that is set to true if parameter is writable
                      and false if it is only readable.
      @param name     Name of parameter.
      @return         True if parameter exists and is readable.
    */

    bool getWritable(bool &writable, const char *name);

    /**
      Get value of boolean GenICam parameter.

      @param name Name of parameter.
      @return     Value of parameter.
    */

    bool getBoolean(const char *name);

    /**
      Set value of boolean GenICam parameter.

      @param name  Name of parameter.
      @param value Value of parameter.
    */

    void setBoolean(const char *name, bool value);

    /**
      Get value of enum GenICam parameter.

      @param name Name of parameter.
      @param list List of all enums.
      @return     Current value of parameter.
    */

    std::string getEnum(const char *name, std::vector<std::string> &list);

    /**
      Set value of enum GenICam parameter.

      @param name  Name of parameter.
      @param value Current value of parameter.
    */

    void setEnum(const char *name, const std::string &value);

  private:

    void run();

    std::shared_ptr<Modeler> modeler;

    std::shared_ptr<rcg::Device> dev;
    std::shared_ptr<GenApi::CNodeMapRef> nodemap;

    double timeout;
    double f, t, scale, offset;
    double inv;
    uint64_t tol;

    gutil::Thread thread;
    std::atomic_bool running;

    gutil::Semaphore sem_nodemap;
};

}

#endif
