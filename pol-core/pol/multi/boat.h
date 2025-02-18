/*
History
=======
2005/04/02 Shinigami: move_offline_mobiles - added realm param
2009/09/03 MuadDib:	  Changes for account related source file relocation
                      Changes for multi related source file relocation

Notes
=======

*/


#ifndef BOAT_H
#define BOAT_H

#include "multi.h"
#include "../poltype.h"
#include "../reftypes.h"

struct BoatShape;
class MultiDef;
class BObjectImp;
class Realm;

class UBoat : public UMulti
{
    typedef UMulti base;

    class BoatContext
    {
        const MultiDef& mdef;
        unsigned short x;
        unsigned short y;

        explicit BoatContext( const UBoat& ub ) : mdef(ub.multidef()), x(ub.x), y(ub.y) {};
        friend class UBoat;
		BoatContext & operator=( const BoatContext & ) { return *this; }
    };

public:
    virtual UBoat* as_boat();

    bool move( UFACING dir );
	bool move_xy( unsigned short x, unsigned short y, long flags, Realm* oldrealm);

    enum RELATIVE_DIR { NO_TURN, RIGHT, AROUND, LEFT };
    bool turn( RELATIVE_DIR dir );

    virtual void register_object( UObject* obj );
    UFACING boat_facing() const;
    
    void cleanup_deck();
    bool hold_empty() const;
    bool deck_empty() const;
    bool has_offline_mobiles() const;
    void remove_orphans();
    void destroy_components();
    void regself();
    void unregself();

    static BObjectImp* scripted_create( const ItemDesc& descriptor, u16 x, u16 y, s8 z, Realm* realm, long flags );

    // these must match EXTOBJ_TILLERMAN etc from objtype.h
    enum BOAT_COMPONENT { 
        COMPONENT_TILLERMAN = 0,  
        COMPONENT_PORT_PLANK = 1,
        COMPONENT_STARBOARD_PLANK = 2,
        COMPONENT_HOLD = 3,
        COMPONENT__COUNT
    };
    static const char* component_names[ COMPONENT__COUNT ];
    virtual BObjectImp* make_ref();
	static bool navigable( const MultiDef&, unsigned short x, unsigned short y, short z, Realm* realm);
	void realm_changed();
	void adjust_traveller_z(s8 delta_z);

protected:
    void move_travellers( enum UFACING facing, const BoatContext& oldlocation, unsigned short x = USHRT_MAX, unsigned short y = USHRT_MAX, Realm* oldrealm = NULL );
    void turn_travellers( RELATIVE_DIR dir, const BoatContext& oldlocation );
    void turn_traveller_coords( Character* chr, RELATIVE_DIR dir );
    static bool on_ship( const BoatContext& bc, const UObject* obj );
    void move_offline_mobiles( xcoord x, ycoord y, zcoord z, Realm* realm );
    const MultiDef& multi_ifturn( RELATIVE_DIR dir );
    unsigned short graphic_ifturn( RELATIVE_DIR dir );


    void do_tellmoves();
    const BoatShape& boatshape() const;
    void rescan_components();
    void transform_components( const BoatShape& old_boatshape, Realm* oldrealm );

    explicit UBoat( const ItemDesc& descriptor );
    virtual void readProperties( ConfigElem& elem );
    virtual void printProperties( std::ostream& os ) const;
    friend class UMulti;
    
    virtual class BObjectImp* get_script_member( const char *membername ) const;
    virtual class BObjectImp* get_script_member_id( const int id ) const; ///id test
    virtual class BObjectImp* script_method( const char *methodname, Executor& ex );
    virtual class BObjectImp* script_method_id( const int id, Executor& ex );
    virtual bool script_isa( unsigned isatype ) const;
    BObjectImp* items_list() const;
    BObjectImp* mobiles_list() const;

    friend class EUBoatRefObjImp;
private:
    void create_components();
    typedef UObjectRef Traveller;
    typedef std::vector< Traveller > Travellers;
    Travellers travellers_;

    typedef ItemRef Component;
    Component components_[COMPONENT__COUNT];
};

BObjectImp* destroy_boat( UBoat* boat );
void send_boat_to_inrange( const UBoat* item );

bool BoatShapeExists( u16 graphic );
void clean_boatshapes();

#endif
