/* * pawscharcreatemain.cpp - author: Andrew Craig
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
#include <ctype.h>
#include <imesh/spritecal3d.h>
#include <igraphic/image.h>
#include <ivideo/txtmgr.h>
#include <imap/loader.h>

#include "globals.h"
#include "charapp.h"
#include "iclient/ibgloader.h"
#include "net/messages.h"
#include "net/clientmsghandler.h"
#include "net/charmessages.h"
#include "iclient/isoundmngr.h"

#include "paws/pawsmanager.h"
#include "paws/pawstextbox.h"
#include "paws/pawsradio.h"
#include "paws/pawsokbox.h"
#include "paws/pawsobjectview.h"
#include "paws/pawsmainwidget.h"

#include "pawscharpick.h"
#include "pawscharcreatemain.h"
#include "psclientchar.h"

//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//  PAWS BUTTON IDENTIFIERS
//////////////////////////////////////////////////////////////////////////////
#define BACK_BUTTON     1000
#define QUICK_BUTTON 2000
#define NEXT_BUTTON     3000

#define MALE_BUTTON     100
#define FEMALE_BUTTON   200

#define NAMEHELP_BUTTON 900

#define FEMALE_ICON "mainfemale"
#define NEUTRAL_ICON "mainneutral"

//////////////////////////////////////////////////////////////////////////////

// Copied from the server
bool FilterName(const char* name);

//////////////////////////////////////////////////////////////////////////////
pawsCreationMain::pawsCreationMain()
{      
    createManager = psengine->GetCharManager()->GetCreation();
    
    psengine->GetMsgHandler()->Subscribe(this, MSGTYPE_CHAR_CREATE_CP); 
    psengine->GetMsgHandler()->Subscribe(this, MSGTYPE_CHAR_CREATE_NAME);
    createManager->SetGender( PSCHARACTER_GENDER_MALE ); 
    currentGender = PSCHARACTER_GENDER_MALE;
    lastGender = -1;
    
    femaleImage = PawsManager::GetSingleton().GetTextureManager()->GetPawsImage(FEMALE_ICON);
    if (!femaleImage)
    {
        Warning1( LOG_PAWS, "Could not locate the female icon image. This image is not part of the GPL.");
    }
    neutralImage = PawsManager::GetSingleton().GetTextureManager()->GetPawsImage(NEUTRAL_ICON);
    if (!neutralImage)
    {
        Warning1( LOG_PAWS, "Could not locate the neutral icon image. This image is not part of the GPL.");
    }
        
    currentFaceChoice = 0; 
    activeHairStyle = 0;
    currentBeardStyleChoice = 0;
    activeHairColour = 0;
    currentSkinColour = 0;
    race = 0;
    
    nameWarning = 0;
    
    charApp = new psCharAppearance(psengine->GetObjectRegistry());
    loaded = true;

    psengine->RegisterDelayedLoader(this);
}


void pawsCreationMain::Reset()
{
    createManager->SetGender( PSCHARACTER_GENDER_MALE ); 
    currentGender = PSCHARACTER_GENDER_MALE;
    
    currentFaceChoice = 0; 
    activeHairStyle = 0;
    currentBeardStyleChoice = 0;
    activeHairColour = 0;
    currentSkinColour = 0;
    race = 0;
    
    view->Clear();
    
    pawsRadioButtonGroup* raceBox = (pawsRadioButtonGroup*)FindWidget("RaceBox");        
    raceBox->TurnAllOff();
    
    pawsEditTextBox* name = (pawsEditTextBox*)FindWidget("charnametext");
    name->SetText("");
    nameWarning = 0;                           
          
    ResetAllWindows();        
}


pawsCreationMain::~pawsCreationMain()
{
    psengine->UnregisterDelayedLoader(this);
}

bool pawsCreationMain::PostSetup()
{
    cpPoints = (pawsTextBox*)FindWidget("cppoints");
    
    if ( !(cpPoints) )
    {        
        return false;
    }
    
         
    view = (pawsObjectView*)FindWidget("ModelView");

    faceLabel = (pawsTextBox*)FindWidget( "Face" );                
    hairStyleLabel = (pawsTextBox*)FindWidget( "HairStyles" );                
    beardStyleLabel = (pawsTextBox*)FindWidget( "BeardStyles" );                
    hairColourLabel = (pawsTextBox*)FindWidget( "HairColours" );                
    skinColourLabel = (pawsTextBox*)FindWidget( "SkinColours" );     

    nameTextBox  = dynamic_cast <pawsEditTextBox*> (FindWidget("charnametext"));
    if (nameTextBox == NULL)
        return false;

    GrayRaceButtons();
    GrayStyleButtons();

    createManager->GetTraitData();

    return true;
}

void pawsCreationMain::ResetAllWindows()
{  
    const char *names[] =
    {
        "CharBirth", "birth.xml",
        "Childhood", "childhood.xml",
        "LifeEvents","lifeevents.xml",
        "Parents","parents.xml", 
        /*"Paths","paths.xml",*/ 
        NULL
    };
    
    pawsWidget * wnd;
    pawsMainWidget * mainWidget = PawsManager::GetSingleton().GetMainWidget();
    
    for (int nameNum = 0; names[nameNum] != NULL; nameNum += 2)
    {
        wnd = PawsManager::GetSingleton().FindWidget(names[nameNum]);
        if (wnd != NULL)
        {
            mainWidget->DeleteChild(wnd);
            PawsManager::GetSingleton().LoadWidget(names[nameNum+1]);
        }
        else
            Error2("Failed to find window %s", names[nameNum]);
    }
    createManager->ClearChoices();
}

