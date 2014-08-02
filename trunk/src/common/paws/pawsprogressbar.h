/*
 * pawsprogressbar.h - Author: Andrew Craig
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
#ifndef PAWS_PROGRESS_BAR_HEADER
#define PAWS_PROGRESS_BAR_HEADER

#include "pawswidget.h"

/**
 * \addtogroup common_paws
 * @{ */

class pawsProgressBar : public pawsWidget
{
public:
    pawsProgressBar();
    pawsProgressBar(const pawsProgressBar &origin);
    ~pawsProgressBar();

    float GetTotalValue() const
    {
        return totalValue;
    }
    void SetTotalValue(float newValue)
    {
        totalValue = newValue;
    }

    /** set the base color
     *  @param red     red saturation value 
     *  @param green   green saturation value 
     *  @param blue    blue saturation value 
     */
    virtual void SetColor( int red, int green, int blue );

    /** flash a different color if value is below/above threshold.
     **  numbers outside of min & max = no flash.
     **  default = no flash.
     *  @param value  within this threshold, flash; outside it, no flash.
     *  @param low    if TRUE then flash below this level; if FALSE flash above.
     *  @param rate   switch colors every 'rate' ticks
     *  @param red     red saturation value 
     *  @param green   green saturation value 
     *  @param blue    blue saturation value 
     */
    virtual void SetFlash(float level, bool low, int rate, int red, int green, int blue );

    /** return the current "flash" level
     */
    float GetFlashLevel();

    /** use a different primary color if above a certain threshold
     *  @param level   threshold
     *  @param low     if TRUE then active below this level; if FALSE active above.
     *  @param red     red saturation value 
     *  @param green   green saturation value 
     *  @param blue    blue saturation value 
     */
    virtual void SetWarning( float level, bool low, int red, int green, int blue );

    /** return the current "Warn" level
     */
    float GetWarningLevel();

    /** use a different primary color if above a certain threshold
     *  @param level   threshold
     *  @param low     if TRUE then active below this level; if FALSE active above.
     *  @param red     red saturation value
     *  @param green   green saturation value
     *  @param blue    blue saturation value
     */
    virtual void SetDanger( float level, bool low, int red, int green, int blue );

    /** return the current "Danger" level
     */
    float GetDangerLevel();

    void Completed()
    {
        complete = true;
    }
    void SetCurrentValue(float newValue);
    float GetCurrentValue()
    {
        return currentValue;
    }
    virtual void Draw();

    static void DrawProgressBar(const csRect &rect, iGraphics3D* graphics3D, float percent,
                                int start_r, int start_g, int start_b,
                                int diff_r,  int diff_g,  int diff_b,
                                int alpha = 255);

    bool IsDone()
    {
        return complete;
    }
    bool Setup(iDocumentNode* node);

    void OnUpdateData(const char* dataname,PAWSData &value);

    /** increse values from left-to-right, or right-to-left ?
     * @param left  FALSE = left-to-right (default); TRUE = right-to-left (reversed)
     */
    void SetReversed( bool val );

private:
    float totalValue;
    float flashLevel;
    float warnLevel;
    float dangerLevel;
    float currentValue;
    float percent;
    int   flashRate;
    int   flashLastTime;
    bool  complete;
    bool  flashLow;	//Low = TRUE; High = FALSE
    bool  warnLow;	//Low = TRUE; High = FALSE
    bool  dangerLow;	//Low = TRUE; High = FALSE
    bool  On;		//Primary color showing = TRUE; flash color = FALSE;
    bool  reversed;     //FALSE = left-to-right (default); TRUE = right-to-left (reversed)

    int   start_r,start_g,start_b;
    int   flash_r,flash_g,flash_b;
    int   warn_r,warn_g,warn_b;
    int   danger_r,danger_g,danger_b;
    int   diff_r,diff_g,diff_b;
};

CREATE_PAWS_FACTORY(pawsProgressBar);

/** @} */

#endif
