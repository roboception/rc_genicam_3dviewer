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

#ifndef RC_GENICAM_VIEWER_GCWORLD
#define RC_GENICAM_VIEWER_GCWORLD

#include <gutil/semaphore.h>
#include <gutil/proctime.h>
#include <gvr/glworld.h>
#include "receiver.h"

#include <memory>

namespace rcgv
{

/**
  Adds possibility to change some parameters of the device to the GLWorld class.
*/

class GCWorld: public gvr::GLWorld
{
  public:

    GCWorld(int w, int h, const std::shared_ptr<Receiver> &receiver);
    virtual ~GCWorld();

    void addModel(const std::shared_ptr<gvr::Model> &model);
    void setFramerate(double fps);

    virtual void onSpecialKey(int key, int x, int y);
    virtual void onKey(unsigned char key, int x, int y);
    virtual void onMouseButton(int button, int state, int x, int y);

  private:

    int selected;
    bool show_info;
    double fps;
    std::shared_ptr<Receiver> receiver;

    bool toggle_texture_on_double_click;
    gutil::ProcTime mt;
    int mx, my;

    gutil::Semaphore sem_model;
    std::shared_ptr<gvr::Model> current_model;
};

}

#endif