void pawsCreationMain::HandleMessage( MsgEntry* me )
{
    switch ( me->GetType() )
    {
        case MSGTYPE_CHAR_CREATE_CP:
        {  
            int race = me->GetInt32();

            createManager->SetCurrentCP( createManager->GetRaceCP( race ) );
            
            UpdateCP();
            return;
        }
        
        case MSGTYPE_CHAR_CREATE_NAME:
        {
            psNameCheckMessage msg;
            msg.FromServer(me);
            if (!msg.accepted)
            {
                PawsManager::GetSingleton().CreateWarningBox( msg.reason ); 
            }
            else
            {
                if ( newWindow.Length() > 0 )
                {
                    Hide();
                    pawsEditTextBox* name = (pawsEditTextBox*)FindWidget("charnametext");
                    if ( name )
                        createManager->SetName( name->GetText() );
                    PawsManager::GetSingleton().FindWidget(newWindow)->Show();
                }
            }
        }
    }
}

void pawsCreationMain::ChangeSkinColour( int currentChoice )
{    
    if ( race->location[PSTRAIT_LOCATION_SKIN_TONE][currentGender].GetSize() == 0 )
    {
        skinColourLabel->SetText(PawsManager::GetSingleton().Translate("Skin Colour"));
        return;
    }

    Trait * trait = race->location[PSTRAIT_LOCATION_SKIN_TONE][currentGender][currentChoice];
    
    while ( trait )
    {
        charApp->SetSkinTone(trait->mesh, trait->material);
        trait = trait->next_trait;
    }
    
    skinColourLabel->SetText( race->location[PSTRAIT_LOCATION_SKIN_TONE][currentGender][currentChoice]->name );                       
}

void pawsCreationMain::ChangeHairColour( int newHair )
{
    if ( newHair <(int) race->location[PSTRAIT_LOCATION_HAIR_COLOR][currentGender].GetSize() )
    {
        Trait* trait = race->location[PSTRAIT_LOCATION_HAIR_COLOR][currentGender][newHair]; 
        
        csVector3 at(0, race->zoomLocations[PSTRAIT_LOCATION_FACE].y, 0); 
        view->UnlockCamera();
        view->LockCamera(race->zoomLocations[PSTRAIT_LOCATION_FACE], at, true);                                  
        
        charApp->HairColor(trait->shader);
        
        hairColourLabel->SetText( trait->name );                               
        
        activeHairColour = newHair;
    }
    else
        hairColourLabel->SetText(PawsManager::GetSingleton().Translate("Hair Colour"));
}



void pawsCreationMain::SetHairStyle( int newStyle )
{    
    if ( newStyle < (int)race->location[PSTRAIT_LOCATION_HAIR_STYLE][currentGender].GetSize() )
    {
        Trait* trait = race->location[PSTRAIT_LOCATION_HAIR_STYLE][currentGender][newStyle];
        charApp->HairMesh(trait->mesh);
        csVector3 at(0, race->zoomLocations[PSTRAIT_LOCATION_FACE].y, 0);
        view->UnlockCamera();
        view->LockCamera(race->zoomLocations[PSTRAIT_LOCATION_FACE], at, true);                                         
        hairStyleLabel->SetText( trait->name );                       
    }
    else
        hairStyleLabel->SetText(PawsManager::GetSingleton().Translate("Hair Style"));
}

void pawsCreationMain::ChangeHairStyle( int newChoice, int oldChoice )
{        
//    RemoveHairStyle( oldChoice );
    SetHairStyle( newChoice );    
}



