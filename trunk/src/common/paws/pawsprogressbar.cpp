/*
 * pawsprogressbar.cpp- Author: Andrew Craig
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

#include <psconfig.h>
#include <csgeom/vector4.h>
#include "pawsprogressbar.h"
#include "pawsmanager.h"

pawsProgressBar::pawsProgressBar()
{
    totalValue   = 0.0f;
    complete     = false;
    currentValue = 0.0f;
    percent      = 0.0f;
    factory      = "pawsProgressBar";
    warnLevel    = -1;
    warnLow      = true;
    dangerLevel  = -1;
    dangerLow    = true;
    flashLevel   = -1;
    flashLastTime= -1;
    flashLow     = true;
    flashRate    = 0;
    On           = true;
    reversed     = false; 

    start_r=start_g=start_b=255;
    flash_r=flash_g=flash_b=0;
    warn_r=warn_g=200;
    warn_b=0;
    danger_r=200;
    danger_g=danger_b=0;
    diff_r=diff_g=diff_b=100;

}
pawsProgressBar::pawsProgressBar(const pawsProgressBar &origin)
    :pawsWidget(origin),
     totalValue(origin.totalValue),
     currentValue(origin.currentValue),
     percent(origin.percent),
     complete(origin.complete),
     start_r(origin.start_r),start_g(origin.start_g),start_b(origin.start_b),
     diff_r(origin.diff_r),diff_g(origin.diff_g),diff_b(origin.diff_b)
{

}
pawsProgressBar::~pawsProgressBar()
{
}

bool pawsProgressBar::Setup(iDocumentNode* node)
{
    csRef<iDocumentNode> ColourNode = node->GetNode("color");
    if(ColourNode)
    {
        start_r = ColourNode->GetAttributeValueAsInt("r");
        start_g = ColourNode->GetAttributeValueAsInt("g");
        start_b = ColourNode->GetAttributeValueAsInt("b");
    }
    else
    {
        start_r = 0;
        start_g = 0;
        start_b = 25;
    }

    csRef<iDocumentNode> ColourNode2 = node->GetNode("fadecolor");
    if(ColourNode2)
    {
        diff_r = ColourNode2->GetAttributeValueAsInt("r") - start_r;
        diff_g = ColourNode2->GetAttributeValueAsInt("g") - start_g;
        diff_b = ColourNode2->GetAttributeValueAsInt("b") - start_b;
    }

    csRef<iDocumentNode> ColourNode3 = node->GetNode("flashcolor");
    if(ColourNode3)
    {
        flash_r = ColourNode3->GetAttributeValueAsInt("r");
        flash_g = ColourNode3->GetAttributeValueAsInt("g");
        flash_b = ColourNode3->GetAttributeValueAsInt("b");
    }
    else
    {
        flash_r = 0;
        flash_g = 0;
        flash_b = 0;
    }

    csRef<iDocumentNode> ColourNode4 = node->GetNode("warncolor");
    if(ColourNode4)
    {
        warn_r = ColourNode4->GetAttributeValueAsInt("r");
        warn_g = ColourNode4->GetAttributeValueAsInt("g");
        warn_b = ColourNode4->GetAttributeValueAsInt("b");
    }
    else
    {
        warn_r = 200;
        warn_g = 200;
        warn_b = 0;
    }

    csRef<iDocumentNode> ColourNode5 = node->GetNode("dangercolor");
    if(ColourNode5)
    {
        danger_r = ColourNode5->GetAttributeValueAsInt("r");
        danger_g = ColourNode5->GetAttributeValueAsInt("g");
        danger_b = ColourNode5->GetAttributeValueAsInt("b");
    }
    else
    {
        danger_r = 200;
        danger_g = 0;
        danger_b = 0;
    }
    return true;
}

void pawsProgressBar::SetCurrentValue(float newValue)
{
    currentValue = newValue;

    percent = totalValue ? (currentValue / totalValue) : 0;
}

void pawsProgressBar::Draw()
{
    ClipToParent(false);

    int alpha = 255;
    int Time  = csGetTicks();
    int primary_r;
    int primary_g;
    int primary_b;

    if(parent && !parent->GetBackground().IsEmpty() && parent->isFadeEnabled() && parent->GetMaxAlpha() != parent->GetMinAlpha())
    {
        alpha = (int) (255 - (parent->GetMinAlpha() + (parent->GetMaxAlpha()-parent->GetMinAlpha()) * parent->GetFadeVal() * 0.010));
    }
    DrawBackground();
    if( flashLevel > 0 )
    {
        if( ( (flashLow && percent<flashLevel) || (!flashLow && percent>flashLevel) ) )
        {
            if( flashLastTime+flashRate <= Time )
            {
                On=!On;
                flashLastTime = Time;
            }
        }
        else
        {
            On=true;
        }
    }
    if( warnLevel > 0 && ( (warnLow && percent < warnLevel) || (!warnLow && percent >warnLevel) ) )
    {
        if( dangerLevel > 0 && ( (dangerLow && percent < dangerLevel) || (!dangerLow && percent >dangerLevel) ) )
        {
            primary_r = danger_r;
            primary_g = danger_g;
            primary_b = danger_b;
        }
        else
        {
            primary_r = warn_r;
            primary_g = warn_g;
            primary_b = warn_b;
        }
    }
    else
    {
        primary_r = start_r;
        primary_g = start_g;
        primary_b = start_b;
    }
    if( On )
    {
        DrawProgressBar(screenFrame, PawsManager::GetSingleton().GetGraphics3D(), percent,
            primary_r, primary_g, primary_b,
            diff_r,  diff_g,  diff_b, alpha);
    }
    else
    {
        DrawProgressBar(screenFrame, PawsManager::GetSingleton().GetGraphics3D(), percent,
            flash_r, flash_g, flash_b,
            flash_r, flash_g, flash_b, alpha);
    }
    DrawChildren();
    DrawMask();
}

void pawsProgressBar::DrawProgressBar(
    const csRect &rect, iGraphics3D* graphics3D, float percent,
    int start_r, int start_g, int start_b,
    int diff_r,  int diff_g,  int diff_b, int alpha)
{
    csSimpleRenderMesh mesh;
    static uint indices[4] = {0, 1, 2, 3};
    csVector3 verts[4];
    csVector4 colors[4];
    float fr1 = start_r / 255.0f;
    float fg1 = start_g / 255.0f;
    float fb1 = start_b / 255.0f;
    float fr2 = fr1 + percent * (diff_r / 255.0f);
    float fg2 = fg1 + percent * (diff_g / 255.0f);
    float fb2 = fb1 + percent * (diff_b / 255.0f);
    float fa = alpha / 255.0f;

    mesh.meshtype = CS_MESHTYPE_QUADS;
    mesh.indexCount = 4;
    mesh.indices = indices;
    mesh.vertexCount = 4;
    mesh.vertices = verts;
    mesh.colors = colors;
    mesh.mixmode = CS_FX_COPY;
    mesh.alphaType.autoAlphaMode = false;
    mesh.alphaType.alphaType = csAlphaMode::alphaSmooth;

    verts[0].Set(rect.xmin, rect.ymin, 0);
    colors[0].Set(fr1, fg1, fb1, fa);

    verts[1].Set(rect.xmin + (rect.Width() * percent), rect.ymin, 0);
    colors[1].Set(fr2, fg2, fb2, fa);

    verts[2].Set(rect.xmin + (rect.Width() * percent), rect.ymax, 0);
    colors[2].Set(fr2, fg2, fb2, fa);

    verts[3].Set(rect.xmin, rect.ymax, 0);
    colors[3].Set(fr1, fg1, fb1, fa);

    graphics3D->DrawSimpleMesh(mesh, csSimpleMeshScreenspace);
}

void pawsProgressBar::OnUpdateData(const char* /*dataname*/, PAWSData &value)
{
    SetCurrentValue(value.GetFloat());
}

void pawsProgressBar::SetColor( int red, int green, int blue )
{
    start_r    = red;
    start_g    = green;
    start_b    = blue;
}

void pawsProgressBar::SetFlash(float level, bool low, int rate, int red, int green, int blue )
{
    flashLevel = level;
    flashLow   = low;
    flashRate  = rate;
    flash_r    = red;
    flash_g    = green;
    flash_b    = blue;
}
float pawsProgressBar::GetFlashLevel()
{
    return flashLevel;
}


void pawsProgressBar::SetReversed( bool val )
{
    reversed = val;
}

void pawsProgressBar::SetWarning( float level, bool low, int red, int green, int blue )
{
    warnLevel = level;
    warnLow   = low;
    warn_r    = red;
    warn_g    = green;
    warn_b    = blue;
}
float pawsProgressBar::GetWarningLevel()
{
    return warnLevel;
}

void pawsProgressBar::SetDanger( float level, bool low, int red, int green, int blue )
{
    dangerLevel = level;
    dangerLow   = low;
    danger_r    = red;
    danger_g    = green;
    danger_b    = blue;
}
float pawsProgressBar::GetDangerLevel()
{
    return dangerLevel;
}


