#ifndef PAWS_SLOT_HEADER
#define PAWS_SLOT_HEADER


#include "paws/pawswidget.h"

class psSlotManager;
class pawsTextBox;

class pawsSlot : public pawsWidget
{
public:
    pawsSlot();
    ~pawsSlot();

    void SetDrag( bool isDragDrop ) { dragDrop = isDragDrop; }
    void SetEmptyOnZeroCount( bool emptyOnZeroCount ) { this->emptyOnZeroCount = emptyOnZeroCount; }
    bool Setup( iDocumentNode* node );
    void SetContainer( int id ) { containerID  = id; }
    
    bool OnMouseDown( int button, int modifiers, int x, int y );
    void Draw();
    void SetToolTip( const char* text );
    
    int StackCount() { return stackCount; }
    void StackCount( int newCount );

    int GetPurifyStatus();
    void SetPurifyStatus(int status);
    
    void PlaceItem( const char* imageName, const char* meshFactName, int count = 0 );
    csRef<iPawsImage> Image() { return image;}
    const char *ImageName();

    const char *GetMeshFactName()
    {
      return meshfactName;
    }
    
    const csString & SlotName() const { return slotName; }
    void SetSlotName(const csString & name) { slotName = name; }

    int ContainerID() { return containerID; };
    int ID() { return slotID; }
    void SetSlotID( int id ) { slotID = id; }

    virtual void Clear();
    bool IsEmpty() { if(!reserved) return empty; else return false; }

    void Reserve() { reserved = true; }

    void DrawStackCount(bool value);
    
    bool SelfPopulate( iDocumentNode *node);
     
    void OnUpdateData(const char *dataname,PAWSData& value);

	void ScalePurifyStatus();
       
protected:
    psSlotManager*   mgr;
    csString         meshfactName;
    csString         slotName;
    int              containerID;  
    int              slotID;
    int              stackCount;
    int              purifyStatus;
    bool empty;
    bool dragDrop;
    bool drawStackCount;
    bool reserved;		// implemented to fix dequip behaviour. Cleared on PlaceItem and Clear
    
    csRef<iPawsImage> image;
    pawsWidget* purifySign;
    pawsTextBox* stackCountLabel;
    bool handleMouseClicks;
    bool emptyOnZeroCount;      // should the slot clear itself when the stackcount hits 0 ?
};

CREATE_PAWS_FACTORY( pawsSlot );

#endif

