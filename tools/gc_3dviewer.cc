/*
 * This file is part of the rc_genicam_viewer package.
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

  std::cout << prgname << " [[<interface-id>:]<device-id>]" << std::endl;
  std::cout << std::endl;
  std::cout << "Requests synchronized intensity and disparity images, creates a colored mesh" << std::endl;
  std::cout << "and shows it in an OpenGL window." << std::endl;
  std::cout << std::endl;
  std::cout << "<device-id> Device from which images will taken. It can be ommitted if there" << std::endl;
  std::cout << "is only one device available." << std::endl;
}

std::shared_ptr<rcgv::Modeler> modeler;
std::shared_ptr<rcgv::Receiver> receiver;
std::shared_ptr<rcgv::GCWorld> world;
int id=0;

void getNextModel(int)
{
  std::shared_ptr<gvr::Model> model=modeler->nextModel();

  if (model)
  {
    int nextid=(id+1)%2;
    model->setID(1000+nextid);
    world->addModel(*model.get());
    world->removeAllModels(1000+id);
    id=nextid;
    gvr::GLRedisplay();
  }

  if (!receiver->isRunning())
  {
    exit(0);
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

    if (i < argc && std::string(argv[i]) == "-h")
    {
      printHelp(argv[0]);
      return 0;
    }

    const char *name=0;
    if (i < argc)
    {
      name=argv[i++];
    }

    // create modeler and receiver

    modeler=std::make_shared<rcgv::Modeler>();
    receiver=std::make_shared<rcgv::Receiver>(modeler, name);
    atexit(closeDevice);

    // create window

    gvr::GLInitWindow(-1, -1, 800, 600, "gc_viewer");
    world=std::make_shared<rcgv::GCWorld>(800, 600, receiver);

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

  rcg::System::clearSystems();

  return 0;
}
