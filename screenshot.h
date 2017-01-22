#ifndef _SCREEN_SHOT_H_
#define _SCREEN_SHOT_H_

#include <stdio.h>

class ScreenRecorder{

  int nShot;
  int m_WindowWidth;
  int m_WindowHeight;

  char *cFileName;
  FILE *fScreenshot;
  int nSize;
  GLubyte *pixels;

  unsigned char* TGAheader;
  unsigned char* header;

  public:

  ScreenRecorder(){

  }

  ScreenRecorder(int width,int height){
    nShot = 0;
    m_WindowWidth = width;
    m_WindowHeight = height;

    cFileName = new char[64];
    nSize = m_WindowWidth * m_WindowHeight * 3;

    pixels = new GLubyte [nSize];
    if (pixels == NULL) return;    

    nShot = 0;

    TGAheader = new unsigned char[12];
    TGAheader[0] = 0;
    TGAheader[1] = 0;
    TGAheader[2] = 2;
    TGAheader[3] = 0;
    TGAheader[4] = 0;
    TGAheader[5] = 0;
    TGAheader[6] = 0;
    TGAheader[7] = 0;
    TGAheader[8] = 0;
    TGAheader[9] = 0;
    TGAheader[10] = 0;
    TGAheader[11] = 0;

    header = new unsigned char[6];
    header[0] = (unsigned char)(m_WindowWidth%256);
    header[1] = (unsigned char)(m_WindowWidth/256);
    header[2] = (unsigned char)(m_WindowHeight%256);
    header[3] = (unsigned char)(m_WindowHeight/256);
    header[4] = 24;
    header[5] = 0;


  }

  void TakeScreenshot()
  {

    while (1)
    {
      sprintf(cFileName,"data/frame%d.tga",1000000+nShot);
      fScreenshot = fopen(cFileName,"rb");
      if (fScreenshot == NULL) break;
      else fclose(fScreenshot);

      ++nShot;
    }   

    fScreenshot = fopen(cFileName,"wb");

    glReadPixels(0, 0, m_WindowWidth, m_WindowHeight, GL_RGB, 
        GL_UNSIGNED_BYTE, pixels);

    //convert to BGR format    
    //unsigned char temp;
    //int i = 0;
    //while (i < nSize)
    //{
    //  temp = pixels[i];       //grab blue
    //  pixels[i] = pixels[i+2];//assign red to blue
    //  pixels[i+2] = temp;     //assign blue to red
    //
    //  i += 3;     //skip to next blue byte
    //}

    fwrite(TGAheader, sizeof(unsigned char), 12, fScreenshot);
    fwrite(header, sizeof(unsigned char), 6, fScreenshot);
    fwrite(pixels, sizeof(GLubyte), nSize, fScreenshot);
    fclose(fScreenshot);

    return;


  }

};

#endif

