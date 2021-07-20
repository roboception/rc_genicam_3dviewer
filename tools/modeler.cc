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

#include "modeler.h"

#include <rc_genicam_api/pixel_formats.h>

#include <gvr/coloredmesh.h>
#include <gimage/size.h>

namespace rcgv
{

Modeler::Modeler() : in(1), sem(1)
{
  // start background thread for streaming images

  running=true;
  thread.create(*this);
}

Modeler::~Modeler()
{
  // stop grabbing thread

  running=false;
  in.push(std::shared_ptr<InputMsg>());
  thread.join();
}

void Modeler::process(double f, double t, double inv, double scale, double offset,
  std::shared_ptr<const rcg::Image> left, std::shared_ptr<const rcg::Image> disp)
{
  std::shared_ptr<InputMsg> msg=std::make_shared<InputMsg>();

  msg->f=f;
  msg->t=t;
  msg->inv=inv;
  msg->scale=scale;
  msg->offset=offset;
  msg->left=left;
  msg->disp=disp;

  in.push(msg);
}

std::shared_ptr<gvr::Model> Modeler::nextModel()
{
  gutil::Lock lock(sem);

  std::shared_ptr<gvr::Model> ret=model;
  model.reset();

  return ret;
}

namespace
{

void getImage(gimage::ImageU8 &out, const std::shared_ptr<const rcg::Image> &in)
{
  size_t width=in->getWidth();
  size_t height=in->getHeight();
  size_t px=in->getXPadding();

  const uint8_t *ps=in->getPixels();

  if (in->getPixelFormat() == Mono8)
  {
    out.setSize(static_cast<long>(width), static_cast<long>(height), 1);

    gutil::uint8 *pt=out.getPtr(0, 0, 0);

    for (size_t k=0; k<height; k++)
    {
      for (size_t i=0; i<width; i++)
      {
        *pt++=*ps++;
      }

      ps+=px;
    }
  }
  else if (in->getPixelFormat() == RGB8)
  {
    out.setSize(static_cast<long>(width), static_cast<long>(height), 3);

    gutil::uint8 *rt=out.getPtr(0, 0, 0);
    gutil::uint8 *gt=out.getPtr(0, 0, 1);
    gutil::uint8 *bt=out.getPtr(0, 0, 2);

    for (size_t k=0; k<height; k++)
    {
      for (size_t i=0; i<width; i++)
      {
        *rt++=*ps++;
        *gt++=*ps++;
        *bt++=*ps++;
      }

      ps+=px;
    }
  }
  else if (in->getPixelFormat() == YCbCr411_8)
  {
    out.setSize(static_cast<long>(width), static_cast<long>(height), 3);

    size_t pstep=(width>>2)*6+px;

    gutil::uint8 *rt=out.getPtr(0, 0, 0);
    gutil::uint8 *gt=out.getPtr(0, 0, 1);
    gutil::uint8 *bt=out.getPtr(0, 0, 2);

    for (size_t k=0; k<height; k++)
    {
      for (size_t i=0; i<width; i+=4)
      {
        uint8_t rgb[12];
        rcg::convYCbCr411toQuadRGB(rgb, ps, static_cast<int>(i));

        for (int j=0; j<12; j+=3)
        {
          *rt++=rgb[j];
          *gt++=rgb[j+1];
          *bt++=rgb[j+2];
        }
      }

      ps+=pstep;
    }
  }
}

int getDisp(gimage::ImageFloat &dout, const std::shared_ptr<const rcg::Image> &din,
                 double inv, double scale, double offset)
{
  const int iinv=static_cast<int>(inv);

  size_t width=din->getWidth();
  size_t height=din->getHeight();

  const uint8_t *dps=din->getPixels();
  size_t dstep=din->getWidth()*sizeof(uint16_t)+din->getXPadding();

  dout.setSize(static_cast<long>(width), static_cast<long>(height), 1);
  float *dpt=dout.getPtr(0, 0, 0);

  int ret=0;
  if (din->isBigEndian()) // big endian
  {
    for (size_t k=0; k<height; k++)
    {
      int j=0;
      for (size_t i=0; i<width; i++)
      {
        int val=(static_cast<int>(dps[j])<<8)|dps[j+1];
        j+=2;

        float d=std::numeric_limits<float>::infinity();
        if (val != iinv)
        {
          d=static_cast<float>(val*scale+offset);
          ret++;
        }

        *dpt++=d;
      }

      dps+=dstep;
    }
  }
  else // little endian
  {
    for (size_t k=0; k<height; k++)
    {
      int j=0;
      for (size_t i=0; i<width; i++)
      {
        int val=(static_cast<int>(dps[j+1])<<8)|dps[j];
        j+=2;

        float d=std::numeric_limits<float>::infinity();
        if (val != iinv)
        {
          d=static_cast<float>(val*scale+offset);
          ret++;
        }

        *dpt++=d;
      }

      dps+=dstep;
    }
  }

  return ret;
}

}

void Modeler::run()
{
  while (running)
  {
    // wait for input message

    std::shared_ptr<InputMsg> msg=in.pop();

    if (msg)
    {
      float dstep=1.0f;

      // convert disparity image and get number of valid points

      gimage::ImageFloat disp;
      int n=getDisp(disp, msg->disp, msg->inv, msg->scale, msg->offset);

      // convert intensity or color image and resize to disparity image

      gimage::ImageU8 fimage;
      getImage(fimage, msg->left);

      gimage::ImageU8 dsimage;
      gimage::ImageU8 *image=&fimage;

      {
        int ds=(fimage.getWidth()+disp.getWidth()-1)/disp.getWidth();

        if (ds > 1)
        {
          dsimage=gimage::downscaleImage(fimage, ds);
          image=&dsimage;
          fimage.setSize(0, 0, 0);
        }
      }

      // create mesh

      gvr::ColoredMesh *mesh=new gvr::ColoredMesh();
      mesh->resizeVertexList(n, true, false);

      // store colors

      n=0;
      if (image->getDepth() == 3)
      {
        for (long k=0; k<image->getHeight(); k++)
        {
          for (long i=0; i<image->getWidth(); i++)
          {
            if (disp.isValid(i, k))
            {
              mesh->setColorComp(n, 0, image->get(i, k, 0));
              mesh->setColorComp(n, 1, image->get(i, k, 1));
              mesh->setColorComp(n, 2, image->get(i, k, 2));
              n++;
            }
          }
        }
      }
      else
      {
        for (long k=0; k<image->getHeight(); k++)
        {
          for (long i=0; i<image->getWidth(); i++)
          {
            if (disp.isValid(i, k))
            {
              gutil::uint8 c=image->get(i, k, 0);
              mesh->setColorComp(n, 0, c);
              mesh->setColorComp(n, 1, c);
              mesh->setColorComp(n, 2, c);
              n++;
            }
          }
        }
      }

      // reconstruct and store vertices

      double w2=disp.getWidth()/2.0-0.5;
      double h2=disp.getHeight()/2.0-0.5;

      double f=msg->f*disp.getWidth();

      n=0;
      for (long k=0; k<disp.getHeight(); k++)
      {
        for (long i=0; i<disp.getWidth(); i++)
        {
          double d=disp.get(i, k);
          if (disp.isValidS(static_cast<float>(d)))
          {
            gmath::Vector3d P;

            double s=msg->t/std::max(d, 0.1);

            P[0]=(i-w2)*s;
            P[1]=(k-h2)*s;
            P[2]=f*s;

            mesh->setVertexComp(n, 0, static_cast<float>(P[0]));
            mesh->setVertexComp(n, 1, static_cast<float>(P[1]));
            mesh->setVertexComp(n, 2, static_cast<float>(P[2]));

            double dx=(i+0.5-w2)*s-P[0];
            double dy=(k+0.5-h2)*s-P[1];

            mesh->setScanSize(n, static_cast<float>(2*std::sqrt(dx*dx+dy*dy)));

            double dz=P[2]-f*msg->t/(d+0.5);

            mesh->setScanError(n, static_cast<float>(dz));
            mesh->setScanConf(n, 1.0f);

            n++;
          }
        }
      }

      // count number of triangles

      int tn=0;
      for (long k=1; k<disp.getHeight(); k++)
      {
        for (long i=1; i<disp.getWidth(); i++)
        {
          float dmin=std::numeric_limits<float>::max();
          float dmax=-std::numeric_limits<float>::max();
          int   valid=0;

          for (int kk=0; kk<2; kk++)
          {
            for (int ii=0; ii<2; ii++)
            {
              if (disp.isValid(i-ii, k-kk))
              {
                dmin=std::min(dmin, disp.get(i-ii, k-kk));
                dmax=std::max(dmax, disp.get(i-ii, k-kk));
                valid++;
              }
            }
          }

          if (valid >= 3 && dmax-dmin <= dstep)
          {
            tn+=valid-2;
          }
        }
      }

      mesh->resizeTriangleList(tn);

      // create triangles

      std::vector<int> line0(static_cast<unsigned int>(disp.getWidth()));
      std::vector<int> line1(static_cast<unsigned int>(disp.getWidth()));
      std::vector<int> *l0=&line0;
      std::vector<int> *l1=&line1;

      n=0;
      tn=0;

      for (long i=0; i<disp.getWidth(); i++)
      {
        l1->at(i)=-1;

        if (disp.isValid(i, 0))
        {
          l1->at(i)=n++;
        }
      }

      for (long k=1; k<disp.getHeight(); k++)
      {
        std::vector<int> *t=l0;
        l0=l1;
        l1=t;

        l1->at(0)=-1;

        if (disp.isValid(0, k))
        {
          l1->at(0)=n++;
        }

        for (long i=1; i<disp.getWidth(); i++)
        {
          float dmin=std::numeric_limits<float>::max();
          float dmax=-std::numeric_limits<float>::max();
          int   valid=0;

          l1->at(static_cast<unsigned int>(i))=-1;

          if (disp.isValid(i, k))
          {
            l1->at(i)=n++;
          }

          for (int kk=0; kk<2; kk++)
          {
            for (int ii=0; ii<2; ii++)
            {
              if (disp.isValid(i-ii, k-kk))
              {
                dmin=std::min(dmin, disp.get(i-ii, k-kk));
                dmax=std::max(dmax, disp.get(i-ii, k-kk));
                valid++;
              }
            }
          }

          if (valid >= 3 && dmax-dmin <= dstep)
          {
            int j=0;
            int ff[4];

            if (l0->at(i-1) >= 0)
            {
              ff[j++]=l0->at(i-1);
            }

            if (l1->at(i-1) >= 0)
            {
              ff[j++]=l1->at(i-1);
            }

            if (l1->at(i) >= 0)
            {
              ff[j++]=l1->at(i);
            }

            if (l0->at(i) >= 0)
            {
              ff[j++]=l0->at(i);
            }

            mesh->setTriangleIndex(tn, 0, ff[0]);
            mesh->setTriangleIndex(tn, 1, ff[1]);
            mesh->setTriangleIndex(tn, 2, ff[2]);
            tn++;

            if (j == 4)
            {
              mesh->setTriangleIndex(tn, 0, ff[2]);
              mesh->setTriangleIndex(tn, 1, ff[3]);
              mesh->setTriangleIndex(tn, 2, ff[0]);
              tn++;
            }
          }
        }
      }

      // compute normals

      mesh->recalculateNormals();

      // set default camera

      mesh->setDefCameraRT(gmath::Matrix33d(), gmath::Vector3d());

      // make model available for polling

      {
        gutil::Lock lock(sem);
        model.reset(mesh);
      }
    }
  }
}

}