void pawsCreationMain::ChangeBeardStyle( int newStyle )
{   
    if ( newStyle < (int)race->location[PSTRAIT_LOCATION_BEARD_STYLE][currentGender].GetSize() )
    {
        Trait* trait = race->location[PSTRAIT_LOCATION_BEARD_STYLE][currentGender][newStyle];
        charApp->BeardMesh(trait->mesh);
        csVector3 at(0, race->zoomLocations[PSTRAIT_LOCATION_FACE].y, 0); 
        view->UnlockCamera();
        view->LockCamera(race->zoomLocations[PSTRAIT_LOCATION_FACE], at, true);                                         
        beardStyleLabel->SetText( trait->name );                       
    }
    else
        beardStyleLabel->SetText(PawsManager::GetSingleton().Translate("Beard Style"));


    if ( race->location[PSTRAIT_LOCATION_BEARD_STYLE][currentGender].GetSize() == 0 )
    {
        beardStyleLabel->SetText(PawsManager::GetSingleton().Translate("Beard Style"));
        return;
    }
                  
//    iMeshWrapper * mesh = view->GetObject();

//    psengine->SetTrait(mesh,race->location[PSTRAIT_LOCATION_BEARD_STYLE][currentGender][currentChoice]);
//    beardStyleLabel->SetText( race->location[PSTRAIT_LOCATION_BEARD_STYLE][currentGender][currentChoice]->name );                       

/* TODO: Move this code to psengine SetTrait
    if ( race->beardStyles[currentGender].GetSize() == 0 )
        return;
        
    iMeshWrapper * mesh = view->GetObject();
    csRef<iSpriteCal3DState> spstate =  scfQueryInterface<iSpriteCal3DState> (mesh->GetMeshObject());
        
    if ( lastAddedBeardStyle != -1 )
    {
        spstate->DetachCoreMesh( race->beardStyles[currentGender][lastAddedBeardStyle]->mesh ) ;         
    }        
    
    // If the choice is no mesh then make sure there is no new mesh added.
    if ( race->beardStyles[currentGender][currentChoice]->mesh.GetSize() == 0 )
    {
        lastAddedBeardStyle = -1;
        beardStyleLabel->SetText( race->beardStyles[currentGender][currentChoice]->name );            
        return;
    }
    else
    {        
        // Check to see if a mesh was given:
        bool attach = spstate->AttachCoreMesh( race->beardStyles[currentGender][currentChoice]->mesh );
        
        if ( currentHairColour != -1 )
        {
            // Use the 1. hair colour for the beard 
            iMaterialWrapper* material = psengine->LoadMaterial( race->hairColours[currentGender][currentHairColour]->materials[0]->material,
                                                       race->hairColours[currentGender][currentHairColour]->materials[0]->texture  );               

            // If the material was found then all is ok                                                       
            if (material)                                                                   
            {
                spstate->SetMaterial( race->beardStyles[currentGender][currentChoice]->mesh, material );                            
            }
            else // Remove the mesh since there is no valid material for it.
            {
                Warning2( LOG_NEWCHAR, "Material Not Loaded ( No Texture: %s )", race->hairColours[currentGender][currentHairColour]->materials[0]->texture.GetData() );      
                spstate->DetachCoreMesh( race->beardStyles[currentGender][currentChoice]->mesh ) ;                                 
            }
        }
    }        
    
    
    lastAddedBeardStyle = currentChoice;                          
    beardStyleLabel->SetText( race->beardStyles[currentGender][currentChoice]->name );    
*/
}


void pawsCreationMain::ChangeFace( int newFace )
{
    if ( newFace < (int)race->location[PSTRAIT_LOCATION_FACE][currentGender].GetSize() )
    {
        csVector3 at(0, race->zoomLocations[PSTRAIT_LOCATION_FACE].y, 0);
        Trait* trait = race->location[PSTRAIT_LOCATION_FACE][currentGender][newFace];
        view->UnlockCamera();
        view->LockCamera(race->zoomLocations[PSTRAIT_LOCATION_FACE], at, true);
                      
        charApp->FaceTexture(trait->material);
        
        faceLabel->SetText( trait->name );                                       
    }
    else
        faceLabel->SetText(PawsManager::GetSingleton().Translate("Face"));

}


void pawsCreationMain::GrayRaceButtons( )
{
    pawsRadioButtonGroup* raceBox = (pawsRadioButtonGroup*)FindWidget("RaceBox");
    for (unsigned id = 0; id < 12; id++)
    {
        pawsRadioButton* button = (pawsRadioButton*)raceBox->FindWidget(id);
        if (button == NULL)
            continue;
        if (!createManager->IsAvailable(id,currentGender))
            button->GetTextBox()->Grayed(true);
        else
            button->GetTextBox()->Grayed(false);
    }
}

