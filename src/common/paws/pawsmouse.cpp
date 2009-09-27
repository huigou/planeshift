/*
 * pawsmouse.cpp - Author: Andrew Craig
 *
 * Copyright (C) 2003 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation (version 2 of the License)
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */
// pawsmouse.cpp: implementation of the pawsMouse class.
//
//////////////////////////////////////////////////////////////////////
#include <psconfig.h>

#include <csutil/ref.h>
#include <ivideo/graph3d.h>
#include <ivideo/graph2d.h>
#include <igraphic/imageio.h>
#include <iutil/vfs.h>
#include <csutil/databuf.h>

#include "pawsmouse.h"
#include "pawsmanager.h"
#include "pawstexturemanager.h"
#include "pawsimagedrawable.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

pawsMouse::pawsMouse()
{
    graphics3D = PawsManager::GetSingleton().GetGraphics3D();
    vfs =  csQueryRegistry<iVFS > ( PawsManager::GetSingleton().GetObjectRegistry());
    imageLoader =  csQueryRegistry<iImageIO > ( PawsManager::GetSingleton().GetObjectRegistry());

    currentPosition = psPoint( 0,0 );
    deltas = psPoint( 0, 0 );
    hidden = false;
    crosshair = false;
    crosshairImage = PawsManager::GetSingleton().GetTextureManager()->GetPawsImage("Crosshair Mouse Pointer");
    useOS = false;
}

pawsMouse::~pawsMouse()
{
}

void pawsMouse::SetPosition( int x, int y )
{
    deltas.x = x - currentPosition.x;
    deltas.y = y - currentPosition.y;

    currentPosition.x = x;
    currentPosition.y = y;
}

void pawsMouse::ChangeImage( const char* imageName )
{
    cursorImage = PawsManager::GetSingleton().GetTextureManager()->GetPawsImage(imageName);
    SetOSMouse(cursorImage);
}

void pawsMouse::ChangeImage(csRef<iPawsImage> drawable)
{
    cursorImage = drawable;
    SetOSMouse(cursorImage);
}

void pawsMouse::SetOSMouse(csRef<iPawsImage> drawable)
{
#ifdef CS_PLATFORM_WIN32
    pawsImageDrawable * pwDraw = dynamic_cast<pawsImageDrawable *>((iPawsImage *)drawable);
    if (!pwDraw)
      return;

    image = pwDraw->GetImage();
    if (!image)
      return;

    iGraphics2D *g2d = graphics3D->GetDriver2D();

    transparentR = pwDraw->GetTransparentRed();
    transparentG = pwDraw->GetTransparentGreen();
    transparentB = pwDraw->GetTransparentBlue();

    // Attempt to use image to enable OS level cursor
    csRGBcolor color(transparentR, transparentG, transparentB);
    if (g2d->SetMouseCursor (image, &color))
    {
      if (!useOS)
          Debug1(LOG_PAWS,0,"Using OS Cursor\n");
      useOS = true;
      return;
    }
    else g2d->SetMouseCursor (csmcNone);
#endif
}

void pawsMouse::Draw()
{
    if (!useOS && !hidden && cursorImage)
        cursorImage->Draw(currentPosition.x , currentPosition.y );
    if (crosshair && crosshairImage)
        crosshairImage->Draw( graphics3D->GetDriver2D()->GetWidth() / 2, graphics3D->GetDriver2D()->GetHeight() / 2 );
}

void pawsMouse::Hide(bool h)
{
    hidden = h;
#ifdef CS_PLATFORM_WIN32
    if (h)
        graphics3D->GetDriver2D()->SetMouseCursor (csmcNone);
    else
        if (useOS)
        {
            csRGBcolor color(transparentR, transparentG, transparentB);
            graphics3D->GetDriver2D()->SetMouseCursor (image, &color);
        }
#endif
}
