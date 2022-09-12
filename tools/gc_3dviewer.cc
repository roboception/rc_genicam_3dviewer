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
#include "modeler.h"
#include "gcworld.h"

#include <Base/GCException.h>

#include <gvr/model.h>
#include <gvr/glmain.h>
#include <gvr/glworld.h>
#include <gutil/proctime.h>
#include <gutil/misc.h>
#include <gutil/exception.h>

#ifdef WIN32
#undef min
#undef max
#endif

namespace
{

/*
  Print help text on standard output.
*/

void printHelp(const char *prgname)
{
  // show help

  std::cout << prgname << " <options> [[<interface-id>:]<device-id> [<key>=<value> ...]]" << std::endl;
  std::cout << std::endl;
  std::cout << "Requests synchronized intensity and disparity images, creates a colored mesh" << std::endl;
  std::cout << "and shows it in an OpenGL window." << std::endl;
  std::cout << std::endl;
  std::cout << "- Press 'h' for an overview of general key codes." << std::endl;
  std::cout << "- Use cursor keys to switch between some GenICam parameters and their values." << std::endl;
  std::cout << std::endl;
  std::cout << "Command line options are:" << std::endl;
  std::cout << "-h              Shows this help and exits." << std::endl;
  std::cout << "-bg <r>,<g>,<b> Setting background color." << std::endl;
  std::cout << "-key <codes>    Sends the given keycodes to the viewer on startup." << std::endl;
  std::cout << "-timeout <t>    Timeout in seconds until giving up. 0 for inifinity." << std::endl;
  std::cout << std::endl;
  std::cout << "<device-id> Device from which images will taken. It can be ommitted if there" << std::endl;
  std::cout << "is only one device available." << std::endl;
  std::cout << std::endl;
  std::cout << "Genicam parameters can be given as key value pairs. They will be applied to the" << std::endl;
  std::cout << "device before streaming starts" << std::endl;
}

std::shared_ptr<rcgv::Modeler> modeler;
std::shared_ptr<rcgv::Receiver> receiver;
std::shared_ptr<rcgv::GCWorld> world;
int id=0;

void getNextModel(int)
{
  static double tprev=0;
  static int n=0;

  std::shared_ptr<gvr::Model> model=modeler->nextModel();

  if (model)
  {
    // set model and remove old one

    int nextid=(id+1)%2;
    model->setID(1000+nextid);
    world->addModel(model);
    world->removeAllModels(1000+id);
    id=nextid;

    // measure framerate

    n++;
    double tcurr=gutil::ProcTime::monotonic();

    if (tprev == 0)
    {
      tprev=tcurr;
      n=0;
    }

    if (tcurr-tprev > 2)
    {
      world->setFramerate(n/(tcurr-tprev));
      tprev=tcurr;
      n=0;
    }

    // redisplay

    gvr::GLRedisplay();
  }

  if (!receiver->isRunning())
  {
    gvr::GLLeaveMainLoop();
    return;
  }

  gvr::GLTimerFunc(40, getNextModel, 0);
}

void closeDevice()
{
  receiver->close();
}

}

int main(int argc, char *argv[])
{
  try
  {
    gvr::GLInit(argc, argv);

    int i=1;
    std::string bg="44,51,58";
    std::string keycodes;
    double timeout=3;

    while (i < argc && argv[i][0] == '-')
    {
      if (i < argc && std::string(argv[i]) == "-h")
      {
        printHelp(argv[0]);
        return 0;
      }
      else if (i+1 < argc && std::string(argv[i]) == "-bg")
      {
        i++;
        bg=argv[i++];
      }
      else if (i+1 < argc && std::string(argv[i]) == "-key")
      {
        i++;
        keycodes=argv[i++];
      }
      else if (i+1 < argc && std::string(argv[i]) == "-timeout")
      {
        i++;
        timeout=std::stod(argv[i++]);
      }
      else
      {
        gutil::showError((std::string("Unknown parameter or missing value: ")+argv[i]).c_str());
        return 1;
      }
    }

    const char *name=0;
    std::vector<std::string> genicam_param;
    if (i < argc)
    {
      name=argv[i++];

      while (i < argc)
      {
        genicam_param.push_back(std::string(argv[i++]));
      }
    }

    // create modeler and receiver

    modeler=std::make_shared<rcgv::Modeler>();
    receiver=std::make_shared<rcgv::Receiver>(modeler, name, timeout, genicam_param);
    atexit(closeDevice);

    // create window

    gvr::GLInitWindow(-1, -1, 800, 600, "gc_3dviewer");
    world=std::make_shared<rcgv::GCWorld>(800, 600, receiver);
    world->setCapturePrefix("capture");

    // set background color

    if (bg.size() > 0)
    {
      std::vector<std::string> list;

      gutil::split(list, bg, ',');

      if (list.size() != 3)
      {
        throw gutil::InvalidArgumentException(std::string("Illegal format: ")+bg);
      }

      float r=std::max(0.0f, std::min(1.0f, std::stoi(list[0])/255.0f));
      float g=std::max(0.0f, std::min(1.0f, std::stoi(list[1])/255.0f));
      float b=std::max(0.0f, std::min(1.0f, std::stoi(list[2])/255.0f));

      world->setBackgroundColor(r, g, b);
    }

    // apply keycodes

    for (size_t k=0; k<keycodes.size(); k++)
    {
      world->onKey(keycodes[k], 0, 0);
    }

    // register additional timer callback

    gvr::GLTimerFunc(40, getNextModel, 0);

    // enter main loop

    GLMainLoop(*world.get());

    // free receiver and modeler

    receiver.reset();
    modeler.reset();
  }
  catch (const std::exception &ex)
  {
    gutil::showError(ex.what());
  }
  catch (const GENICAM_NAMESPACE::GenericException &ex)
  {
    gutil::showError(ex.what());
  }
  catch (...)
  {
    gutil::showError("Unknown exception!");
  }

  rcg::System::clearSystems();

  return 0;
}