void pawsCreationMain::GrayStyleButtons( )
{    
    bool faceEnable = true;
    if ( race != 0 && race->location[PSTRAIT_LOCATION_FACE][currentGender].GetSize())
    {
        faceEnable = false;
    }
    faceLabel->Grayed(faceEnable);

    bool hairStyleEnable = true;
    if ( race != 0 && race->location[PSTRAIT_LOCATION_HAIR_STYLE][currentGender].GetSize())
    {
        hairStyleEnable = false;
    }
    hairStyleLabel->Grayed(hairStyleEnable);

    bool beardStyle = true;
    if ( race != 0 && race->location[PSTRAIT_LOCATION_BEARD_STYLE][currentGender].GetSize())
    {
        beardStyle = false;
    }
    beardStyleLabel->Grayed(beardStyle);

    bool hairColour = true;
    if ( race != 0 && race->location[PSTRAIT_LOCATION_HAIR_COLOR][currentGender].GetSize())
    {
        hairColour = false;
    }
    hairColourLabel->Grayed(hairColour);

    bool skinColour = true;
    if ( race != 0 && race->location[PSTRAIT_LOCATION_SKIN_TONE][currentGender].GetSize())
    {
        skinColour = false;
    }
    skinColourLabel->Grayed(skinColour);
}

void pawsCreationMain::SelectGender(int newGender)
{
    currentGender = newGender;
    
    GrayRaceButtons();
    GrayStyleButtons();

    if( createManager->GetSelectedRace() == -1)
        return;

    int race = createManager->GetSelectedRace();

    while (!createManager->IsAvailable(race, currentGender))
    {
        race++;
        if (race > 11)
        {
            race = 0; 
            pawsRadioButtonGroup* raceBox = (pawsRadioButtonGroup*)FindWidget("RaceBox");
            raceBox->TurnAllOff();
            return;
        } 
             
    }

    if (race != createManager->GetSelectedRace())
    {
        //PawsManager::GetSingleton().CreateWarningBox( "That gender isn't implemented for that race yet, sorry." );
        pawsRadioButtonGroup* raceBox = (pawsRadioButtonGroup*)FindWidget("RaceBox");
        raceBox->TurnAllOff();
        pawsRadioButton* button = (pawsRadioButton*)raceBox->FindWidget(race);
        button->SetState(true);
    }

    createManager->SetGender( currentGender );

    // Trigger gender update
    UpdateRace(race);
}

