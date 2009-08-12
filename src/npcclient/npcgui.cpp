#include <psconfig.h>

#include <cstool/initapp.h>
#include <iutil/cfgmgr.h>
#include <iutil/eventq.h>
#include <ivideo/graph2d.h>
#include <ivideo/natwin.h>

#include "paws/pawsmainwidget.h"
#include "paws/pawsmanager.h"

#include "npcgui.h"

NpcGui::NpcGui(iObjectRegistry* object_reg, psNPCClient* npcclient)
: object_reg(object_reg), npcclient(npcclient)
{
}

bool NpcGui::Initialise()
{
  g3d = csQueryRegistry<iGraphics3D> (object_reg);
  if (!g3d)
  {
    printf("iGraphics3D failed to Init!\n");
    return false;
  }

  g2d = g3d->GetDriver2D();
  if (!g2d)
  {
    printf("iGraphics2D failed to Init!\n");
    return false;
  }

  if(!csInitializer::OpenApplication(object_reg))
  {
    printf("Error initialising app (CRYSTAL not set?)\n");
    return false;
  }

  iNativeWindow *nw = g2d->GetNativeWindow();
  if (nw)
    nw->SetTitle("PlaneShift NPC Client");

  g2d->AllowResize(true);

  csRef<iConfigManager> configManager = csQueryRegistry<iConfigManager> (object_reg);
  if (!configManager)
  {
    printf("configManager failed to Init!\n");
    return false;
  }

  // paws initialization
  csString skinPath;
  skinPath = configManager->GetStr("PlaneShift.GUI.Skin", "/planeshift/art/npcclient.zip");
  paws = new PawsManager(object_reg, skinPath);
  if (!paws)
  {
    printf("Failed to init PAWS!\n");
    return false;
  }

  mainWidget = new pawsMainWidget();
  paws->SetMainWidget(mainWidget);

  // Load and assign a default button click sound for pawsbutton
  paws->LoadSound("/planeshift/art/sounds/gui/next.wav","sound.standardButtonClick");

  // Set mouse image.
  paws->GetMouse()->ChangeImage("Standard Mouse Pointer");

  // Register our event handler
  event_handler = csPtr<EventHandler> (new EventHandler (this));
  csEventID esub[] = 
  {
    csevFrame (object_reg),
    csevMouseEvent (object_reg),
    csevKeyboardEvent (object_reg),
    csevQuit (object_reg),
    CS_EVENTLIST_END
  };

  queue = csQueryRegistry<iEventQueue> (object_reg);
  if (!queue)
  {
    printf("No iEventQueue plugin!\n");
    return false;
  }
  queue->RegisterListener(event_handler, esub);

  return true;
}

NpcGui::~NpcGui()
{
  csInitializer::CloseApplication(object_reg);
}

bool NpcGui::HandleEvent (iEvent &ev)
{
  if (paws->HandleEvent(ev))
    return true;

  if (ev.Name == csevFrame (object_reg))
  {
    if (drawScreen)
    {
      FrameLimit();
      g3d->BeginDraw(CSDRAW_2DGRAPHICS);
      paws->Draw();
    }
    else
    {
      csSleep(150);
    }

    g3d->FinishDraw ();
    g3d->Print (NULL);
    return true;
  }
  else if (ev.Name == csevCanvasHidden (object_reg, g2d))
  {
    drawScreen = false;
  }
  else if (ev.Name == csevCanvasExposed (object_reg, g2d))
  {
    drawScreen = true;
  }
  return false;
}

void NpcGui::FrameLimit()
{
  csTicks sleeptime;
  csTicks elapsedTime = csGetTicks() - elapsed;

  // Define sleeptime to limit fps to around 30
  sleeptime = 30;

  // Here we sacrifice drawing time
  if(elapsedTime < sleeptime)
    csSleep(sleeptime - elapsedTime);

  elapsed = csGetTicks();
}
