#include "util/genericevent.h"

class EventHandler;
class PawsManager;
class pawsMainWidget;
class psNPCClient;
struct iGraphics2D;
struct iGraphics3D;
struct iObjectRegistry;

class NpcGui
{
public:
  NpcGui(iObjectRegistry* object_reg, psNPCClient* npcclient);
  ~NpcGui();

  bool Initialise();

private:
  iObjectRegistry* object_reg;
  psNPCClient* npcclient;
  csRef<iGraphics3D> g3d;
  csRef<iGraphics2D> g2d;

  // PAWS
  PawsManager*    paws;
  pawsMainWidget* mainWidget;

  // Event handling.
  DeclareGenericEventHandler(EventHandler, NpcGui, "planeshift.launcher");
  csRef<EventHandler> event_handler;
  csRef<iEventQueue> queue;

  /* Handles an event from the event handler */
  bool HandleEvent (iEvent &ev);

  /* keeps track of whether the window is visible or not. */
  bool drawScreen;

  /* Limits the frame rate either by sleeping. */
  void FrameLimit();

  /* Time when the last frame was drawn. */
  csTicks elapsed;
};