bool pawsCreationMain::OnButtonPressed( int mouseButton, int keyModifier, pawsWidget* widget )
{
     ////////////////////////////////////////////////////////////////////
     // Previous face.
     ////////////////////////////////////////////////////////////////////        
    if ( strcmp( widget->GetName(), "Custom Choice Set 1 <") == 0 )
    {
        if ( race )
        {                                        
            if ( race->location[PSTRAIT_LOCATION_FACE][currentGender].GetSize() == 0 )
                return true;
                
            currentFaceChoice--;
            
            if ( currentFaceChoice < 0 )
            {
                currentFaceChoice =  (int)race->location[PSTRAIT_LOCATION_FACE][currentGender].GetSize()-1;
            }    
                               
         
            ChangeFace( currentFaceChoice );                
        }                                                                       
        return true;        
    }
    
     ////////////////////////////////////////////////////////////////////
     // Next Set of faces.
     ////////////////////////////////////////////////////////////////////            
    if ( strcmp( widget->GetName(), "Custom Choice Set 1 >") == 0 )
    {
        if ( race )
        {                        
            if ( race->location[PSTRAIT_LOCATION_FACE][currentGender].GetSize() == 0 )
            {
                return true;
            }
            
            currentFaceChoice++;
            
            if ( currentFaceChoice == (int)race->location[PSTRAIT_LOCATION_FACE][currentGender].GetSize() )
            {
                currentFaceChoice = 0;
            }

            ChangeFace( currentFaceChoice );                                            
        }                
        
        return true;        
    }
    
    ////////////////////////////////////////////////////////////////////
    // Next Set of hair Styles.
    ////////////////////////////////////////////////////////////////////        
    if ( strcmp( widget->GetName(), "Custom Choice Set 2 >") == 0 )
    {
        if ( race )
        {                        
            if ( race->location[PSTRAIT_LOCATION_HAIR_STYLE][currentGender].GetSize() == 0 )
            {
                return true;
            }
            
            int old = activeHairStyle;                            
            activeHairStyle++;
            
            if ( activeHairStyle == (int)race->location[PSTRAIT_LOCATION_HAIR_STYLE][currentGender].GetSize() )
            {
                activeHairStyle = 0;
            }
                              
            ChangeHairStyle( activeHairStyle, old );  
        }
        
        return true;                        
    }
    
    ////////////////////////////////////////////////////////////////////
    // Previous Set of Hair Styles.
    ////////////////////////////////////////////////////////////////////        
    if ( strcmp( widget->GetName(), "Custom Choice Set 2 <") == 0 )
    {
        if ( race )
        {                        
            if ( race->location[PSTRAIT_LOCATION_HAIR_STYLE][currentGender].GetSize() == 0 )
            {
                return true;
            }
            
            int old = activeHairStyle;                
            activeHairStyle--;
            
            if ( activeHairStyle < 0 )
            {
                activeHairStyle = (int)race->location[PSTRAIT_LOCATION_HAIR_STYLE][currentGender].GetSize()-1;
            }
                                
            ChangeHairStyle( activeHairStyle, old );  
        }
        
        return true;                        
    }
    
    
    ////////////////////////////////////////////////////////////////////
    // Next Beard Style.
    ////////////////////////////////////////////////////////////////////        
    if ( strcmp( widget->GetName(), "Custom Choice Set 3 >") == 0 )
    {
        if ( race )
        {                        
            if ( race->location[PSTRAIT_LOCATION_BEARD_STYLE][currentGender].GetSize() == 0 )
            {
                return true;
            }
            
            currentBeardStyleChoice++;
            
            if ( currentBeardStyleChoice == (int)race->location[PSTRAIT_LOCATION_BEARD_STYLE][currentGender].GetSize() )
            {
                currentBeardStyleChoice = 0;
            }
            
                              
            ChangeBeardStyle( currentBeardStyleChoice );  
        }
        
        return true;                        
    }
    
    ////////////////////////////////////////////////////////////////////
    // Previous Set of Beard Styles.
    ////////////////////////////////////////////////////////////////////        
    if ( strcmp( widget->GetName(), "Custom Choice Set 3 <") == 0 )
    {
        if ( race )
        {                        
            if ( race->location[PSTRAIT_LOCATION_BEARD_STYLE][currentGender].GetSize() == 0 )
            {
                return true;
            }
            
            currentBeardStyleChoice--;
            
            if ( currentBeardStyleChoice < 0 )
            {
                currentBeardStyleChoice = (int)race->location[PSTRAIT_LOCATION_BEARD_STYLE][currentGender].GetSize()-1;
            }
                                
            ChangeBeardStyle( currentBeardStyleChoice );  
        }
        
        return true;                        
    }
    
    
    
    ////////////////////////////////////////////////////////////////////
    // Next Hair Colours.
    ////////////////////////////////////////////////////////////////////        
    if ( strcmp( widget->GetName(), "Custom Choice Set 4 >") == 0 )
    {
        if ( race )
        {                        
            if ( race->location[PSTRAIT_LOCATION_HAIR_COLOR][currentGender].GetSize() == 0 )
            {
                return true;
            }
            
            activeHairColour++;
            
            if ( activeHairColour >= (int)race->location[PSTRAIT_LOCATION_HAIR_COLOR][currentGender].GetSize() )
            {
                activeHairColour = 0;
            }
                              
            ChangeHairColour( activeHairColour );  
        }
        
        return true;                        
    }
    
    ////////////////////////////////////////////////////////////////////
    // Previous Set of HairColours
    ////////////////////////////////////////////////////////////////////        
    if ( strcmp( widget->GetName(), "Custom Choice Set 4 <") == 0 )
    {
        if ( race )
        {
            if ( race->location[PSTRAIT_LOCATION_HAIR_COLOR][currentGender].GetSize() == 0 )
                return true;
            
            activeHairColour--;
            
            if ( activeHairColour < 0 )
                activeHairColour = (int)race->location[PSTRAIT_LOCATION_HAIR_COLOR][currentGender].GetSize()-1;
                                
            ChangeHairColour( activeHairColour );  
        }
        
        return true;                        
    }

    ////////////////////////////////////////////////////////////////////
    // Skin Colours.
    ////////////////////////////////////////////////////////////////////        
    if ( strcmp( widget->GetName(), "Custom Choice Set 5 >") == 0 )
    {
        if ( race )
        {
            if ( race->location[PSTRAIT_LOCATION_SKIN_TONE][currentGender].GetSize() == 0 )
                return true;
            
            currentSkinColour++;
            
            if ( currentSkinColour == (int)race->location[PSTRAIT_LOCATION_SKIN_TONE][currentGender].GetSize() )
                currentSkinColour = 0;                        
                              
            ChangeSkinColour( currentSkinColour );  
        }
        
        return true;                        
    }
    
    ////////////////////////////////////////////////////////////////////
    // Previous Set of Skin Colours
    ////////////////////////////////////////////////////////////////////        
    if ( strcmp( widget->GetName(), "Custom Choice Set 5 <") == 0 )
    {
        if ( race )
        {                        
            if ( race->location[PSTRAIT_LOCATION_SKIN_TONE][currentGender].GetSize() == 0 )
                return true;
            
            currentSkinColour--;
            
            if ( currentSkinColour < 0 )
                currentSkinColour = (int)race->location[PSTRAIT_LOCATION_SKIN_TONE][currentGender].GetSize()-1;
                                
            ChangeSkinColour( currentSkinColour );  
        }
        
        return true;                        
    }
        
    
    
    /////////////////////////////////////////////////////
    // RACE SELECTION BUTTONS
    /////////////////////////////////////////////////////    
    if ( widget->GetID() >= 0 && widget->GetID() <= 11 )
    {   
        if (
            !createManager->IsAvailable(widget->GetID(),1) &&
            !createManager->IsAvailable(widget->GetID(),2)
            )
        {
            PawsManager::GetSingleton().CreateWarningBox(PawsManager::GetSingleton().Translate(
                                                         "This race isn't implemented yet, sorry"));
            pawsRadioButtonGroup* raceBox = (pawsRadioButtonGroup*)FindWidget("RaceBox");
            
            if ( lastRaceID != -1 )
            {
                csString raceActive;
                raceActive.Format("race%d", lastRaceID );
                if ( raceBox )
                    raceBox->SetActive( raceActive );
            }                 
            else
            {
                if ( raceBox )
                    raceBox->TurnAllOff();
            }                
                                       
            return true;
        }

        if (lastRaceID == -1)
        {
            SelectGender(lastGender);                            
            currentGender = lastGender;
        }
        
        // If current gender isn't available, try every possible
        if (!createManager->IsAvailable(widget->GetID(),currentGender))
        {
            if (createManager->IsAvailable(widget->GetID(),PSCHARACTER_GENDER_FEMALE))
                SelectGender(PSCHARACTER_GENDER_FEMALE);
            else if (createManager->IsAvailable(widget->GetID(),PSCHARACTER_GENDER_MALE))
                SelectGender(PSCHARACTER_GENDER_MALE);
                
            lastGender = currentGender;                
        }

        pawsOkBox* Ok = (pawsOkBox*)PawsManager::GetSingleton().FindWidget("OkWindow");
        Ok->SetText(  createManager->GetRaceDescription( widget->GetID() ) );

        Ok->MoveTo( (graphics2D->GetWidth() -  GetActualWidth(512) )/2,
                    (GetActualHeight(600) - GetActualHeight(256)));

        Ok->Show();  
        PawsManager::GetSingleton().SetModalWidget(Ok);
        
        createManager->SetGender(currentGender);
        UpdateRace(widget->GetID());
        ResetAllWindows();
        return true;
    }
    
    
    
    if ( strcmp( widget->GetName(), "randomName" ) == 0 )
    {
        csString randomName;
        csString lastName;
        
        if ( currentGender == PSCHARACTER_GENDER_FEMALE )
        {                
            createManager->GenerateName(NAMEGENERATOR_FEMALE_FIRST_NAME, randomName,5,5);
        }   
        else
        {
           createManager->GenerateName(NAMEGENERATOR_MALE_FIRST_NAME, randomName,4,7);        
        }  
        
        createManager->GenerateName(NAMEGENERATOR_FAMILY_NAME, lastName,4,7);        

        csString upper( randomName );
        upper.Upcase();
        randomName.SetAt(0, upper.GetAt(0) );

        upper = lastName;
        upper.Upcase();
        lastName.SetAt(0, upper.GetAt(0) );

        randomName += " " + lastName;

        pawsEditTextBox* name = (pawsEditTextBox*)FindWidget("charnametext");
        name->SetText( randomName );
        name->SetCursorPosition(0);
        nameWarning = 1; // no need to warn them
    }
    
    /////////////////////////////////////////////////////    
    // END OF RACE BUTTONS
    /////////////////////////////////////////////////////            
    switch ( widget->GetID() )
    {
        case MALE_BUTTON:
            SelectGender(PSCHARACTER_GENDER_MALE);
            return true;            
        case FEMALE_BUTTON:
            SelectGender(PSCHARACTER_GENDER_FEMALE);
            return true;            
        case BACK_BUTTON:
        {
            Hide();
            pawsCharacterPickerWindow* charPicker = (pawsCharacterPickerWindow*)PawsManager::GetSingleton().FindWidget( "CharPickerWindow" );
            charPicker->Show();
            return true;
        }
        case NAMEHELP_BUTTON:
        {
            csString help;
            help = "A full name is required, the first & last name separated with a space. ";
            help += "You can use any alphabetic (A-Z) in your name, but no numbers. ";
            help += "Each name must be between 3 and 27 letters.\n";
            help += "The name should follow the rules here:\n";
            help += "http://www.planeshift.it/naming.html \n";
            help += "If your name is inappropriate, a Game Master (GM) will change it.";
            PawsManager::GetSingleton().CreateWarningBox(PawsManager::GetSingleton().Translate(help));
            return true;
        }
        case NEXT_BUTTON:
        case QUICK_BUTTON:
        {
            // Check to see if a race was selected.
            if ( race == 0 )
            {
                psSystemMessage error(0,MSG_ERROR,PawsManager::GetSingleton().Translate("Please select a race"));
                error.FireEvent();
                return true;
            }
        
            pawsEditTextBox* name = (pawsEditTextBox*)FindWidget("charnametext");

            // Check to see a name was entered.
            if ( name->GetText() == 0 )
            {
                psSystemMessage error(0,MSG_ERROR,PawsManager::GetSingleton().Translate("Please enter a full name"));
                error.FireEvent();
                return true;
            }

            csString selectedName(name->GetText());

            if ( !CheckNameForRepeatingLetters(selectedName) )
            {
                psSystemMessage error(0,MSG_ERROR,PawsManager::GetSingleton().Translate(
                                      "No more than 2 letters in a row allowed"));
                error.FireEvent();
                return true;
            }

            csString lastname,firstname;

            firstname = selectedName.Slice(0,selectedName.FindFirst(' '));
            if (selectedName.FindFirst(' ') != SIZET_NOT_FOUND)
                lastname = selectedName.Slice(selectedName.FindFirst(' ') +1,selectedName.Length());
            else
            {
                psSystemMessage error(0,MSG_ERROR,PawsManager::GetSingleton().Translate(
                                      "Please enter a full name"));
                error.FireEvent();
                return true;
            }

            // Check the firstname
            if ( firstname.Length() < 3 || !FilterName(firstname) )
            {
                psSystemMessage error(0,MSG_ERROR,PawsManager::GetSingleton().Translate(
                                      "First name is invalid"));
                error.FireEvent();
                return true;
            }

            // Check the lastname
            if ( lastname.Length() < 3 || !FilterName(lastname) )
            {
                psSystemMessage error(0,MSG_ERROR,PawsManager::GetSingleton().Translate(
                                      "Last name is invalid"));
                error.FireEvent();
                return true;
            }

            firstname = NormalizeCharacterName(firstname);
            lastname = NormalizeCharacterName(lastname);

            if (lastname.Length())
                name->SetText(firstname + " " + lastname);
            else
                name->SetText(firstname);

            // Check to see if they entered their own name.
            if (nameWarning == 0)
            {
                csString policy;
                policy =  "Before clicking next again, please make sure that your character name ";
                policy += "is not offensive and would be appropriate for a medieval setting. ";
                policy += "It must be unique and not similar to that of any real person or thing. ";
                policy += "Names in violation of this policy (www.planeshift.it/naming.html) will ";
                policy += "be changed by authorized people in-game.  ";
                policy += "The game forums can be found at:  http://www.hydlaa.com/smf/";
                PawsManager::GetSingleton().CreateWarningBox(PawsManager::GetSingleton().Translate(policy));
            
                nameWarning = 1;
                return true;                
            }

            // Set our choices in the creation manager
            createManager->SetCustomization( race->location[PSTRAIT_LOCATION_FACE][currentGender].GetSize()?race->location[PSTRAIT_LOCATION_FACE][currentGender][currentFaceChoice]->uid:0,
                                             race->location[PSTRAIT_LOCATION_HAIR_STYLE][currentGender].GetSize()?race->location[PSTRAIT_LOCATION_HAIR_STYLE][currentGender][activeHairStyle]->uid:0,
                                             race->location[PSTRAIT_LOCATION_BEARD_STYLE][currentGender].GetSize()?race->location[PSTRAIT_LOCATION_BEARD_STYLE][currentGender][currentBeardStyleChoice]->uid:0,
                                             race->location[PSTRAIT_LOCATION_HAIR_COLOR][currentGender].GetSize()?race->location[PSTRAIT_LOCATION_HAIR_COLOR][currentGender][activeHairColour]->uid:0, 
                                             race->location[PSTRAIT_LOCATION_SKIN_TONE][currentGender].GetSize()?race->location[PSTRAIT_LOCATION_SKIN_TONE][currentGender][currentSkinColour]->uid:0 );

            
            if ( widget->GetID() == NEXT_BUTTON )
            {
                createManager->GetChildhoodData();                   
                newWindow = "CharBirth";    
            }
            else
            {
                newWindow = "Paths";               
            }                

            psNameCheckMessage msg(name->GetText());
            msg.SendMessage();
        }
    }    
    
    return false;
}


