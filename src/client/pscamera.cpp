// TODO: change name of offsets to correspond to correct stuff


/*
* Author: Andrew Robberts
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
//=============================================================================
// Crystal Space Includes
//=============================================================================
#include <iengine/movable.h>
#include <iengine/camera.h>
#include <iengine/engine.h>
#include <cstool/csview.h>
#include <cstool/enginetools.h>
#include <ivideo/graph2d.h>
#include <ivideo/graph3d.h>
#include <csutil/flags.h>
#include <iengine/mesh.h>
#include <csutil/xmltiny.h>
#include <imesh/object.h>
#include <imesh/spritecal3d.h>
#include <cstool/collider.h>
#include <ivaria/collider.h>
#include <csqsqrt.h>
#include <csgeom/plane3.h>

//=============================================================================
// Project Includes
//=============================================================================
#include "iclient/ibgloader.h"

#include "engine/linmove.h"

#include "util/psstring.h"
#include "util/strutil.h"

#include "gui/psmainwidget.h"

#include "net/messages.h"
#include "net/cmdhandler.h"
#include "net/cmdhandler.h"

//=============================================================================
// Local Includes
//=============================================================================
#include "pscamera.h"
#include "pscelclient.h"
#include "pscharcontrol.h"
#include "globals.h"
#include "shadowmanager.h"


psCamera::psCamera()
        : psCmdBase(psengine->GetMsgHandler(), psengine->GetCmdHandler(), PawsManager::GetSingleton().GetObjectRegistry())
{
    view = csPtr<iView> (new csView(psengine->GetEngine(), psengine->GetG3D()));

    actor = NULL;

    vc =  csQueryRegistry<iVirtualClock> (psengine->GetObjectRegistry());
    cdsys =  csQueryRegistry<iCollideSystem> (psengine->GetObjectRegistry ());

    // most of these are default values that will be overwritten
    firstPersonPositionOffset = csVector3(0, 1, 0);
    thirdPersonPositionOffset = csVector3(0, 1, 3);

    camData[CAMERA_FIRST_PERSON].springCoef = 10.0f;
    camData[CAMERA_FIRST_PERSON].InertialDampeningCoef = 0.1f;
    camData[CAMERA_FIRST_PERSON].springLength = 0.01f;
    camData[CAMERA_FIRST_PERSON].maxDistance = 1.0f;
    camData[CAMERA_FIRST_PERSON].minDistance = 1.0f;
    camData[CAMERA_FIRST_PERSON].turnSpeed = 0.0f;
    camData[CAMERA_FIRST_PERSON].swingCoef = 0.0f;
    camData[CAMERA_FIRST_PERSON].defaultPitch = camData[CAMERA_FIRST_PERSON].pitch = 0.0f;
    camData[CAMERA_FIRST_PERSON].defaultRoll  = camData[CAMERA_FIRST_PERSON].roll  = 0.0f;
    camData[CAMERA_FIRST_PERSON].defaultYaw   = camData[CAMERA_FIRST_PERSON].yaw   = 0.0f;

    camData[CAMERA_THIRD_PERSON].springCoef = 3.5f;
    camData[CAMERA_THIRD_PERSON].InertialDampeningCoef = 0.25f;
    camData[CAMERA_THIRD_PERSON].springLength = 0.01f;
    camData[CAMERA_THIRD_PERSON].maxDistance = 15.0f;
    camData[CAMERA_THIRD_PERSON].minDistance = 1.0f;
    camData[CAMERA_THIRD_PERSON].turnSpeed = 0.0f;
    camData[CAMERA_THIRD_PERSON].swingCoef = 0.0f;
    camData[CAMERA_THIRD_PERSON].defaultPitch = camData[CAMERA_THIRD_PERSON].pitch = 0.0f;
    camData[CAMERA_THIRD_PERSON].defaultRoll  = camData[CAMERA_THIRD_PERSON].roll  = 0.0f;
    camData[CAMERA_THIRD_PERSON].defaultYaw   = camData[CAMERA_THIRD_PERSON].yaw   = 0.0f;

    camData[CAMERA_M64_THIRD_PERSON].springCoef = 3.5f;
    camData[CAMERA_M64_THIRD_PERSON].InertialDampeningCoef = 0.25f;
    camData[CAMERA_M64_THIRD_PERSON].springLength = 0.01f;
    camData[CAMERA_M64_THIRD_PERSON].minDistance = 2.0f;
    camData[CAMERA_M64_THIRD_PERSON].maxDistance = 6.0f;
    camData[CAMERA_M64_THIRD_PERSON].turnSpeed = 1.0f;
    camData[CAMERA_M64_THIRD_PERSON].swingCoef = 0.0f;
    camData[CAMERA_M64_THIRD_PERSON].defaultPitch = camData[CAMERA_M64_THIRD_PERSON].pitch = 0.0f;
    camData[CAMERA_M64_THIRD_PERSON].defaultRoll  = camData[CAMERA_M64_THIRD_PERSON].roll  = 0.0f;
    camData[CAMERA_M64_THIRD_PERSON].defaultYaw   = camData[CAMERA_M64_THIRD_PERSON].yaw   = 0.0f;

    camData[CAMERA_LARA_THIRD_PERSON].springCoef = 3.5f;
    camData[CAMERA_LARA_THIRD_PERSON].InertialDampeningCoef = 0.25f;
    camData[CAMERA_LARA_THIRD_PERSON].springLength = 0.01f;
    camData[CAMERA_LARA_THIRD_PERSON].minDistance = 2.0f;
    camData[CAMERA_LARA_THIRD_PERSON].maxDistance = 6.0f;
    camData[CAMERA_LARA_THIRD_PERSON].turnSpeed = 1.0f;
    camData[CAMERA_LARA_THIRD_PERSON].swingCoef = 0.7f;
    camData[CAMERA_LARA_THIRD_PERSON].defaultPitch = camData[CAMERA_LARA_THIRD_PERSON].pitch = 0.0f;
    camData[CAMERA_LARA_THIRD_PERSON].defaultRoll  = camData[CAMERA_LARA_THIRD_PERSON].roll  = 0.0f;
    camData[CAMERA_LARA_THIRD_PERSON].defaultYaw   = camData[CAMERA_LARA_THIRD_PERSON].yaw   = 0.0f;

    camData[CAMERA_FREE].springCoef = 3.5f;
    camData[CAMERA_FREE].InertialDampeningCoef = 0.25f;
    camData[CAMERA_FREE].springLength = 0.01f;
    camData[CAMERA_FREE].minDistance = 2.0f;
    camData[CAMERA_FREE].maxDistance = 16.0f;
    camData[CAMERA_FREE].turnSpeed = 0.0f;
    camData[CAMERA_FREE].swingCoef = 0.0f;
    camData[CAMERA_FREE].defaultPitch = camData[CAMERA_FREE].pitch = 0.0f;
    camData[CAMERA_FREE].defaultRoll  = camData[CAMERA_FREE].roll  = 0.0f;
    camData[CAMERA_FREE].defaultYaw   = camData[CAMERA_FREE].yaw   = 0.0f;

    camData[CAMERA_ACTUAL_DATA].springCoef = 0.0f;
    camData[CAMERA_ACTUAL_DATA].InertialDampeningCoef = 0.0;
    camData[CAMERA_ACTUAL_DATA].springLength = 0.01f;
    camData[CAMERA_ACTUAL_DATA].minDistance = 0.0f;
    camData[CAMERA_ACTUAL_DATA].maxDistance = 0.0f;
    camData[CAMERA_ACTUAL_DATA].turnSpeed = 0.0f;
    camData[CAMERA_ACTUAL_DATA].swingCoef = 0.0f;
    camData[CAMERA_ACTUAL_DATA].defaultPitch = camData[CAMERA_ACTUAL_DATA].pitch = 0.0f;
    camData[CAMERA_ACTUAL_DATA].defaultRoll  = camData[CAMERA_ACTUAL_DATA].roll  = 0.0f;
    camData[CAMERA_ACTUAL_DATA].defaultYaw   = camData[CAMERA_ACTUAL_DATA].yaw   = 0.0f;

    camData[CAMERA_LAST_ACTUAL].springCoef = 0.0f;
    camData[CAMERA_LAST_ACTUAL].InertialDampeningCoef = 0.0;
    camData[CAMERA_LAST_ACTUAL].springLength = 0.01f;
    camData[CAMERA_LAST_ACTUAL].minDistance = 0.0f;
    camData[CAMERA_LAST_ACTUAL].maxDistance = 0.0f;
    camData[CAMERA_LAST_ACTUAL].turnSpeed = 0.0f;
    camData[CAMERA_LAST_ACTUAL].swingCoef = 0.0f;
    camData[CAMERA_LAST_ACTUAL].defaultPitch = camData[CAMERA_LAST_ACTUAL].pitch = 0.0f;
    camData[CAMERA_LAST_ACTUAL].defaultRoll  = camData[CAMERA_LAST_ACTUAL].roll  = 0.0f;
    camData[CAMERA_LAST_ACTUAL].defaultYaw   = camData[CAMERA_LAST_ACTUAL].yaw   = 0.0f;

    camData[CAMERA_TRANSITION].springCoef = 3.5f;
    camData[CAMERA_TRANSITION].InertialDampeningCoef = 0.25f;
    camData[CAMERA_TRANSITION].springLength = 0.01f;
    camData[CAMERA_TRANSITION].minDistance = 1.0f;
    camData[CAMERA_TRANSITION].maxDistance = 16.0f;
    camData[CAMERA_TRANSITION].turnSpeed = 0.0f;
    camData[CAMERA_TRANSITION].swingCoef = 0.0f;
    camData[CAMERA_TRANSITION].defaultPitch = camData[CAMERA_TRANSITION].pitch = 0.0f;
    camData[CAMERA_TRANSITION].defaultRoll  = camData[CAMERA_TRANSITION].roll  = 0.0f;
    camData[CAMERA_TRANSITION].defaultYaw   = camData[CAMERA_TRANSITION].yaw   = 0.0f;

    camData[CAMERA_NPCTALK].springCoef = 3.5f;
    camData[CAMERA_NPCTALK].InertialDampeningCoef = 0.25f;
    camData[CAMERA_NPCTALK].springLength = 0.01f;
    camData[CAMERA_NPCTALK].maxDistance = 15.0f;
    camData[CAMERA_NPCTALK].minDistance = 0.50f;
    camData[CAMERA_NPCTALK].turnSpeed = 0.0f;
    camData[CAMERA_NPCTALK].swingCoef = 0.0f;
    camData[CAMERA_NPCTALK].defaultPitch = camData[CAMERA_NPCTALK].pitch = 0.0f;
    camData[CAMERA_NPCTALK].defaultRoll  = camData[CAMERA_NPCTALK].roll  = 0.0f;
    camData[CAMERA_NPCTALK].defaultYaw   = camData[CAMERA_NPCTALK].yaw   = 0.0f;

    camData[CAMERA_COLLISION].springCoef = 1.0f;
    camData[CAMERA_COLLISION].InertialDampeningCoef = 0.25f;
    camData[CAMERA_COLLISION].springLength = 0.01f;
    camData[CAMERA_COLLISION].minDistance = 2.0f;
    camData[CAMERA_COLLISION].maxDistance = 16.0f;
    camData[CAMERA_COLLISION].turnSpeed = 0.0f;
    camData[CAMERA_COLLISION].swingCoef = 0.0f;
    camData[CAMERA_COLLISION].defaultPitch = camData[CAMERA_COLLISION].pitch = 0.0f;
    camData[CAMERA_COLLISION].defaultRoll  = camData[CAMERA_COLLISION].roll  = 0.0f;
    camData[CAMERA_COLLISION].defaultYaw   = camData[CAMERA_COLLISION].yaw   = 0.0f;

    camData[CAMERA_ERR].springCoef = 0.0f;
    camData[CAMERA_ERR].InertialDampeningCoef = 0.0;
    camData[CAMERA_ERR].springLength = 0.01f;
    camData[CAMERA_ERR].minDistance = 0.0f;
    camData[CAMERA_ERR].maxDistance = 0.0f;
    camData[CAMERA_ERR].turnSpeed = 0.0f;
    camData[CAMERA_ERR].swingCoef = 0.0f;
    camData[CAMERA_ERR].defaultPitch = camData[CAMERA_ERR].pitch = 0.0f;
    camData[CAMERA_ERR].defaultRoll  = camData[CAMERA_ERR].roll  = 0.0f;
    camData[CAMERA_ERR].defaultYaw   = camData[CAMERA_ERR].yaw   = 0.0f;

    currCameraMode = CAMERA_ERR;

    transitionThresholdSquared = 1.0f;
    cameraHasBeenPositioned = false;
    hasCollision = false;

    distanceCfg.adaptive = false;
    distanceCfg.dist     = 200;
    distanceCfg.minFPS   = 20;
    distanceCfg.maxFPS   = 30;
    distanceCfg.minDist  = 30;

    fixedDistClip = 200;

    useCameraCD = false;
    useNPCCam = false;

    lastActorSector = 0;

    cameraInitialized = false;

    cmdsource->Subscribe("/tellnpc", this);
    cmdsource->Subscribe("/npcmenu", this);

    psengine->GetOptions()->RegisterOptionsClass("camera", this);

    LoadOptions();
}

psCamera::~psCamera()
{
    // save all the camera setting to the config file just once
    distanceCfg.dist = fixedDistClip;

    // reset the camera mode to the last active mode if the player logs out while talking to a npc
    if(currCameraMode == CAMERA_NPCTALK)
    {
        currCameraMode = lastCameraMode;
    }

    SaveToFile();
}

// scale is used to translate springyness option to actual camera values
const float CAM_MODE_SCALE[psCamera::CAMERA_MODES_COUNT] = {
    0.1f,   //CAMERA_FIRST_PERSON
    0.285f, //CAMERA_THIRD_PERSON
    0.285f, //CAMERA_M64_THIRD_PERSON
    0.285f, //CAMERA_LARA_THIRD_PERSON
    0.285f, //CAMERA_FREE
    0.0f,   //CAMERA_ACTUAL_DATA
    0.0f,   //CAMERA_LAST_ACTUAL
    0.285f, //CAMERA_TRANSITION
    0.285f, //CAMERA_NPCTALK
    1.0f,   //CAMERA_COLLISION
    0.0f    //CAMERA_ERR
};

const char * psCamera::HandleCommand(const char *cmd)
{
    WordArray words(cmd);

    GEMClientActor* target = dynamic_cast<GEMClientActor*>(psengine->GetCharManager()->GetTarget());

    if (words.GetCount() == 0 || !target || !useNPCCam || target == actor)
        return 0;

    // enable npc mode if the targeted npc is in talking distance and not a statue
    if(GetCameraMode() != CAMERA_NPCTALK && actor->RangeTo(target, false) < NPC_MODE_DISTANCE &&
       target->GetMode() != psModeMessage::STATUE)
    {
        //rotate the target so that it faces the player
        csVector3 playerPos = psengine->GetCelClient()->GetMainPlayer()->GetPosition();
        csVector3 targetPos = target->GetPosition();

        csVector3 diff = playerPos - targetPos;
        if (!diff.x)
            diff.x = 0.00001F; // div/0 protect

        float angle = -atan2(diff.x,-diff.z);
        if(angle < 0) angle += TWO_PI;
        
        float npcrot = target->GetRotation();
        
        
        float velNormSquared = target->GetVelocity().SquaredNorm();
                
        if((angle-npcrot) < TWO_PI && (angle-npcrot) > 0 && velNormSquared == 0)
        {
            npcOldRot = npcrot;
            // decide to turn clockwise or counterclockwise (angle < 180 degrees)
            vel = ((angle-npcrot) < PI) ? 3.0f : -3.0f;
            target->GetMovement()->SetAngularVelocity(-vel, csVector3(0.0f, angle, 0.0f));
        }

        npcModeTarget = target;
        npcModePosition = actor->GetMesh()->GetMovable()->GetFullPosition();
        SetCameraMode(CAMERA_NPCTALK);
    }

    return 0;
}


void psCamera::HandleMessage(MsgEntry *msg)
{
    return;
}

float psCamera::CalcSpringCoef(float springyness, float scale) const
{
    float s = springyness * scale;
    if (fabs(s) < 0.0001f)
        return 0.0f;

    return 1.0f / s;
}

float psCamera::CalcInertialDampeningCoef(float springyness, float scale) const
{
    float s = springyness * scale;
    return s * 0.25f;
}

float psCamera::EstimateSpringyness(size_t camMode) const
{
    float s = camData[camMode].InertialDampeningCoef * 4.0f;
    float scale = CAM_MODE_SCALE[camMode];
    if (fabs(scale) < 0.0001f)
        return 0.0f;
    return s / scale;
}

void psCamera::LoadOptions()
{
    psOptions * options = psengine->GetOptions();
    useCameraCD = options->GetOption("camera", "usecd", true);
    useNPCCam = options->GetOption("camera", "useNPCam", false);

    float springyness = options->GetOption("camera", "springyness", true);
    for (size_t a=0; a<CAMERA_MODES_COUNT; ++a)
    {
        camData[a].springCoef = CalcSpringCoef(springyness, CAM_MODE_SCALE[a]);
        camData[a].InertialDampeningCoef = CalcInertialDampeningCoef(springyness, CAM_MODE_SCALE[a]);
    }
}

void psCamera::SaveOptions()
{
    psOptions * options = psengine->GetOptions();
    options->SetOption("camera", "usecd", useCameraCD);
    options->SetOption("camera", "usenpccam", useNPCCam);
    options->SetOption("camera", "springyness", EstimateSpringyness(0));
}

bool psCamera::InitializeView(GEMClientActor* entity)
{
    actor = entity;

    lastCameraMode = CAMERA_ERR;
    SetCameraMode(CAMERA_FIRST_PERSON);
    SetPitch(0.0f);
    SetPitchVelocity(0.0f);
    SetYaw(0.0f);
    SetYawVelocity(0.0f);

    csVector3 pos;
    float yRot;
    iSector* sector;
    actor->GetLastPosition(pos, yRot, sector);

    view->GetCamera()->SetSector(sector);
    view->GetCamera()->GetTransform().SetOrigin(pos);

    int width = psengine->GetG2D()->GetWidth();
    int height = psengine->GetG2D()->GetHeight();

    view->SetRectangle(0, 0, width, height);
    view->GetPerspectiveCamera()->SetPerspectiveCenter(0.5, 0.5);

    view->SetContext(psengine->GetG3D());

    for (int i=0; i<CAMERA_MODES_COUNT; i++)
    {
        SetPosition(pos, i);
        SetTarget(pos, i);
        SetUp(csVector3(0, 1, 0), i);
        SetDistance(5.0f, i);
        SetPitch(0.0f, i);
        SetDefaultPitch(0.0f, i);
    }

    // the error has to be set to 0 to begin with
    SetPosition(csVector3(0,0,0), CAMERA_ERR);
    SetTarget(csVector3(0,0,0), CAMERA_ERR);
    SetUp(csVector3(0,0,0), CAMERA_ERR);

    // load settings from a file
    if (!LoadFromFile())
        Error1("Failed to load camera options");

    if (distanceCfg.adaptive)
        UseAdaptiveDistanceClipping(distanceCfg.minFPS, distanceCfg.maxFPS, distanceCfg.minDist);
    else
        UseFixedDistanceClipping(distanceCfg.dist);

    // grab race definition for camera offsets
    csString raceDef(psengine->GetCelClient()->GetMainPlayer()->race);
    race = psengine->GetCharManager()->GetCreation()->GetRace(raceDef.GetData());

    if (race)
    {
        // calculate new offsets based on the race data

        // note that the x coordinate is ignored for speed reasons, so the offset
        // can only be either directly behind or in front of the actor.
        firstPersonPositionOffset = race->FirstPos;
        thirdPersonPositionOffset = race->FollowPos;
    }

    return true;
}

bool psCamera::LoadFromFile(bool useDefault, bool overrideCurrent)
{
    csString fileName = "/planeshift/userdata/options/camera.xml";
    if (!psengine->GetVFS()->Exists(fileName) || useDefault)
    {
        fileName = "/planeshift/data/options/camera_def.xml";
    }

    csRef<iDocument> doc;
    csRef<iDocumentNode> root, camNode, settingNode;
    csRef<iDocumentNodeIterator> xmlbinds;
    csRef<iVFS> vfs;
    csRef<iDocumentSystem>  xml;
    const char* error;

    vfs = psengine->GetVFS();
    assert(vfs);
    csRef<iDataBuffer> buff = vfs->ReadFile(fileName);
    if (buff == NULL)
    {
        Error2("Could not find file: %s", fileName.GetData());
        return false;
    }
    xml = psengine->GetXMLParser ();
    doc = xml->CreateDocument();
    assert(doc);
    error = doc->Parse( buff );
    if ( error )
    {
        Error3("Parse error in %s: %s", fileName.GetData(), error);
        return false;
    }
    if (doc == NULL)
        return false;

    root = doc->GetRoot();
    if (root == NULL)
    {
        Error1("No root in XML");
        return false;
    }

    // general settings
    camNode = root->GetNode("General");
    if (camNode != NULL)
    {
        xmlbinds = camNode->GetNodes("camsetting");
        while (xmlbinds->HasNext())
        {
            settingNode = xmlbinds->Next();

            csString settingName = settingNode->GetAttributeValue("name");
            csString settingValue = settingNode->GetAttributeValue("value");
            if (settingName == "UseCollisionDetection")
                useCameraCD = (settingValue.Upcase() == "ON");
            if (settingName == "UseNPCCam")
                useNPCCam = (settingValue.Upcase() == "ON");
            else if (settingName == "TransitionThreshold")
            {
                transitionThresholdSquared = atof(settingValue.GetData());
                transitionThresholdSquared *= transitionThresholdSquared;
            }
            else if (settingName == "StartingCameraMode" && !cameraInitialized)
            {
                int mode = atoi(settingValue.GetDataSafe());
                if (mode < 0 || mode >= CAMERA_MODES_COUNT)
                    mode = CAMERA_THIRD_PERSON;
                SetCameraMode(mode);
            }
        }
    }

    camNode = root->GetNode("DistanceClipping");
    if (camNode != NULL)
    {
        xmlbinds = camNode->GetNodes("camsetting");
        while (xmlbinds->HasNext())
        {
            settingNode = xmlbinds->Next();

            csString settingName = settingNode->GetAttributeValue("name");
            csString settingValue = settingNode->GetAttributeValue("value");
            if (settingName == "Adaptive")
                distanceCfg.adaptive = (settingValue.Upcase() == "ON");
            else if (settingName == "Dist")
            {
                distanceCfg.dist = atoi(settingValue.GetData());
                fixedDistClip = distanceCfg.dist;
            }
            else if (settingName == "MinFPS")
                distanceCfg.minFPS = atoi(settingValue.GetData());
            else if (settingName == "MaxFPS")
                distanceCfg.maxFPS = atoi(settingValue.GetData());
            else if (settingName == "MinDist")
                distanceCfg.minDist = atoi(settingValue.GetData());
        }
    }

    // camera mode specific settings
    for (int c=0; c<CAMERA_MODES_COUNT; c++)
    {
        switch (c)
        {
            case CAMERA_FIRST_PERSON:
                camNode = root->GetNode("FirstPerson");
                break;
            case CAMERA_THIRD_PERSON:
                camNode = root->GetNode("ThirdPersonFollow");
                break;
            case CAMERA_M64_THIRD_PERSON:
                camNode = root->GetNode("ThirdPersonM64");
                break;
            case CAMERA_LARA_THIRD_PERSON:
                camNode = root->GetNode("ThirdPersonLara");
                break;
            case CAMERA_FREE:
                camNode = root->GetNode("FreeRotation");
                break;
            case CAMERA_ACTUAL_DATA:
                camNode = root->GetNode("CameraStart");
                break;
            case CAMERA_TRANSITION:
                camNode = root->GetNode("CameraTransition");
                break;
            case CAMERA_COLLISION:
                camNode = root->GetNode("CollisionBuffer");
        }
        if (camNode != NULL)
        {
            xmlbinds = camNode->GetNodes("camsetting");
            while (xmlbinds->HasNext())
            {
                settingNode = xmlbinds->Next();

                csString settingName = settingNode->GetAttributeValue("name");
                csString settingValue = settingNode->GetAttributeValue("value");
                if (settingName == "StartingPitch")
                {
                    camData[c].defaultPitch = atof(settingValue.GetData());
                    if(overrideCurrent)
                    {
                        camData[c].pitch = camData[c].defaultPitch;
                    }
                }
                else if (settingName == "StartingYaw")
                {
                    camData[c].defaultYaw = atof(settingValue.GetData());
                    if(overrideCurrent)
                    {
                        camData[c].yaw = camData[c].defaultYaw;
                    }
                }
                else if (settingName == "StartingRoll")
                {
                    camData[c].defaultRoll = atof(settingValue.GetData());
                    if(overrideCurrent)
                    {
                        camData[c].roll = camData[c].defaultRoll;
                    }
                }
                else if (settingName == "CameraDistance")
                {
                    if(overrideCurrent) //there isn't a default distance entry?
                    {
                        camData[c].distance = atof(settingValue.GetData());
                    }
                }
                else if (settingName == "MinCameraDistance")
                    camData[c].minDistance = atof(settingValue.GetData());
                else if (settingName == "MaxCameraDistance")
                    camData[c].maxDistance = atof(settingValue.GetData());
                else if (settingName == "TurningSpeed")
                    camData[c].turnSpeed = atof(settingValue.GetData());
                else if (settingName == "SpringCoefficient")
                    camData[c].springCoef = atof(settingValue.GetData());
                else if (settingName == "DampeningCoefficient")
                    camData[c].InertialDampeningCoef = atof(settingValue.GetData());
                else if (settingName == "SpringLength")
                    camData[c].springLength = atof(settingValue.GetData());
                else if (settingName == "SwingCoefficient")
                    camData[c].swingCoef = atof(settingValue.GetData());
            }
        }
    }

    cameraInitialized = true;
    return true;
}

bool psCamera::SaveToFile()
{
    if (!cameraInitialized)
        return false;  // We don't have any settings to save yet!

    csString xml;

    xml += "<General>\n";
    xml += "    <camsetting name=\"StartingCameraMode\" value=\"";      xml += GetCameraMode();                             xml += "\" />\n";
    xml += "    <camsetting name=\"UseCollisionDetection\" value=\"";   xml += (CheckCameraCD() ? "on" : "off");            xml += "\" />\n";
    xml += "    <camsetting name=\"UseNPCCam\" value=\"";               xml += (GetUseNPCCam() ? "on" : "off");            xml += "\" />\n";
    xml += "    <camsetting name=\"TransitionThreshold\" value=\"";     xml += GetTransitionThreshold();                    xml += "\" />\n";
    xml += "</General>\n";

    xml += "<CameraTransition>\n";
    xml += "    <camsetting name=\"SpringLength\" value=\"";            xml += GetSpringLength(CAMERA_TRANSITION);          xml += "\" />\n";
    xml += "    <camsetting name=\"SpringCoefficient\" value=\"";       xml += GetSpringCoef(CAMERA_TRANSITION);            xml += "\" />\n";
    xml += "    <camsetting name=\"DampeningCoefficient\" value=\"";    xml += GetDampeningCoef(CAMERA_TRANSITION);         xml += "\" />\n";
    xml += "</CameraTransition>\n";

    xml += "<CollisionBuffer>\n";
    xml += "    <camsetting name=\"SpringLength\" value=\"";            xml += GetSpringLength(CAMERA_COLLISION);          xml += "\" />\n";
    xml += "    <camsetting name=\"SpringCoefficient\" value=\"";       xml += GetSpringCoef(CAMERA_COLLISION);            xml += "\" />\n";
    xml += "    <camsetting name=\"DampeningCoefficient\" value=\"";    xml += GetDampeningCoef(CAMERA_COLLISION);         xml += "\" />\n";
    xml += "</CollisionBuffer>\n";

    xml += "<FirstPerson>\n";
    xml += "    <camsetting name=\"SpringLength\" value=\"";            xml += GetSpringLength(CAMERA_FIRST_PERSON);        xml += "\" />\n";
    xml += "    <camsetting name=\"SpringCoefficient\" value=\"";       xml += GetSpringCoef(CAMERA_FIRST_PERSON);          xml += "\" />\n";
    xml += "    <camsetting name=\"DampeningCoefficient\" value=\"";    xml += GetDampeningCoef(CAMERA_FIRST_PERSON);       xml += "\" />\n";
    xml += "    <camsetting name=\"StartingPitch\" value=\"";           xml += GetDefaultPitch(CAMERA_FIRST_PERSON);        xml += "\" />\n";
    xml += "</FirstPerson>\n";

    xml += "<ThirdPersonFollow>\n";
    xml += "    <camsetting name=\"SpringLength\" value=\"";            xml += GetSpringLength(CAMERA_THIRD_PERSON);        xml += "\" />\n";
    xml += "    <camsetting name=\"SpringCoefficient\" value=\"";       xml += GetSpringCoef(CAMERA_THIRD_PERSON);          xml += "\" />\n";
    xml += "    <camsetting name=\"DampeningCoefficient\" value=\"";    xml += GetDampeningCoef(CAMERA_THIRD_PERSON);       xml += "\" />\n";
    xml += "    <camsetting name=\"CameraDistance\" value=\"";          xml += GetDistance(CAMERA_THIRD_PERSON);            xml += "\" />\n";
    xml += "    <camsetting name=\"StartingPitch\" value=\"";           xml += GetDefaultPitch(CAMERA_THIRD_PERSON);        xml += "\" />\n";
    xml += "</ThirdPersonFollow>\n";

    xml += "<ThirdPersonM64>\n";
    xml += "    <camsetting name=\"SpringLength\" value=\"";            xml += GetSpringLength(CAMERA_M64_THIRD_PERSON);    xml += "\" />\n";
    xml += "    <camsetting name=\"SpringCoefficient\" value=\"";       xml += GetSpringCoef(CAMERA_M64_THIRD_PERSON);      xml += "\" />\n";
    xml += "    <camsetting name=\"DampeningCoefficient\" value=\"";    xml += GetDampeningCoef(CAMERA_M64_THIRD_PERSON);   xml += "\" />\n";
    xml += "    <camsetting name=\"MaxCameraDistance\" value=\"";       xml += GetMaxDistance(CAMERA_M64_THIRD_PERSON);     xml += "\" />\n";
    xml += "    <camsetting name=\"MinCameraDistance\" value=\"";       xml += GetMinDistance(CAMERA_M64_THIRD_PERSON);     xml += "\" />\n";
    xml += "    <camsetting name=\"StartingPitch\" value=\"";           xml += GetDefaultPitch(CAMERA_M64_THIRD_PERSON);    xml += "\" />\n";
    xml += "    <camsetting name=\"TurningSpeed\" value=\"";            xml += GetTurnSpeed(CAMERA_M64_THIRD_PERSON);       xml += "\" />\n";
    xml += "</ThirdPersonM64>\n";

    xml += "<ThirdPersonLara>\n";
    xml += "    <camsetting name=\"SpringLength\" value=\"";            xml += GetSpringLength(CAMERA_LARA_THIRD_PERSON);   xml += "\" />\n";
    xml += "    <camsetting name=\"SpringCoefficient\" value=\"";       xml += GetSpringCoef(CAMERA_LARA_THIRD_PERSON);     xml += "\" />\n";
    xml += "    <camsetting name=\"DampeningCoefficient\" value=\"";    xml += GetDampeningCoef(CAMERA_LARA_THIRD_PERSON);  xml += "\" />\n";
    xml += "    <camsetting name=\"MaxCameraDistance\" value=\"";       xml += GetMaxDistance(CAMERA_LARA_THIRD_PERSON);    xml += "\" />\n";
    xml += "    <camsetting name=\"MinCameraDistance\" value=\"";       xml += GetMinDistance(CAMERA_LARA_THIRD_PERSON);    xml += "\" />\n";
    xml += "    <camsetting name=\"StartingPitch\" value=\"";           xml += GetDefaultPitch(CAMERA_LARA_THIRD_PERSON);   xml += "\" />\n";
    xml += "    <camsetting name=\"TurningSpeed\" value=\"";            xml += GetTurnSpeed(CAMERA_LARA_THIRD_PERSON);      xml += "\" />\n";
    xml += "    <camsetting name=\"SwingCoefficient\" value=\"";        xml += GetSwingCoef(CAMERA_LARA_THIRD_PERSON);      xml += "\" />\n";
    xml += "</ThirdPersonLara>\n";

    xml += "<FreeRotation>\n";
    xml += "    <camsetting name=\"SpringLength\" value=\"";            xml += GetSpringLength(CAMERA_FREE);                xml += "\" />\n";
    xml += "    <camsetting name=\"SpringCoefficient\" value=\"";       xml += GetSpringCoef(CAMERA_FREE);                  xml += "\" />\n";
    xml += "    <camsetting name=\"DampeningCoefficient\" value=\"";    xml += GetDampeningCoef(CAMERA_FREE);               xml += "\" />\n";
    xml += "    <camsetting name=\"MaxCameraDistance\" value=\"";       xml += GetMaxDistance(CAMERA_FREE);                 xml += "\" />\n";
    xml += "    <camsetting name=\"MinCameraDistance\" value=\"";       xml += GetMinDistance(CAMERA_FREE);                 xml += "\" />\n";
    xml += "    <camsetting name=\"StartingPitch\" value=\"";           xml += GetDefaultPitch(CAMERA_FREE);                xml += "\" />\n";
    xml += "</FreeRotation>\n";

    xml += "<DistanceClipping>\n";
    xml += "    <camsetting name=\"Adaptive\" value=\"";                xml += distanceCfg.adaptive ? "on" : "off";         xml += "\" />\n";
    xml += "    <camsetting name=\"Dist\" value=\"";                    xml += distanceCfg.dist;                            xml += "\" />\n";
    xml += "    <camsetting name=\"MinFPS\" value=\"";                  xml += distanceCfg.minFPS;                          xml += "\" />\n";
    xml += "    <camsetting name=\"MaxFPS\" value=\"";                  xml += distanceCfg.maxFPS;                          xml += "\" />\n";
    xml += "    <camsetting name=\"MinDist\" value=\"";                 xml += distanceCfg.minDist;                         xml += "\" />\n";
    xml += "</DistanceClipping>\n";

    fixedDistClip = distanceCfg.dist;

    return psengine->GetVFS()->WriteFile("/planeshift/userdata/options/camera.xml", xml.GetData(), xml.Length());
}


bool psCamera::Draw()
{
    if (!actor)  // Not ready yet
        return false;

    AdaptDistanceClipping();

    // calculate the elapsed time between this frame and the last one
    csTicks elapsedTicks = vc->GetElapsedTicks();
    float elapsedSeconds = elapsedTicks / 1000.0f;

    // perform the velocity calculations
    MovePitch(pitchVelocity * elapsedSeconds);
    MoveYaw(yawVelocity * elapsedSeconds);

    // get actor info
    csVector3 actorPos;
    float actorYRot;
    iSector* actorSector;
    actor->GetLastPosition(actorPos, actorYRot, actorSector);
    actorYRot = SaturateAngle(actorYRot);

    // decide whether the current camera is elastic or not
    bool isElastic = true;

    // store the previous frame's ideal camera data (it will be compared against curr frame later to become the delta part)
    CameraData deltaIdeal;
    if (isElastic)
    {
        deltaIdeal.worldPos = GetPosition();
        deltaIdeal.worldTar = GetTarget();
        deltaIdeal.worldUp = GetUp();
    }

    // calculate the eye position of the actor according to his eye offset (defined by the race definition)
    csVector3 actorEye = actorPos;
    if(actor->GetMode() == psModeMessage::SIT)
    {
        actorEye += csVector3(sinf(actorYRot)*firstPersonPositionOffset.z,
                              firstPersonPositionOffset.y*0.5f,
                              cosf(actorYRot)*firstPersonPositionOffset.z);
    }
    else
    {
        actorEye += csVector3(sinf(actorYRot)*firstPersonPositionOffset.z,
                                                   firstPersonPositionOffset.y,
                                                   cosf(actorYRot)*firstPersonPositionOffset.z);
    }


    // calculate ideal camera data (won't affect the actual camera data yet)
    DoCameraIdealCalcs(elapsedTicks, actorPos, actorEye, actorYRot);

    if (!cameraHasBeenPositioned)
    {
        // this will only get called once to ensure that the camera starts off in an appropriate place
        cameraHasBeenPositioned = true;
        ResetActualCameraData();
    }

    // transition phase calculations
    DoCameraTransition();

    // this makes the deltaIdeal data true to it's delta wording by subtracting the current ideal data
    if (isElastic)
    {
        deltaIdeal.worldPos -= GetPosition();
        deltaIdeal.worldTar -= GetTarget();
        deltaIdeal.worldUp -= GetUp();
    }

    // interpolate between ideal and actual camera data
    DoElasticPhysics(isElastic, elapsedTicks, deltaIdeal, actorSector);

    EnsureActorVisibility();

    // tell CS to render the scene
    if (!psengine->GetG3D()->BeginDraw( psengine->GetEngine()->GetBeginDrawFlags() | CSDRAW_3DGRAPHICS))
        return false;

    // if the actor changed sectors, then ensure that the position is correct (in case it's a warping portal)
    if (actorSector != lastActorSector && lastActorSector != 0)
    {
        // calculate the new actual data using relative error from last frame
        SetPosition(GetPosition() + GetPosition(CAMERA_ERR), CAMERA_ACTUAL_DATA);
        SetTarget(GetTarget() + GetTarget(CAMERA_ERR), CAMERA_ACTUAL_DATA);
        SetUp(GetUp() + GetUp(CAMERA_ERR), CAMERA_ACTUAL_DATA);
    }

    // assume the normal camera movement is good, and move the camera
    view->GetCamera()->SetSector(actorSector);
    view->GetCamera()->GetTransform().SetOrigin(actorPos + csVector3(0,1,0));
    view->GetCamera()->OnlyPortals(true);
    view->GetCamera()->GetTransform().LookAt(GetTarget(CAMERA_ACTUAL_DATA) - GetPosition(CAMERA_ACTUAL_DATA), GetUp(CAMERA_ACTUAL_DATA));
    view->GetCamera()->MoveWorld(GetPosition(CAMERA_ACTUAL_DATA) - view->GetCamera()->GetTransform().GetOrigin());

    view->Draw();

    // calculate the error of the camera
    SetPosition(GetPosition(CAMERA_ACTUAL_DATA) - GetPosition(), CAMERA_ERR);
    SetTarget(GetTarget(CAMERA_ACTUAL_DATA) - GetTarget(), CAMERA_ERR);
    SetUp(GetUp(CAMERA_ACTUAL_DATA) - GetUp(), CAMERA_ERR);

    lastActorSector = actorSector;

    // not needed at this point
    //CloneCameraModeData(CAMERA_ACTUAL_DATA, CAMERA_LAST_ACTUAL);
    return true;
}

void psCamera::SetPosition(const csVector3& pos, int mode)
{
    if (mode < 0) mode = currCameraMode;
    camData[mode].worldPos = pos;
}

csVector3 psCamera::GetPosition(int mode) const
{
    if (mode < 0) mode = currCameraMode;
    return camData[mode].worldPos;
}

void psCamera::SetTarget(const csVector3& tar, int mode)
{
    if (mode < 0) mode = currCameraMode;
    camData[mode].worldTar = tar;
}

csVector3 psCamera::GetTarget(int mode) const
{
    if (mode < 0) mode = currCameraMode;
    return camData[mode].worldTar;
}

void psCamera::SetUp(const csVector3& up, int mode)
{
    if (mode < 0) mode = currCameraMode;
    camData[mode].worldUp = up;
    camData[mode].worldUp.Normalize();
}

csVector3 psCamera::GetUp(int mode) const
{
    if (mode < 0) mode = currCameraMode;
    return camData[mode].worldUp;
}

void psCamera::NextCameraMode()
{
    if (GetCameraMode()+1 >= CAMERA_ACTUAL_DATA)
        SetCameraMode(0);
    else
        SetCameraMode(GetCameraMode()+1);

    // display new mode onscreen
    psengine->GetMainWidget()->ClearFadingText();
    psSystemMessage mode(0,MSG_ACK,PawsManager::GetSingleton().Translate(GetCameraModeVerbose()));
    mode.FireEvent();
}

void psCamera::SetCameraMode(int mode)
{
//  if (lastCameraMode == currCameraMode)
//      return;

    lastCameraMode = currCameraMode;
    currCameraMode = mode;

    // reset camera velocities
    SetYawVelocity(0.0f);
    SetPitchVelocity(0.0f);

    // get actor info
    csVector3 actorPos;
    float actorYRot;
    iSector* actorSector;
    actor->GetLastPosition(actorPos, actorYRot, actorSector);

    PawsManager::GetSingleton().GetMouse()->WantCrosshair(false);
    switch (mode)
    {
        case CAMERA_FIRST_PERSON:
            if (psengine->GetCharControl() && psengine->GetCharControl()->GetMovementManager()->MouseLook())
                PawsManager::GetSingleton().GetMouse()->WantCrosshair();
            break;
        case CAMERA_THIRD_PERSON:
            SetPosition(actorPos + csVector3(sinf(actorYRot)*thirdPersonPositionOffset.z,
                                            thirdPersonPositionOffset.y,
                                            cosf(actorYRot)*thirdPersonPositionOffset.z));
            SetYaw(actorYRot);
            break;
        case CAMERA_M64_THIRD_PERSON:
            //SetYaw(actorYRot);
            SetYaw(GetYaw(lastCameraMode));
            SetDistance(GetMaxDistance());
            EnsureCameraDistance();
            CalculatePositionFromYawPitchRoll();
            break;
        case CAMERA_LARA_THIRD_PERSON:
            //SetYaw(actorYRot);
            SetYaw(GetYaw(lastCameraMode));
            SetDistance(GetMaxDistance());
            EnsureCameraDistance();
            CalculatePositionFromYawPitchRoll();
            break;

        case CAMERA_FREE:
            //SetYaw(actorYRot);
            SetYaw(GetYaw(lastCameraMode));
            break;

        case CAMERA_NPCTALK:
            SetYaw(0);
            SetPitch(0.1f);
            SetDistance(camData[CAMERA_NPCTALK].minDistance);
            break;
    }

    // enable transition phase
    inTransitionPhase = true;
    actor->GetMesh()->SetFlagsRecursive(CS_ENTITY_INVISIBLE, 0);
}

int psCamera::GetCameraMode() const
{
    return currCameraMode;
}

csString psCamera::GetCameraModeVerbose() const
{
    switch (currCameraMode)
    {
        case CAMERA_FIRST_PERSON:
            return csString("First Person");
        case CAMERA_THIRD_PERSON:
            return csString("Third Person Follow");
        case CAMERA_M64_THIRD_PERSON:
            return csString("Free Movement");
        case CAMERA_LARA_THIRD_PERSON:
            return csString("Dynamic Follow");
        case CAMERA_FREE:
            return csString("Free Rotation");
    }
    return csString("Unknown");
}

iPerspectiveCamera *psCamera::GetICamera()
{
    return view->GetPerspectiveCamera();
}

iView *psCamera::GetView()
{
    return view;
}

iMeshWrapper *psCamera::Get3DPointFrom2D(int x, int y, csVector3 * worldCoord, csVector3 * untransfCoord)
{
    if (!GetICamera())
        return NULL;

    csVector3 vc, vo, vw;
    csVector2 perspective( x, GetICamera()->GetShiftY() * psengine->GetG2D()->GetHeight() * 2 - y );
    vc = GetICamera()->GetCamera()->InvPerspective( perspective, 1 );
    vw = GetICamera()->GetCamera()->GetTransform().This2Other( vc );

    iSector* sector = GetICamera()->GetCamera()->GetSector();

    if ( sector )
    {
        vo = GetICamera()->GetCamera()->GetTransform().GetO2TTranslation();
        csVector3 isect;
        csVector3 end = vo + (vw-vo)*1000;
        csIntersectingTriangle closest_tri;
        iMeshWrapper* sel = NULL;
        float dist = csColliderHelper::TraceBeam (cdsys, sector,
                           vo, end, true, closest_tri, isect, &sel);
        if (dist < 0)
            return NULL;

        if (worldCoord != 0)
            *worldCoord = isect;
        if (untransfCoord)
            *untransfCoord = vo + (vw-vo).Unit()*csQsqrt(dist);
        return sel;
    }
    return NULL;
}

iMeshWrapper* psCamera::FindMeshUnder2D(int x, int y, csVector3 *pos, int *poly)
{
    if (!GetICamera() || !GetICamera()->GetCamera()->GetSector())
        return NULL;

    // RS: when in mouselook mode, do not use the mouse x,y coords ... the
    // mouse is pinned to the center of the screen in this case, and the mesh
    // at this position will be the player
    if (psengine->GetCharControl() &&
        psengine->GetCharControl()->GetMovementManager()->MouseLook() &&
        currCameraMode != CAMERA_FIRST_PERSON )
    {
        csVector3 actorPos;
        csVector3 targetPos;
        csVector3 isect;
        float actorYRot;
        iSector* sector;

        psCelClient * cel = psengine->GetCelClient();
        if(!cel)
        {
            return 0;
        }

        GEMClientObject* myEntity = cel->GetMainPlayer();
        if(!myEntity)
        {
            return 0;
        }

        actor->GetLastPosition(actorPos, actorYRot, sector);
        //printf("actor %f %f %f  rot %f\n", actorPos.x, actorPos.y, actorPos.z, actorYRot);

        float optRange = 1000000.0;
        csRef<iMeshWrapper> bestMesh = 0;
        csVector3 bestPos;

        float s = sin(actorYRot);
        float c = cos(actorYRot);

        csArray<GEMClientObject*> entities = cel->FindNearbyEntities(sector, actorPos, RANGE_TO_SELECT*2);

        size_t entityCount = entities.GetSize();

        for (size_t i = 0; i < entityCount; ++i)
        {
            GEMClientObject* entity = entities[i];

            if ( (entity == myEntity) || !entity)
            {
                continue;
            }

            csRef<iMeshWrapper> mesh = entity->GetMesh();
            csVector3 objPos = mesh->GetMovable()->GetPosition();

            //printf("object %s etype %d pos %f %f %f\n", object->GetName(), eType, objPos.x, objPos.y, objPos.z);

            float dx = objPos.x - actorPos.x;
            float dz = objPos.z - actorPos.z;
            float a = 0; float b = 0;

            /* check if the object is in the view cone of the player by solving
             * actorPos + a * viewvec + b * normalvec = objectPos and verify a > b
             * for a 45 degree angle. Ignore height for now.
             * a * s + b * c + dx = 0;  ||  a * c + b * s + dz = 0;
             */
            if( s == 0.0 )
            {
                a = (-dz) / c; b = (-dx) / c;
            }
            else if( c == 0.0 )
            {
                a = (-dx) / s; b = (-dz) / s;
            }
            else
            {
                a = (dx / c - dz / s) / (c/s - s/c);
                b = (dx / s - dz / c) / (s/c - c/s);
            }
            //printf("a: %f   b: %f\n", a, b);

            float score = fabs(b) + fabs(a);

            // outside the view cone gets a bad score
            if( fabs(b) > fabs(a) )
            {
                 score = score + 10;
            }
            // and even worse at the back of the player
            if( a < 0.0 )
            {
                 score = score + 30;
            }

            if( score < optRange)
            {
                bestMesh = mesh;
                bestPos = objPos;
                optRange = score;
            }
        }
        if ( pos != NULL && bestMesh )
        {
            *pos = bestPos;
        }
        return (bestMesh);
    }
    else
    {
        csScreenTargetResult result = csEngineTools::FindScreenTarget(csVector2(x,y), 100.0f, GetICamera()->GetCamera());
        if(pos != NULL)
        {
            *pos = result.isect;
        }
        return result.mesh;
    }
    return NULL;
}

