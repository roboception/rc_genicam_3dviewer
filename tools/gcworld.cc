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

#include "gcworld.h"

#include <string>
#include <sstream>
#include <iomanip>
#include <vector>

namespace rcgv
{

GCWorld::GCWorld(int w, int h, const std::shared_ptr<Receiver> &_receiver) : GLWorld(w, h)
{
  selected=0;
  show_info=false;
  receiver=_receiver;
  sem_model.increment();
}

GCWorld::~GCWorld()
{ }

void GCWorld::addModel(const std::shared_ptr<gvr::Model> &model)
{
  {
    gutil::Lock lock(sem_model);
    current_model=model;
  }

  GLWorld::addModel(*model.get());
}

void GCWorld::setFramerate(double _fps)
{
  fps=_fps;

  if (show_info)
  {
    std::ostringstream out;
    out << "Framerate: " << std::setprecision(3) << fps << " Hz";
    setInfoLine(out.str().c_str());
  }
}

namespace
{

int getIndex(const std::vector<std::string> &list, const std::string &value)
{
  int ret=-1;

  for (size_t i=0; i<list.size(); i++)
  {
    if (value == list[i])
    {
      ret=i;
    }
  }

  return ret;
}

std::string paramEnum2String(const std::shared_ptr<Receiver> &receiver, const char *name)
{
  std::ostringstream out;
  bool writable;

  out << name;

  if (receiver->getWritable(writable, name))
  {
    std::vector<std::string> list;
    std::string value=receiver->getEnum(name, list);

    out << " [";

    if (writable)
    {
      for (size_t i=0; i<list.size(); i++)
      {
        out << list[i];
        if (i+1 < list.size()) out << ", ";
      }
    }
    else
    {
      out << value;
    }

    out << "]: " << value;
  }
  else
  {
    out << ": (not available)";
  }

  return out.str();
}

std::string paramBoolean2String(const std::shared_ptr<Receiver> &receiver, const char *name)
{
  std::ostringstream out;
  bool writable;

  out << name;

  if (receiver->getWritable(writable, name))
  {
    bool value=receiver->getBoolean(name);

    if (writable)
    {
      out << " [false, true]: ";

      if (value)
      {
        out << "true";
      }
      else
      {
        out << "false";
      }
    }
    else
    {
      if (value)
      {
        out << " [true]: true";
      }
      else
      {
        out << " [false]: false";
      }
    }
  }
  else
  {
    out << ": (not available)";
  }

  return out.str();
}

}

void GCWorld::onSpecialKey(int key, int x, int y)
{
  // disable showing framerate

  if (show_info)
  {
    show_info=false;
    setInfoLine("");
  }

  // change selection on cursor up or down

  if (key == GLUT_KEY_UP)
  {
    if (selected > 0) selected--;
  }

  if (key == GLUT_KEY_DOWN)
  {
    if (selected < 4-1) selected++;
  }

  // change value on cursor left or right

  switch (selected)
  {
    case 0: // enum DepthQuality
      {
        if (key == GLUT_KEY_LEFT)
        {
          std::vector<std::string> slist;
          std::string value=receiver->getEnum("DepthQuality", slist);
          int i=getIndex(slist, value);

          if (i > 0) i--;
          if (slist.size() > 0) receiver->setEnum("DepthQuality", slist[i]);
        }
        else if (key == GLUT_KEY_RIGHT)
        {
          std::vector<std::string> slist;
          std::string value=receiver->getEnum("DepthQuality", slist);
          int i=getIndex(slist, value);

          if (i+1 < static_cast<int>(slist.size())) i++;
          if (slist.size() > 0) receiver->setEnum("DepthQuality", slist[i]);
        }

        // show current setting

        setInfoLine(paramEnum2String(receiver, "DepthQuality"));
      }
      break;

    case 1: // bool DepthStaticScene
      {
        if (key == GLUT_KEY_LEFT)
        {
          bool value=receiver->getBoolean("DepthStaticScene");
          if (value) receiver->setBoolean("DepthStaticScene", false);
        }
        else if (key == GLUT_KEY_RIGHT)
        {
          bool value=receiver->getBoolean("DepthStaticScene");
          if (!value) receiver->setBoolean("DepthStaticScene", true);
        }

        // show current setting

        setInfoLine(paramBoolean2String(receiver, "DepthStaticScene"));
      }
      break;

    case 2: // bool DepthSmooth
      {
        if (key == GLUT_KEY_LEFT)
        {
          bool value=receiver->getBoolean("DepthSmooth");
          if (value) receiver->setBoolean("DepthSmooth", false);
        }
        else if (key == GLUT_KEY_RIGHT)
        {
          bool value=receiver->getBoolean("DepthSmooth");
          if (!value) receiver->setBoolean("DepthSmooth", true);
        }

        // show current setting

        setInfoLine(paramBoolean2String(receiver, "DepthSmooth"));
      }
      break;

    case 3: // enum LineSource (LineSelector=Out1)
      {
        receiver->setEnum("LineSelector", "Out1");

        if (key == GLUT_KEY_LEFT)
        {
          std::vector<std::string> slist;
          std::string value=receiver->getEnum("LineSource", slist);
          int i=getIndex(slist, value);

          if (i > 0) i--;
          if (slist.size() > 0) receiver->setEnum("LineSource", slist[i]);
        }
        else if (key == GLUT_KEY_RIGHT)
        {
          std::vector<std::string> slist;
          std::string value=receiver->getEnum("LineSource", slist);
          int i=getIndex(slist, value);

          if (i+1 < static_cast<int>(slist.size())) i++;
          if (slist.size() > 0) receiver->setEnum("LineSource", slist[i]);
        }

        // show current setting

        setInfoLine(paramEnum2String(receiver, "LineSource"));
      }
      break;
  }
}

void GCWorld::onKey(unsigned char key, int x, int y)
{
  if (key == 'c')
  {
    gutil::Lock lock(sem_model);

    if (current_model)
    {
      // get home directory

      std::string prefix;

      {
#ifdef WIN32
        const char *d=getenv("HomeDrive");
        const char *p=getenv("HomePath");

        if (d && p) prefix=std::string(d)+std::string(p)+"\\capture";
#else
        const char *p=getenv("HOME");
        if (p) prefix=std::string(p)+"/capture";
#endif
      }

      // determine name of file

      int c=0;
      std::string name;

      while (name.size() == 0 && c < 1000)
      {
        std::ostringstream out;
        out << prefix << "_" << std::setw(4) << std::setfill('0') << c++ << ".ply";

        std::ifstream file(out.str().c_str());
        if (!file.is_open())
        {
          name=out.str();
        }

        file.close();
      }

      try
      {
        // save PLY

        current_model->savePLY(name.c_str());

        // inform user that file has been saved

        setInfoLine(("Saved as "+name).c_str());
      }
      catch (const std::exception &ex)
      {
        setInfoLine(("Cannot store file "+name).c_str());
      }
    }
  }
  else if (key == 'i')
  {
    show_info=true;

    std::ostringstream out;
    out << "Framerate: " << std::setprecision(3) << fps << " Hz";
    setInfoLine(out.str().c_str());

    gvr::GLRedisplay();
  }
  else
  {
    if (show_info)
    {
      show_info=false;
      setInfoLine("");
    }

    GLWorld::onKey(key, x, y);
  }
}

void GCWorld::onMouseButton(int button, int state, int x, int y)
{
  show_info=false;
  GLWorld::onMouseButton(button, state, x, y);
}

}
