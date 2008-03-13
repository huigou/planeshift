/*
 * pawsSketchWindow.h - Author: Keith Fulton
 *
 * Copyright (C) 2006 Atomic Blue (info@planeshift.it, http://www.atomicblue.org)
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

#ifndef PAWS_SKETCH_WINDOW_HEADER
#define PAWS_SKETCH_WINDOW_HEADER

#include "paws/pawswidget.h"
#include "paws/pawsstringpromptwindow.h"

/**
 * A window that shows a map or picture.
 */
class pawsSketchWindow : public pawsWidget, public psClientNetSubscriber,public iOnStringEnteredAction
{
    struct SketchObject
    {
        bool selected;
        int x,y;
        csString str;
        int frame;    /// This is incremented each frame when the object is selected to make it flash

        pawsSketchWindow *parent;

        const char *GetStr() { return str; }
        virtual void SetStr(const char *s) { str=s; }
        SketchObject()  { x=0; y=0; selected=false; }
        virtual ~SketchObject() {};

        virtual bool Load(iDocumentNode *node, pawsSketchWindow *parent)=0;
        virtual void WriteXml(csString& xml) = 0;
        virtual void Draw()=0;
        virtual bool IsHit(int mouseX, int mouseY)=0;
        virtual void Select(bool _selected)         { selected=_selected;  frame=0; }
        virtual void UpdatePosition(int _x, int _y) { x = _x; y = _y;     }
    };
    struct SketchIcon : public SketchObject
    {
        csRef<iPAWSDrawable> iconImage;
        SketchIcon() : SketchObject() {}
        SketchIcon(int _x, int _y, const char *icon, pawsSketchWindow *parent);
        virtual ~SketchIcon() {};
        bool Init(int _x, int _y, const char *icon, pawsSketchWindow *parent);
        virtual bool Load(iDocumentNode *node, pawsSketchWindow *parent);
        virtual void WriteXml(csString& xml);
        virtual void Draw();
        virtual bool IsHit(int mouseX, int mouseY);
        virtual void SetStr(const char *s);
    };
    struct SketchText : public SketchObject
    {
        csString font;

        SketchText(int _x,int _y,const char *value,pawsSketchWindow *_parent)
        {
            x = _x;
            y = _y;
            str = value;
            parent = _parent;
        }
        SketchText() {}
        virtual ~SketchText() {};
        virtual bool Load(iDocumentNode *node, pawsSketchWindow *parent);
        virtual void WriteXml(csString& xml);
        virtual void Draw();
        virtual bool IsHit(int mouseX, int mouseY);
    };
    struct SketchLine : public SketchObject
    {
        int x2,y2;
        int offsetX, offsetY;
        int dragMode;

        SketchLine(int _x,int _y, int _x2, int _y2,pawsSketchWindow *_parent)
        {
            x=_x;    y=_y;
            x2=_x2; y2=_y2;
            parent = _parent;
        }
        SketchLine() {};
        virtual ~SketchLine() {};
        virtual bool Load(iDocumentNode *node, pawsSketchWindow *parent);
        virtual void WriteXml(csString& xml);
        virtual void Draw();
        virtual bool IsHit(int mouseX, int mouseY);
        virtual void UpdatePosition(int _x, int _y);
    };

protected:
    csPDelArray<SketchObject> objlist;
    size_t selectedIndex;
    int dirty;
    uint32_t currentItemID;
    csRef<iPAWSDrawable> blackBox;
    bool editMode;
    bool mouseDown;
    csStringArray iconList;
    int primCount;
    bool readOnly;
    bool stringPending;

    void DrawSketch();
    bool ParseSketch(const char *xml);
    bool ParseLimits(const char *xmlstr);

    void AddSketchText();
    void AddSketchIcon();
    void AddSketchLine();
    void RemoveSelected();
    void NextPrevIcon(int delta);
    void MoveObject(int dx, int dy);
    void ChangeSketchName();

    csString sketchName;

public:
    pawsSketchWindow();
    virtual ~pawsSketchWindow();

    bool PostSetup();

    void HandleMessage( MsgEntry* message );

    // inherited from iOnStringEnteredAction
    void OnStringEntered(const char *name,int param,const char *value);

    // inherited from iScriptableVar from pawsWidget
    double CalcFunction(const char * functionName, const double * params);


    virtual bool OnMouseDown( int button, int modifiers, int x, int y );
    virtual bool OnMouseUp( int button, int modifiers, int x, int y );
    virtual bool OnKeyDown( int keyCode, int key, int modifiers );
    virtual void Hide();
    virtual void Draw();

    // SketchObject helper functions
    iGraphics2D *GetG2D();
    void DrawBlackBox(int x, int y);
    bool IsMouseDown() { return mouseDown; }

    virtual const bool GetFocusOverridesControls() const { return true; }
};

CREATE_PAWS_FACTORY( pawsSketchWindow );


#endif 