void psCamera::SetPitch(float pitch, int mode)
{
    if (mode < 0) mode = currCameraMode;

    if (pitch > PI/2.1f)
        pitch = PI/2.1f;
    if (pitch < -PI/2.1f)
        pitch = -PI/2.1f;

    camData[mode].pitch = pitch;
}

void psCamera::MovePitch(float deltaPitch, int mode)
{
    if (mode < 0) mode = currCameraMode;
    SetPitch(GetPitch(mode) + deltaPitch, mode);
}

float psCamera::GetPitch(int mode) const
{
    if (mode < 0) mode = currCameraMode;
    return camData[mode].pitch;
}

int psCamera::GetLastCameraMode()
{
    return lastCameraMode;
}

void psCamera::SetPitchVelocity(float pitchVel)
{
    pitchVelocity = pitchVel;
}

float psCamera::GetPitchVelocity() const
{
    return pitchVelocity;
}

void psCamera::SetYaw(float yaw, int mode)
{
    if (mode < 0) mode = currCameraMode;
    camData[mode].yaw = SaturateAngle(yaw);
}

void psCamera::MoveYaw(float deltaYaw, int mode)
{
    if (mode < 0) mode = currCameraMode;
    SetYaw(GetYaw(mode) + deltaYaw, mode);
}

