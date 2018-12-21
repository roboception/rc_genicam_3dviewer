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

#ifndef RC_GENICAM_VIEWER_MODELER
#define RC_GENICAM_VIEWER_MODELER

#include <gimage/image.h>
#include <rc_genicam_api/image.h>

#include <gvr/model.h>
#include <gutil/thread.h>
#include <gutil/msgqueue.h>
#include <gutil/semaphore.h>

#include <atomic>
#include <memory>

namespace rcgv
{

/**
  Modeler object gets synchronized intensity and disparity images and creates
  a colored mesh object in a background thread.
*/

class Modeler: public gutil::ThreadFunction
{
  public:

    Modeler();
    ~Modeler();

    /**
      Provides synchronized data for creating the next model. This call may
      override a previously given model if it was not processed fast enough.
    */

    void process(double f, double t, double inv, double scale, double offset,
                 std::shared_ptr<const rcg::Image> left,
                 std::shared_ptr<const rcg::Image> disp);

    /**
      Returns the next model if available.
    */

    std::shared_ptr<gvr::Model> nextModel();

    /**
      Returns true if the background thread is running.
    */

    bool isRunning() { return running; }

  private:

    void run();

    struct InputMsg
    {
      double f;
      double t;
      double inv;
      double scale;
      double offset;
      std::shared_ptr<const rcg::Image> left;
      std::shared_ptr<const rcg::Image> disp;
    };

    gutil::MsgQueueReplace<std::shared_ptr<InputMsg> > in;

    gutil::Semaphore sem;
    std::shared_ptr<gvr::Model> model;

    gutil::Thread thread;
    std::atomic_bool running;
};

}

#endif
