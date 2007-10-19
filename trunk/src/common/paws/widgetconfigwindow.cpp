/*
 * widgetconfigwindow.cpp - Author: Ian Donderwinkel
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


// CS INCLUDES
#include <psconfig.h>

#include "pawswidget.h"
#include "pawscrollbar.h"
#include "pawstextbox.h"
#include "pawsprogressbar.h"
#include "pawsbutton.h"
#include "pawsmainwidget.h"
#include "pawswidget.h"
#include "pawsmanager.h"
#include "util/psstring.h"

#include "widgetconfigwindow.h"

//-----------------------------------------------------------------------------
//                            class WidgetConfigWindow
//-----------------------------------------------------------------------------
WidgetConfigWindow::WidgetConfigWindow()
{
    SetAlwaysOnTop(true);
    PawsManager::GetSingleton().SetCurrentFocusedWidget(this);  
    configWidget = NULL;
}


bool WidgetConfigWindow::OnScroll( int direction, pawsScrollBar* widget )
{
    if (!configWidget)
        return true;

    psString pct;
    if (widget == scrollBarMinAlpha)
    {
        // get current minimum alpha value from scrollbar
        float value = scrollBarMinAlpha->GetCurrentValue();

        // this minimum value must be less than the maximum alpha value 
        if (value<scrollBarMaxAlpha->GetCurrentValue())
        {
            // update the progress bar
            progressBarMinAlpha->SetCurrentValue(value);
            // set the alpha value of the widget to this value
            // to give the user an idea of how this will look
            // DO NOT call draw() here, it is not necessary
            configWidget->SetBackgroundAlpha((int)value);

            // show alpha in %
            pct.Format("%.0f%%",value/2.55f);
            textMinAlphaPct->SetText(pct);
    
            currentMinAlpha = value;
        }
        else
        {
            // undo scrollbar action
            scrollBarMinAlpha->SetCurrentValue(currentMinAlpha, false);
        }
    }
    else if (widget == scrollBarMaxAlpha)
    {
        // update the scroll and progress bar for max alpha
        float value = scrollBarMaxAlpha->GetCurrentValue();

        // max alpha must not be less than min alpha
        if (value>scrollBarMinAlpha->GetCurrentValue())
        {
            progressBarMaxAlpha->SetCurrentValue(value);
            // If we are fading, then show possible max alpha.
            if( buttonFade->GetState())
                configWidget->SetBackgroundAlpha((int)value);

            pct.Format("%.0f%%",value/2.55f);
            textMaxAlphaPct->SetText(pct);

            currentMaxAlpha = value;
        } else    
        {
            scrollBarMaxAlpha->SetCurrentValue(currentMaxAlpha, false);
        }
    }
    else if (widget == scrollBarFadeSpeed)
    {
        // update the scroll and progress bar
        float value = scrollBarFadeSpeed->GetCurrentValue();
        progressBarFadeSpeed->SetCurrentValue(value);
        configWidget->SetFadeSpeed(value);

        pct.Format("%.0f%%",value/0.10f);
        textFadeSpeedPct->SetText(pct);
        currentFadeSpeed = value;
    }
    else if (widget == scrollBarFontScaling)
    {
        // update the scroll and progress bar
        float value = scrollBarFontScaling->GetCurrentValue();
        progressBarFontScaling->SetCurrentValue(value-50);
        configWidget->SetFontScaling((int)value);

        pct.Format("%.0f%%",value);
        textFontScalingPct->SetText(pct);
        currentFontScaling = value;
    }

    return true;
}


bool WidgetConfigWindow::OnButtonPressed( int mouseButton, int keyModifier, pawsWidget* widget )
{
    if (widget == buttonOK)
    {
        // store 
        configWidget->SetMinAlpha((int)currentMinAlpha);
        configWidget->SetMaxAlpha((int)currentMaxAlpha);    
        configWidget->SetFontScaling((int)currentFontScaling);

        // close this window
        //PawsManager::GetSingleton().GetMainWidget()->DeleteChild(this);
        Hide();
        return true;
    } 
    else if (widget == buttonCancel)
    {
        // restore settings
        configWidget->SetMinAlpha((int)oldMinAlpha);
        configWidget->SetMaxAlpha((int)oldMaxAlpha);
        configWidget->SetFadeSpeed(oldFadeSpeed);
        configWidget->SetFade(oldFadeStatus);
        configWidget->SetFontScaling((int)oldFontScaling);

        // close this window
        //PawsManager::GetSingleton().GetMainWidget()->DeleteChild(this);
        Hide();
        return true;
    }
    else if (widget == buttonFade)
    {
        // enables/disables fading
        bool newFadeStatus = !configWidget->isFadeEnabled();
        configWidget->SetFade(newFadeStatus);
        buttonFade->SetState(newFadeStatus);

        textFadeStatus->SetText( buttonFade->GetState() ? "Enabled" : "Disabled" );

        currentFadeStatus = newFadeStatus;
        return true;
    }
    else if(widget == buttonApply)
    {
        pawsMainWidget* main = PawsManager::GetSingleton().GetMainWidget();
        main->ApplyWindowSettingsOnChildren( configWidget,
                                             (int)currentMinAlpha,
                                             (int)currentMaxAlpha,
                                             currentFadeSpeed,
                                             currentFadeStatus,
	                                         (int)currentFontScaling
                                           );
        Hide();
        return true;
    }
    return false;
}

void WidgetConfigWindow::SetConfigurableWidget(pawsWidget *configWidget)
{
    this->configWidget = configWidget;

    // get new values
    currentMinAlpha   = configWidget->GetMinAlpha();
    currentMaxAlpha   = configWidget->GetMaxAlpha();
    currentFadeStatus = configWidget->isFadeEnabled();
    currentFadeSpeed  = configWidget->GetFadeSpeed();
    currentFontScaling = configWidget->GetFontScaling();

    // set title "Window Settings (widgetname)"
    psString title;
    title = "Window Settings (";
    title += configWidget->GetName();
    title += ")";
    this->SetTitle(title);
    
    csString closeButtonName(configWidget->GetName());
    closeButtonName.Append("settings_close");
    if ( close_widget )
        close_widget->SetName( closeButtonName );
    
    // store old values
    oldMinAlpha = currentMinAlpha;
    oldMaxAlpha = currentMaxAlpha;
    oldFadeStatus = currentFadeStatus;
    oldFadeSpeed  = currentFadeSpeed;
    oldFontScaling = currentFontScaling;

    scrollBarMinAlpha->SetCurrentValue(currentMinAlpha);
    progressBarMinAlpha->SetCurrentValue(currentMinAlpha);

    scrollBarMaxAlpha->SetCurrentValue(currentMaxAlpha);
    progressBarMaxAlpha->SetCurrentValue(currentMaxAlpha);

    scrollBarFadeSpeed->SetCurrentValue(currentFadeSpeed);
    progressBarFadeSpeed->SetCurrentValue(currentFadeSpeed);

    psString pct;
    pct.Format("%.0f%%",currentMinAlpha/2.55f);
    textMinAlphaPct->SetText(pct);

    pct.Format("%.0f%%",currentMaxAlpha/2.55f);
    textMaxAlphaPct->SetText(pct);

    pct.Format("%.0f%%",currentFadeSpeed/0.10f);
    textFadeSpeedPct->SetText(pct);

    buttonFade->SetState(configWidget->isFadeEnabled());
    textFadeStatus->SetText( buttonFade->GetState() ? "Enabled" : "Disabled" );

    // Change the window to allow font size scaling, if allowed for this widget
    bool canScaleFont = (configWidget->GetFontScaling() != 0);
    if (canScaleFont)
    {
        scrollBarFontScaling->SetCurrentValue(currentFontScaling);
        progressBarFontScaling->SetCurrentValue(currentFontScaling-50);

        pct.Format("%.0f%%",currentFontScaling);
        textFontScalingPct->SetText(pct);
    }
    SetFontSliderVisibility(canScaleFont);
}

void WidgetConfigWindow::SetFontSliderVisibility(bool visible)
{
    if (visible)
    {
        // Show all font stuff
        textFontScalingLabel->Show();
        textFontScalingPct->Show();
        progressBarFontScaling->Show();
        scrollBarFontScaling->Show();

        // Change window size to fit
        this->SetSize( this->max_width, this->max_height );
    }
    else
    {
        // Hide all font stuff
        textFontScalingLabel->Hide();
        textFontScalingPct->Hide();
        progressBarFontScaling->Hide();
        scrollBarFontScaling->Hide();

        // Change window size to fit
        this->SetSize( this->min_width, this->min_height );
    }
}

bool WidgetConfigWindow::PostSetup()
{
    if ((scrollBarMinAlpha   =(pawsScrollBar*  )this->FindWidget("MinAlphaScroll")) == NULL) return false;
    if ((progressBarMinAlpha =(pawsProgressBar*)this->FindWidget("MinAlphaProgressBar")) == NULL) return false;
    scrollBarMinAlpha->SetMaxValue(255);
    scrollBarMinAlpha->SetTickValue(5);
    scrollBarMinAlpha->EnableValueLimit(true);
    progressBarMinAlpha->SetTotalValue(255);

    if ((scrollBarMaxAlpha   =(pawsScrollBar*  )this->FindWidget("MaxAlphaScroll")) == NULL) return false;
    if ((progressBarMaxAlpha =(pawsProgressBar*)this->FindWidget("MaxAlphaProgressBar")) == NULL) return false;
    scrollBarMaxAlpha->SetMaxValue(255);
    scrollBarMaxAlpha->SetTickValue(5);
    scrollBarMaxAlpha->EnableValueLimit(true);
    progressBarMaxAlpha->SetTotalValue(255);

    if ((scrollBarFadeSpeed      =(pawsScrollBar*  )this->FindWidget("FadeSpeedScroll")) == NULL) return false;
    if ((progressBarFadeSpeed =(pawsProgressBar*)this->FindWidget("FadeSpeedProgressBar")) == NULL) return false;
    scrollBarFadeSpeed->SetMaxValue(10);
    scrollBarFadeSpeed->SetTickValue(1);
    scrollBarFadeSpeed->EnableValueLimit(true);
    progressBarFadeSpeed->SetTotalValue(10);

    if ((scrollBarFontScaling   = (pawsScrollBar*   )this->FindWidget("FontScalingScroll")) == NULL) return false;
    if ((progressBarFontScaling = (pawsProgressBar* )this->FindWidget("FontScalingProgressBar")) == NULL) return false;
    scrollBarFontScaling->SetMaxValue(150);
    scrollBarFontScaling->SetMinValue(50);
    scrollBarFontScaling->SetTickValue(1);
    scrollBarFontScaling->EnableValueLimit(true);
    progressBarFontScaling->SetTotalValue(100);

    if ((buttonOK     =(pawsButton*)this->FindWidget("OKButton")) == NULL) return false;
    if ((buttonCancel =(pawsButton*)this->FindWidget("CancelButton")) == NULL) return false;
    if ((buttonApply  =(pawsButton*)this->FindWidget("ApplyAllButton")) == NULL) return false;
    if ((buttonFade   =(pawsButton*)this->FindWidget("FadeButton")) == NULL) return false;
    
    if ((textFadeStatus    =(pawsTextBox*)this->FindWidget("FadeStatus")) == NULL) return false;
    if ((textMinAlphaPct   =(pawsTextBox*)this->FindWidget("MinAlphaCurrentPct")) == NULL) return false;
    if ((textMaxAlphaPct   =(pawsTextBox*)this->FindWidget("MaxAlphaCurrentPct")) == NULL) return false;
    if ((textFadeSpeedPct  =(pawsTextBox*)this->FindWidget("FadeSpeedCurrentPct")) == NULL) return false;
    if ((textFontScalingPct=(pawsTextBox*)this->FindWidget("FontScalingCurrentPct")) == NULL) return false;
    if ((textFontScalingLabel=(pawsTextBox*)this->FindWidget("FontScalingText")) == NULL) return false;

    // place window on current mouse position
    MoveTo((int)PawsManager::GetSingleton().GetMouse()->GetPosition().x,
           (int)PawsManager::GetSingleton().GetMouse()->GetPosition().y);

    return true;
}