float psCamera::GetYaw(int mode) const
{
    if (mode < 0) mode = currCameraMode;
    return camData[mode].yaw;
}

void psCamera::SetYawVelocity(float yawVel)
{
    yawVelocity = yawVel;
}

float psCamera::GetYawVelocity() const
{
    return yawVelocity;
}

void psCamera::SetDistance(float distance, int mode)
{
    if (mode < 0) mode = currCameraMode;
    if (distance > camData[mode].maxDistance)
        distance = camData[mode].maxDistance;
    else if (distance < camData[mode].minDistance)
        distance = camData[mode].minDistance;
    camData[mode].distance = distance;
}

void psCamera::MoveDistance(float deltaDistance, int mode)
{
    SetDistance(GetDistance(mode) + deltaDistance, mode);
}

float psCamera::GetDistance(int mode) const
{
    if (mode < 0) mode = currCameraMode;
    return camData[mode].distance;
}

void psCamera::ResetActualCameraData()
{
    SetPosition(GetPosition(), CAMERA_ACTUAL_DATA);
    SetTarget(GetTarget(), CAMERA_ACTUAL_DATA);
    SetUp(GetUp(), CAMERA_ACTUAL_DATA);
}

void psCamera::ResetCameraPositioning()
{
    cameraHasBeenPositioned = false;
}