void pawsCreationMain::Draw()
{
    pawsWidget::Draw();
    
    
    graphics2D->SetClipRect( 0,0, graphics2D->GetWidth(), graphics2D->GetHeight());         
    
    if ( currentGender == PSCHARACTER_GENDER_FEMALE && createManager->GetSelectedRace() != 9)
    {   
        femaleImage->Draw(GetActualWidth(363),GetActualHeight(74), GetActualWidth(82),GetActualHeight(34));        
    }
    if ( createManager->GetSelectedRace() == 9)
    {
        neutralImage->Draw(GetActualWidth(363),GetActualHeight(74), GetActualWidth(82),GetActualHeight(34));
    }
}

bool pawsCreationMain::OnChange(pawsWidget *widget)
{ 
	// If we click on the name box (and we change it), then we might get the name policy warning again
	if (widget == nameTextBox) 
	    nameWarning=0;
	return true;	
}
void pawsCreationMain::Show()
{
    // Play some music
    if(psengine->GetSoundStatus())
    {
        psengine->GetSoundManager()->OverrideBGSong("charcreation");
    }

    pawsWidget::Show();
    UpdateCP();
}

void pawsCreationMain::UpdateRace(int id)
{
    loaded = false;
    int raceCP = createManager->GetRaceCP( id );
    lastRaceID = id;                

    if ( raceCP != REQUESTING_CP && createManager->GetSelectedRace() != id )
    {
        // Reset CP only when changing the race
        createManager->SetCurrentCP( raceCP );
        UpdateCP();
    }
                
    createManager->SetRace( id );       
    race = createManager->GetRace(id);       
        
    currentGender = createManager->GetSelectedGender();            
            
    if ( lastGender != -1 )        
    {            
        currentGender = lastGender;                 
    }            
                
    createManager->SetGender( currentGender );                
    lastGender = -1;
        
    // Store the factory name for the selected model.
    factName = createManager->GetModelName( id, currentGender );
                       
    // Show the model for the selected race.
    view->Show();
    view->EnableMouseControl(true);

    CheckLoadStatus();
}

