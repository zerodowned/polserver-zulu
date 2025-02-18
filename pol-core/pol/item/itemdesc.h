/*
History
=======


Notes
=======

*/

#ifndef ITEMDESC_H
#define ITEMDESC_H

#include <map>
#include <string>
#include <vector>


#include "../../clib/rawtypes.h"
#include "../uobject.h"
#include "../proplist.h"
#include "../dice.h"

class BStruct;
class ConfigElem;
class Package;
class ResourceDef;
class ExportScript;

#include "../scrdef.h"

struct ResourceComponent
{
    ResourceDef* rd;
    unsigned amount;

    ResourceComponent( const std::string& rname, unsigned amount );
};

class ItemDesc
{
public:
    enum Type { 
        ITEMDESC, 
        CONTAINERDESC, 
        DOORDESC, 
        WEAPONDESC, 
        ARMORDESC, 
        BOATDESC,
        HOUSEDESC,
        SPELLBOOKDESC,
        SPELLSCROLLDESC,
        MAPDESC
    };

    static ItemDesc* create( ConfigElem& elem, const Package* pkg );

    ItemDesc( u16 objtype, ConfigElem& elem, Type type, const Package* pkg );
    explicit ItemDesc( Type type );
    virtual ~ItemDesc();
    virtual void PopulateStruct( BStruct* descriptor ) const;
    std::string objtype_description() const;
    bool default_movable() const;

    Type type;
    
    const Package* pkg;
    std::string objtypename;

    u16 objtype;
    u16 graphic;
    u16 color;
    //u16 weight;
    u8 facing;
    int weightmult;
    int weightdiv;
    std::string desc;
    std::string tooltip;
    ScriptDef walk_on_script;
    ScriptDef on_use_script;
    std::string equip_script;
    std::string unequip_script;
    ScriptDef control_script;
    ScriptDef create_script;
    ScriptDef destroy_script;
    bool requires_attention;
    bool lockable;
    unsigned long vendor_sells_for;
    unsigned long vendor_buys_for;
    unsigned decay_time;
    enum Movable { UNMOVABLE, MOVABLE, DEFAULT } movable;
    unsigned short doubleclick_range;
	bool use_requires_los; //DAVE 11/24
	bool ghosts_can_use; //DAVE 11/24
    bool can_use_while_paralyzed;
    bool can_use_while_frozen;
    bool newbie;
    bool invisible;
    bool decays_on_multis;
    bool blocks_casting_if_in_hand;
    unsigned short base_str_req;
	unsigned short stack_limit;    

	Dice resist_dice;

	Resistances element_resist;
	ElementDamages element_damage;

    std::vector<ResourceComponent> resources;

    PropertyList props;
	std::set<std::string> ignore_cprops;  //dave added 1/26/3

    ExportScript* method_script;

    void unload_scripts();
};

class ContainerDesc : public ItemDesc
{
    typedef ItemDesc base;
public:
    ContainerDesc( u16 objtype, ConfigElem& elem, const Package* pkg );
    virtual void PopulateStruct( BStruct* descriptor ) const;

    // string name;
    // u16 objtype;
    u16 gump;
    u16 minx, maxx;
    u16 miny, maxy;

    u16 max_weight;
    u16 max_items;

	u8 max_slots;

    ScriptDef can_insert_script;
    ScriptDef on_insert_script;
    ScriptDef can_remove_script;
    ScriptDef on_remove_script;
};

class DoorDesc : public ItemDesc
{
    typedef ItemDesc base;
public:
    DoorDesc( u16 objtype, ConfigElem& elem, const Package* pkg );
	virtual void PopulateStruct( BStruct* descriptor ) const;
    int xmod;
    int ymod;
	u16 open_graphic;    
};

class SpellbookDesc : public ContainerDesc
{
    typedef ContainerDesc base;
public:
    SpellbookDesc( u16 objtype, ConfigElem& elem, const Package* pkg );
	virtual void PopulateStruct( BStruct* descriptor ) const;

    std::string spelltype;
};

class SpellScrollDesc : public ItemDesc
{
    typedef ItemDesc base;
public:
    SpellScrollDesc( u16 objtype, ConfigElem& elem, const Package* pkg );
	virtual void PopulateStruct( BStruct* descriptor ) const;

    std::string spelltype;
};

class MultiDesc : public ItemDesc
{
    typedef ItemDesc base;
public:
    MultiDesc( u16 objtype, ConfigElem& elem, Type type, const Package* pkg );
	virtual void PopulateStruct( BStruct* descriptor ) const;
    u16 multiid;
};

class BoatDesc : public MultiDesc
{
    typedef MultiDesc base;
public:
    BoatDesc( u16 objtype, ConfigElem& elem, const Package* pkg );
	virtual void PopulateStruct( BStruct* descriptor ) const;
};

class HouseDesc : public MultiDesc
{
    typedef MultiDesc base;
public:
    HouseDesc( u16 objtype, ConfigElem& elem, const Package* pkg );
	virtual void PopulateStruct( BStruct* descriptor ) const;
};

class MapDesc : public ItemDesc
{
    typedef ItemDesc base;
public:
    MapDesc( u16 objtype, ConfigElem& elem, const Package* pkg );
	virtual void PopulateStruct( BStruct* descriptor ) const;

    bool editable;
};


const ItemDesc& find_itemdesc( unsigned short objtype );
const ContainerDesc& find_container_desc( u16 objtype );
const DoorDesc& fast_find_doordesc( u16 objtype );
const MultiDesc& find_multidesc( u16 objtype );

extern ItemDesc empty_itemdesc;
extern ItemDesc temp_itemdesc;

//extern std::map< unsigned short, ItemDesc > itemdesc;
const int N_ITEM_DESC = 0x10000;
extern bool has_walkon_script_[N_ITEM_DESC];

inline bool has_walkon_script( u16 objtype )
{
    return has_walkon_script_[objtype];
}

extern bool dont_save_itemtype[N_ITEM_DESC];
unsigned short getgraphic( unsigned short objtype );
unsigned short getcolor( unsigned short objtype );

unsigned short get_objtype_byname( const char* name );
unsigned short get_objtype_from_string( const std::string& str );
bool objtype_is_lockable( u16 objtype );

void load_itemdesc( ConfigElem& elem );
void fillin_itemdesc_table();

typedef std::map<unsigned short, unsigned short> OldObjtypeConversions;
extern OldObjtypeConversions old_objtype_conversions;

const ItemDesc* CreateItemDescriptor( BStruct* itemdesc_struct );
extern vector< ItemDesc* > dynamic_item_descriptors;

#endif