bool psCamera::RotateCameraWithPlayer() const
{
    // it's easier to specify by what modes don't move with the camera
    return !(GetCameraMode() == CAMERA_M64_THIRD_PERSON ||
             GetCameraMode() == CAMERA_LARA_THIRD_PERSON ||
             GetCameraMode() == CAMERA_FREE ||
             GetCameraMode() == CAMERA_NPCTALK);
}

csVector3 psCamera::GetForwardVector(int mode) const
{
    if (mode < 0) mode = currCameraMode;
    csVector3 dir = GetTarget(mode) - GetPosition(mode);
    dir.Normalize();
    return dir;
}

csVector3 psCamera::GetRightVector(int mode) const
{
    if (mode < 0) mode = currCameraMode;
    csVector3 dir = GetTarget(mode) - GetPosition(mode);
    csVector3 right = GetUp(mode) % dir;
    right.Normalize();
    return right;
}

float psCamera::GetMinDistance(int mode) const
{
    if (mode < 0) mode = currCameraMode;
    return camData[mode].minDistance;
}

void psCamera::SetMinDistance(float dist, int mode)
{
    if (mode < 0) mode = currCameraMode;
    camData[mode].minDistance = dist;
}

float psCamera::GetMaxDistance(int mode) const
{
    if (mode < 0) mode = currCameraMode;
    return camData[mode].maxDistance;
}