bool pawsCreationMain::CheckLoadStatus()
{
    if(!loaded)
    {
        csRef<iMeshFactoryWrapper> factory = psengine->GetLoader()->LoadFactory(factName);
        if(factory.IsValid())
        {
            view->View(factory);

            iMeshWrapper* mesh = view->GetObject();
            charApp->SetMesh(mesh);
            if (!mesh)
            {
                PawsManager::GetSingleton().CreateWarningBox(PawsManager::GetSingleton().Translate(
                                                             "Couldn't find mesh! Please run the updater"));
                return true;
            }

            csRef<iSpriteCal3DState> spstate =  scfQueryInterface<iSpriteCal3DState> (mesh->GetMeshObject());
            if (spstate)
            {
                // Setup cal3d to select random 0 velocity anims
                spstate->SetVelocity(0.0,&psengine->GetRandomGen());
            }

            currentFaceChoice = 0; 
            activeHairStyle = 0;
            currentBeardStyleChoice = 0;
            activeHairColour = 0;
            currentSkinColour = 0;

            ChangeFace(0);    
            ChangeHairColour(0);    
            SetHairStyle( activeHairStyle );    
            ChangeSkinColour( currentSkinColour );
            ChangeBeardStyle( currentBeardStyleChoice );

            view->UnlockCamera();

            GrayStyleButtons();
            loaded = true;
        }
    }

    return false;
}


