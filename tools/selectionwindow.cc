/*
 * This file is part of the rc_genicam_3dviewer package.
 *
 * Copyright (c) 2019 Roboception GmbH
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

#include "selectionwindow.h"

#include <sstream>

namespace rcgv
{

SelectionWindow::SelectionWindow(const std::vector<std::string> &list) :
  BaseWindow("Please select ...", 640, 480)
{
  n=static_cast<int>(list.size());
  n=std::min(30, n);
  selection=-1;

  std::ostringstream out;

  for (int i=0; i<n; i++)
  {
    if (i < 10)
    {
      out << i << ": " << list[i] << std::endl;
    }
    else
    {
      out << static_cast<char>('a'+(i-10)) << ": " << list[i] << std::endl;
    }
  }

  if (static_cast<int>(list.size()) > n)
  {
    out << "..." << std::endl;
  }

  setInfoText(out.str().c_str());
  setVisible(true);

  int w, h;
  getDisplaySize(w, h);
  setPosition((w-480)/2, (h-400)/2);

  waitForClose();
}

void SelectionWindow::onResize(int w, int h)
{
  showBuffer();
}

void SelectionWindow::onKey(char c, SpecialKey key, int x, int y)
{
  selection=-1;

  if (c >= '0' && c <= '9')
  {
    selection=c-'0';
  }

  if (c >= 'a' && c <= 'z')
  {
    selection=10+c-'a';
  }

  if (selection < n)
  {
    sendClose();
  }
  else
  {
    selection=-1;
  }

  if (key == k_esc)
  {
    sendClose();
  }
}

int SelectionWindow::getSelection()
{
  return selection;
}

}