void psCamera::SetMaxDistance(float dist, int mode)
{
    if (mode < 0) mode = currCameraMode;
    camData[mode].maxDistance = dist;
}

float psCamera::GetTurnSpeed(int mode) const
{
    if (mode < 0) mode = currCameraMode;
    return camData[mode].turnSpeed;
}

void psCamera::SetTurnSpeed(float speed, int mode)
{
    if (mode < 0) mode = currCameraMode;
    camData[mode].turnSpeed = speed;
}

float psCamera::GetSpringCoef(int mode) const
{
    if (mode < 0) mode = currCameraMode;
    return camData[mode].springCoef;
}

void psCamera::SetSpringCoef(float coef, int mode)
{
    if (mode < 0) mode = currCameraMode;
    camData[mode].springCoef = coef;
}

float psCamera::GetDampeningCoef(int mode) const
{
    if (mode < 0) mode = currCameraMode;
    return camData[mode].InertialDampeningCoef;
}

void psCamera::SetDampeningCoef(float coef, int mode)
{
    if (mode < 0) mode = currCameraMode;
    camData[mode].InertialDampeningCoef = coef;
}

float psCamera::GetSpringLength(int mode) const
{
    if (mode < 0) mode = currCameraMode;
    return camData[mode].springLength;
}

void psCamera::SetSpringLength(float length, int mode)
{
    if (mode < 0) mode = currCameraMode;
    camData[mode].springLength = length;
}