bool FilterName(const char* name)
{
    if (name == NULL)
        return false;

    size_t len = strlen(name);

    if ( (strspn(name,"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ")
                != len) )
    {
        return false;
    }

    if (((int)strspn(((const char*)name)+1,
                     (const char*)"abcdefghijklmnopqrstuvwxyz") != (int)len - 1))
    {
        return false;
    }

    return true;
}

// Stolen from libsserverobjects - psCharacter
csString NormalizeCharacterName(const csString & name)
{
    csString normName = name;
    normName.Downcase();
    if (normName.Length() > 0)
        normName.SetAt(0,toupper(normName.GetAt(0)));
    return normName;
}

void pawsCreationMain::UpdateCP()
{
    int cp = createManager->GetCurrentCP();
    if (cp > 300 || cp < -300 ) // just some random values
        cp=0;

    char buff[10];
    sprintf( buff, "CP: %d", cp );
    cpPoints->SetText( buff );
}

bool CheckNameForRepeatingLetters(csString name)
{
    name.Downcase();

    char letter = ' ';
    int count = 1;

    for (size_t i=0; i<name.Length(); i++)
    {
        if (name[i] == letter)
        {
            count++;
            if (count > 2)
                return false;  // More than 2 of the same letter in a row
        }
        else
        {
            count = 1;
            letter = name[i];
        }
    }
    
    // No more than 2 of one letter in a row
    return true;
}