bool psCamera::CheckCameraCD() const
{
    return useCameraCD;
}

void psCamera::SetCameraCD(bool useCD)
{
    useCameraCD = useCD;
}

void psCamera::SetUseNPCCam(bool useNPCCam)
{
    this->useNPCCam = useNPCCam;
}

bool psCamera::GetUseNPCCam()
{
    return useNPCCam;
}

float psCamera::GetTransitionThreshold() const
{
    return sqrt(transitionThresholdSquared);
}

void psCamera::SetTransitionThreshold(float threshold)
{
    transitionThresholdSquared = threshold*threshold;
}

float psCamera::GetDefaultPitch(int mode) const
{
    if (mode < 0) mode = currCameraMode;
    return camData[mode].defaultPitch;
}

void psCamera::SetDefaultPitch(float pitch, int mode)
{
    if (mode < 0) mode = currCameraMode;
    camData[mode].defaultPitch = SaturateAngle(pitch);
}

float psCamera::GetDefaultYaw(int mode) const
{
    if (mode < 0) mode = currCameraMode;
    return camData[mode].defaultYaw;
}

void psCamera::SetDefaultYaw(float yaw, int mode)
{
    if (mode < 0) mode = currCameraMode;
    camData[mode].defaultYaw = SaturateAngle(yaw);
}

float psCamera::GetSwingCoef(int mode) const
{
    if (mode < 0) mode = currCameraMode;
    return camData[mode].swingCoef;
}

void psCamera::SetSwingCoef(float swingCoef, int mode)
{
    if (mode < 0) mode = currCameraMode;
    camData[mode].swingCoef = swingCoef;
}

void psCamera::UseFixedDistanceClipping(float dist)
{
    distanceCfg.adaptive = false;
    distanceCfg.dist = (int)dist;
    SetDistanceClipping(dist);
}

void psCamera::UseAdaptiveDistanceClipping(int minFPS, int maxFPS, int minDist)
{
    distanceCfg.adaptive = true;
    distanceCfg.minFPS = minFPS;
    distanceCfg.maxFPS = maxFPS;
    distanceCfg.minDist = minDist;
    if (GetDistanceClipping() < minDist)
        SetDistanceClipping(minDist);
}

psCamera::DistanceCfg psCamera::GetDistanceCfg()
{
    return distanceCfg;
}

void psCamera::SetDistanceCfg(psCamera::DistanceCfg newcfg)
{
    distanceCfg = newcfg;
}

void psCamera::DoCameraIdealCalcs(const csTicks elapsedTicks, const csVector3& actorPos, const csVector3& actorEye, const float actorYRot)
{
    csVector3 velocity;
    velocity = actor->GetVelocity();
    float velNormSquared = velocity.SquaredNorm();

    switch (GetCameraMode())
    {
        case CAMERA_FIRST_PERSON:
            SetDistance(1);
            SetPosition(actorEye);
            SetYaw(actorYRot);
            CalculateFromYawPitchRoll();
            SetPosition(GetPosition() + (GetPosition()-GetTarget())*0.1f);
            break;
        case CAMERA_THIRD_PERSON:
            SetTarget(actorEye);
            SetYaw(actorYRot);
            CalculatePositionFromYawPitchRoll();
            break;
        case CAMERA_M64_THIRD_PERSON:
            SetTarget(actorEye);

            SetDistance((GetTarget()-GetPosition()).Norm());
            EnsureCameraDistance();
            CalculatePositionFromYawPitchRoll();
            break;
        case CAMERA_LARA_THIRD_PERSON:
            SetTarget(actorEye);
            if (velNormSquared > 0.01f)
            {
                // when the player is running (only), a new position is interpolated
                // so with this camera mode, two springs are modelled, but the second
                // spring is ignored unless the actor is moving

                // calculate where the camera would be if there weren't a swing effect
                csVector3 newIdealPos = actorPos + csVector3(sinf(actorYRot)*thirdPersonPositionOffset.z,
                                                             thirdPersonPositionOffset.y,
                                                             cosf(actorYRot)*thirdPersonPositionOffset.z);

                // interpolate to the new calculated position
                SetPosition(CalcElasticPos(GetPosition(), newIdealPos, 0, (float)elapsedTicks/1000.0f, GetSwingCoef(), 0.0f, GetSpringLength()));
                SetYaw(CalculateNewYaw(GetTarget()-GetPosition()));
            }

            // ensure valid distance
            SetDistance((GetTarget()-GetPosition()).Norm());
            EnsureCameraDistance();

            // this allows pitch to work
            // note that this doesn't really use the yaw calculation,
            // because whenever the position is modified (above), a new yaw that
            // represents the (position - target) vector is calculated. This ensures
            // that this function won't change the yaw at all and only handle
            // the pitch.
            CalculatePositionFromYawPitchRoll();
            break;
        case CAMERA_FREE:
            SetTarget(actorEye);
            EnsureCameraDistance();
            CalculatePositionFromYawPitchRoll();
            break;
        case CAMERA_NPCTALK:
        {
            if (!npcModeTarget || !actor)
                break;

            psClientCharManager* clientChar = psengine->GetCharManager();
            // Check that we still have the same npc targeted, and that we did not move
            if (npcModeTarget != clientChar->GetTarget() || npcModePosition != actor->GetMesh()->GetMovable()->GetFullPosition())
            {
                npcModeTarget->GetMovement()->SetAngularVelocity(vel, csVector3(0.0f, npcOldRot, 0.0f));
                SetCameraMode(lastCameraMode);
                break;
            }

            csVector3 targetPos = npcModeTarget->GetMesh()->GetMovable()->GetFullPosition();
            targetPos.y = npcModeTarget->GetMesh()->GetWorldBoundingBox().MaxY();
            csVector3 charPos = actorPos;
            charPos.y = actor->GetMesh()->GetWorldBoundingBox().MaxY();
            csVector3 middle = charPos + (targetPos - charPos) * 0.5f;

            SetTarget(middle);

            csVector3 delta = targetPos - charPos;

            float aspect = view->GetPerspectiveCamera()->GetShiftX() / view->GetPerspectiveCamera()->GetShiftY();
            float d = (middle - charPos).Norm() / (tanf(view->GetPerspectiveCamera()->GetFOVAngle()) * aspect * 0.5f) * 20.1f;
            d += GetDistance();

            if (d < 2.0f)
                d = 2.0f;

            float pitch = -GetPitch();
            float yaw = GetYaw();
            float x = cosf(yaw) * cosf(pitch) * d;
            float y = sinf(pitch) * d;
            float z = sinf(yaw) * cosf(pitch) * d;

            SetPosition(middle + (delta % GetUp()).Unit() * x + GetUp() * y + delta.Unit() * z);
            break;
        }
    }
}

csVector3 psCamera::CalcCollisionPos(const csVector3& pseudoTarget, const csVector3& pseudoPosition, iSector* sector)
{
    hasCollision = false;
    if (!useCameraCD)
        return pseudoPosition; // no collision detection

    actor->GetMesh()->GetFlags().Set(CS_ENTITY_NOHITBEAM);
    switch (GetCameraMode())
    {
        case CAMERA_THIRD_PERSON:
        case CAMERA_M64_THIRD_PERSON:
        case CAMERA_LARA_THIRD_PERSON:
        case CAMERA_FREE:
        case CAMERA_NPCTALK:
            csVector3 isect;
            csIntersectingTriangle closest_tri;
            csVector3 modifiedTarget = pseudoTarget;

            //iMeshWrapper* mesh = sector->HitBeamPortals(modifiedTarget, pseudoPosition, isect, &sel);
            //
            iMeshWrapper * mesh;
            csColliderHelper::TraceBeam(cdsys, sector, modifiedTarget, pseudoPosition, true, closest_tri, isect, &mesh);
            if (mesh)
            {
                actor->GetMesh()->GetFlags().Reset(CS_ENTITY_NOHITBEAM);
                hasCollision = true;
                return isect + (modifiedTarget-isect)*0.1f;
            }
//                csRef<iSpriteCal3DState> spstate =  scfQueryInterface<iSpriteCal3DState> (mesh->GetMeshObject());
            break;
    }
    actor->GetMesh()->GetFlags().Reset(CS_ENTITY_NOHITBEAM);
    return pseudoPosition;
}

void psCamera::DoCameraTransition()
{
    if (inTransitionPhase)
    {
        if ((GetPosition() - GetPosition(CAMERA_ACTUAL_DATA)).SquaredNorm() < transitionThresholdSquared)
        {
            inTransitionPhase = false;
        }
    }
}

void psCamera::DoElasticPhysics(bool isElastic, const csTicks elapsedTicks, const CameraData& deltaIdeal, iSector* sector)
{
    // if the camera mode is elastic then progress gradually to the ideal pos
    if (isElastic)
    {
        csVector3 newPseudoPos = CalcCollisionPos(GetTarget(), GetPosition(), sector);

        float cameraSpringCoef, cameraInertialDampeningCoef, cameraSpringLength;
    if (hasCollision)
    {
            cameraSpringCoef = GetSpringCoef(CAMERA_COLLISION);
            cameraInertialDampeningCoef = GetDampeningCoef(CAMERA_COLLISION);
            cameraSpringLength = GetSpringLength(CAMERA_COLLISION);
    }
        else if (!inTransitionPhase)
        {
            cameraSpringCoef = GetSpringCoef();
            cameraInertialDampeningCoef = GetDampeningCoef();
            cameraSpringLength = GetSpringLength();
        }
        else
        {
            cameraSpringCoef = GetSpringCoef(CAMERA_TRANSITION);
            cameraInertialDampeningCoef = GetDampeningCoef(CAMERA_TRANSITION);
            cameraSpringLength = GetSpringLength(CAMERA_TRANSITION);
        }

        csVector3 velIdealPos, velIdealTar, velIdealUp, newPos, newTar, newUp;

        newPos = CalcElasticPos(GetPosition(CAMERA_ACTUAL_DATA), newPseudoPos, deltaIdeal.worldPos,
                                (float)elapsedTicks/1000.0f, cameraSpringCoef,
                                cameraInertialDampeningCoef, cameraSpringLength);
        SetPosition(newPos, CAMERA_ACTUAL_DATA);

        newTar = CalcElasticPos(GetTarget(CAMERA_ACTUAL_DATA), GetTarget(), deltaIdeal.worldTar,
                                (float)elapsedTicks/1000.0f, cameraSpringCoef,
                                cameraInertialDampeningCoef, cameraSpringLength);
        SetTarget(newTar, CAMERA_ACTUAL_DATA);

        newUp = CalcElasticPos(GetUp(CAMERA_ACTUAL_DATA), GetUp(), deltaIdeal.worldUp,
                               (float)elapsedTicks/1000.0f, cameraSpringCoef,
                               cameraInertialDampeningCoef, cameraSpringLength);
        SetUp(newUp, CAMERA_ACTUAL_DATA);
    }
    else
    {
        // camera isn't elastic, so no interpolation is done between ideal and actual
        SetPosition(CalcCollisionPos(GetTarget(), GetPosition(), sector), CAMERA_ACTUAL_DATA);
        SetTarget(GetTarget(), CAMERA_ACTUAL_DATA);
        SetUp(GetUp(), CAMERA_ACTUAL_DATA);
    }
}

void psCamera::EnsureActorVisibility()
{
    actor->GetMesh()->SetFlagsRecursive(CS_ENTITY_INVISIBLE, CS_ENTITY_INVISIBLE);

    // make the actor visible if the camera mode calls for it, and
    // the camera isn't too close to the player
    if (IsActorVisible() || inTransitionPhase)
    {
        if ((GetPosition(CAMERA_ACTUAL_DATA)-GetTarget(CAMERA_ACTUAL_DATA)).SquaredNorm() > 0.3f)
            actor->GetMesh()->SetFlagsRecursive(CS_ENTITY_INVISIBLE, 0);
    }
}

csVector3 psCamera::CalcElasticPos(csVector3 currPos, csVector3 idealPos, csVector3 deltaIdealPos,
                                   float deltaTime, float springCoef, float dampCoef, float springLength) const
{
    csVector3 deltaPos;
    csVector3 vel;
    float force;

    deltaPos = currPos - idealPos;
    if (deltaPos.SquaredNorm() == 0)
        return currPos;

    vel = deltaIdealPos * deltaTime;
    force = springCoef * (springLength - deltaPos.Norm()) + dampCoef * (deltaPos * vel) / deltaPos.Norm();

    float dist = deltaPos.Norm();
    //printf("DIST: %7.3f,  force*deltaTime: %7.3f \n", dist, force*deltaTime);
    if (-dist < force * deltaTime)
        deltaPos *= force * deltaTime / dist;
    else
        deltaPos *= -1;

    return currPos + deltaPos;
}

float psCamera::CalcElasticFloat(float curr, float ideal, float deltaIdeal,
                                 float deltaTime, float springCoef, float dampCoef, float springLength) const
{
    float delta;
    float vel;
    float force;

    delta = curr - ideal;
    if (delta == 0)
        return curr;

    vel = deltaIdeal * deltaTime;
    force = springCoef * (springLength - delta) + dampCoef * (delta * vel) / delta;

    if (-delta < force * deltaTime)
        delta = force * deltaTime;
    else
        delta *= -1;

    return curr + delta;
}

void psCamera::CalculateFromYawPitchRoll(int mode)
{
    float cosYaw, sinYaw;
    float cosPit, sinPit;
    float cosRol, sinRol;

    cosYaw = cosf(GetYaw(mode));    sinYaw = sinf(GetYaw(mode));
    cosPit = cosf(GetPitch(mode));  sinPit = sinf(GetPitch(mode));
    cosRol = 1.0f;                        sinRol = 0.0f;  // at this point, our camera doesn't support Roll

    if (cosPit == 0.0f)
        cosPit = 0.001f;

    SetTarget(GetPosition(mode) + (GetDistance(mode) * csVector3(-sinYaw * cosPit,
                                                                 sinPit,
                                                                 cosPit * -cosYaw)));
}

void psCamera::CalculateNewYawPitchRoll(int mode)
{
    // arcsin((dir / dist).y)
    SetPitch(asinf((((GetTarget(mode).y - GetPosition(mode).y) / GetDistance(mode)))), mode);

    // arccos((dir / dist).z / -cos(pitch))
    SetYaw(acosf(((GetTarget(mode).z - GetPosition(mode).z) / GetDistance(mode)) / -cosf(GetPitch(mode))), mode);
    // but have to check the z value for possible vertical flip
    if (GetTarget(mode).x > GetPosition(mode).x)
        SetYaw(2*PI - GetYaw(mode), mode);

}

void psCamera::CalculatePositionFromYawPitchRoll(int mode)
{
    float cosYaw, sinYaw;
    float cosPit, sinPit;
    float cosRol, sinRol;

    cosYaw = cosf(GetYaw(mode));    sinYaw = sinf(GetYaw(mode));
    cosPit = cosf(GetPitch(mode));  sinPit = sinf(GetPitch(mode));
    cosRol = 1.0f;                        sinRol = 0.0f;  // at this point, our camera doesn't support Roll

    if (cosPit == 0.0f)
        cosPit = 0.001f;

    SetPosition(GetTarget(mode) - (GetDistance(mode) * csVector3(-sinYaw * cosPit,
                                                                 sinPit, // we have to reverse the vertical thing
                                                                 cosPit * -cosYaw)));
}

float psCamera::CalculateNewYaw(csVector3 dir)
{
    if (dir.x == 0.0f)
        dir.x = 0.00001f;

    return atan2(-dir.x, -dir.z);
}

void psCamera::EnsureCameraDistance(int mode)
{
    if (GetDistance(mode) > GetMaxDistance(mode))
        SetDistance(GetMaxDistance(mode), mode);
    else if (GetDistance(mode) < GetMinDistance(mode))
        SetDistance(GetMinDistance(mode), mode);
}

bool psCamera::IsActorVisible(int mode) const
{
    if (mode < 0 || mode == CAMERA_ACTUAL_DATA) mode = currCameraMode;
    return (mode != CAMERA_FIRST_PERSON);
}

float psCamera::SaturateAngle(float angle) const
{
    while (angle >= PI)
        angle -= 2*PI;

    while (angle < -3.14169f)  //TODO : is this right, or should it be replaced with PI?
        angle += 2*PI;

    return angle;
}

bool psCamera::CloneCameraModeData(int fromMode, int toMode)
{
    SetPosition(GetPosition(fromMode), toMode);
    SetTarget(GetTarget(fromMode), toMode);
    SetUp(GetUp(fromMode), toMode);

    SetPitch(GetPitch(fromMode), toMode);
    SetYaw(GetYaw(fromMode), toMode);
    //SetRoll(GetRoll(fromMode), toMode);

    SetDefaultPitch(GetDefaultPitch(fromMode), toMode);
    SetDefaultYaw(GetDefaultYaw(fromMode), toMode);
    //SetDefaultRoll(GetDefaultRoll(fromMode), toMode);

    SetDistance(GetDistance(fromMode), toMode);
    SetMinDistance(GetMinDistance(fromMode), toMode);
    SetMaxDistance(GetMaxDistance(fromMode), toMode);

    SetTurnSpeed(GetTurnSpeed(fromMode), toMode);

    SetSpringCoef(GetSpringCoef(fromMode), toMode);
    SetDampeningCoef(GetDampeningCoef(fromMode), toMode);
    SetSpringLength(GetSpringLength(fromMode), toMode);

    SetSwingCoef(GetSwingCoef(fromMode), toMode);
    return true;
}

void psCamera::SetDistanceClipping(float dist)
{
    csVector3 v1(0, 0, dist), v2(0, 1, dist), v3(1, 0, dist);
    csPlane3  p(v1, v2, v3);
    view->GetCamera()->SetFarPlane(&p);

    // control distance we see shadows as well
    psShadowManager *shadowManager = psengine->GetCelClient()->GetShadowManager();
    shadowManager->SetShadowRange(dist);

    // control load distance.
    psengine->GetLoader()->SetLoadRange(dist+dist/10);
}

float psCamera::GetDistanceClipping()
{
    csPlane3  *p;
    p = view->GetCamera()->GetFarPlane();
    if (p != NULL)
        return p->DD / sqrt(p->norm.x*p->norm.x + p->norm.y*p->norm.y + p->norm.z*p->norm.z);
    else
        return -1;
}

void psCamera::AdaptDistanceClipping()
{
    const float MAX_DIST = 500;             // maximum visibile distance that is never crossed
    const float INITIAL_DISTANCE = 200;     // we begin from this value
    static bool calledFirstTime = true;

    static csTicks lastTime;                // when was this method last called ?
    csTicks currTime;                       // the current time
    static float lastChangeTime;            // when did we change the distance last time ?

    float currFPS;                          // FPS calculated from the last frame
    static float smoothFPS = 30;            // smoothed FPS - less influeced by chaotic FPS movements
    float currDist;                         // current distance limit
    csString debug;

    // when we are called for the first time, we just initialize some variables and exit
    if (calledFirstTime)
    {
        calledFirstTime = false;
        lastTime = csGetTicks();
        lastChangeTime = csGetTicks();
        return;
    }

    currTime = csGetTicks();
    if (currTime == lastTime)
        currFPS = 1000;  // ha, ha .... this is just a theoretial check ;-)
    else
        currFPS = 1000 / (currTime - lastTime);
    smoothFPS = 0.5*currFPS + 0.5*smoothFPS;

    //debug.AppendFmt("fps=%.0f avg=%.0f ", currFPS, smoothFPS);

    if (distanceCfg.adaptive  &&  currTime-lastChangeTime>1000*2)
    {
        float change;

        currDist = GetDistanceClipping();
        if (currDist == -1)
            currDist = INITIAL_DISTANCE;

        if (smoothFPS < distanceCfg.minFPS)
        {
            change = - (distanceCfg.minFPS-smoothFPS);
            currDist += change;
            if (currDist >= distanceCfg.minDist)
                SetDistanceClipping(currDist);
            //debug.AppendFmt("%.5f-=%.5f \n",currDist, change);
        }
        else
        if (smoothFPS > distanceCfg.maxFPS)
        {
            change = (smoothFPS-distanceCfg.maxFPS);
            currDist += change;
            if (currDist <= MAX_DIST)
                SetDistanceClipping(currDist);
            //debug.AppendFmt("%.5f+=%.5f \n",currDist, change);
        }
        lastChangeTime = currTime;
    }

    //Error2("%s", debug.GetData());

    lastTime = currTime;
}



