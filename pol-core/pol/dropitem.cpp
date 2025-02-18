/*
History
=======
2005/05/28 Shinigami: now u can call open_trade_window without item and splitted (for use with uo::SecureTradeWin)
                      place_item_in_secure_trade_container splitted (for use with uo::MoveItemToSecureTradeWin)
2005/06/01 Shinigami: return_traded_items - added realm support
2005/11/28 MuadDib:   Added 2 more send_trade_statuses() for freshing of chr and droped_on
                      in sending of secure trade windows. This was per packet checks on
                      OSI, and works in newer clients. Unable to test in 2.0.0
2009/07/20 MuadDib:   Slot checks added where can_add() is called.
2009/07/23 MuadDib:   updates for new Enum::Packet Out ID
2009/08/06 MuadDib:   Added gotten_by code for items.
2009/08/09 MuadDib:   Refactor of Packet 0x25 for naming convention
2009/08/16 MuadDib:   find_giveitem_container(), removed passert, made it return NULL to reject move instead of a crash.
                      Added slot support to find_giveitem_container()
2009/09/03 MuadDib:	  Changes for account related source file relocation
					  Changes for multi related source file relocation
2009/09/06 Turley:    Changed Version checks to bitfield client->ClientType
2009/11/19 Turley:    removed sysmsg after CanInsert call (let scripter handle it) - Tomi


Notes
=======
FIXME: Does STW use slots with KR or newest 2d? If so, we must do slot checks there too.

*/

#include "../clib/stl_inc.h"

#include <assert.h>
#include <stdio.h>

#include "../clib/endian.h"
#include "../clib/logfile.h"
#include "../clib/passert.h"
#include "../clib/random.h"
#include "../clib/stlutil.h"
#include "../clib/strutil.h"

#include "../bscript/berror.h"

#include "../plib/realm.h"

#include "multi/boat.h"
#include "mobile/charactr.h"
#include "network/client.h"
#include "dtrace.h"
#include "getitem.h"
#include "layers.h"
#include "los.h"
#include "msghandl.h"
#include "npc.h"
#include "objtype.h"
#include "pktboth.h"
#include "pktin.h"
#include "polcfg.h"
#include "realms.h"
#include "sfx.h"
#include "sockio.h"
#include "ssopt.h"
#include "statmsg.h"
#include "storage.h"
#include "ucfg.h"
#include "ufunc.h"
#include "uofile.h"
#include "uoscrobj.h"
#include "uvars.h"
#include "uworld.h"

void send_trade_statuses( Character* chr );

bool place_item_in_container( Client *client, Item *item, UContainer *cont, u16 x, u16 y, u8 slotIndex )
{
	ItemRef itemref(item);
	if ((cont->serial == item->serial) ||
		is_a_parent( cont, item->serial ))
	{
		send_item_move_failure( client, MOVE_ITEM_FAILURE_UNKNOWN );
        return false;
	}

	if (!cont->can_add( *item ))
	{
		send_item_move_failure( client, MOVE_ITEM_FAILURE_UNKNOWN );
		send_sysmessage( client, "That item is too heavy for the container or the container is full." );
        return false;
	}
    if (!cont->can_insert_add_item( client->chr, UContainer::MT_PLAYER, item))
	{
		send_item_move_failure( client, MOVE_ITEM_FAILURE_UNKNOWN );
        return false;
	}

	if(item->orphan())
	{
		return true;
	}
	
	if (!cont->can_add_to_slot(slotIndex))
	{
		send_item_move_failure( client, MOVE_ITEM_FAILURE_UNKNOWN );
		send_sysmessage( client, "The container has no free slots available!" );
        return false;
	}
	if ( !item->slot_index(slotIndex) )
	{
		send_item_move_failure( client, MOVE_ITEM_FAILURE_UNKNOWN );
		send_sysmessage( client, "The container has no free slots available!" );
		return false;
	}

	item->set_dirty();

	client->pause();

	ConstForEach( clients, send_remove_object_if_inrange, item );

	item->x = x;
	item->y = y;

	cont->add( item );
	cont->restart_decay_timer();
    if (!item->orphan())
    {
	    send_put_in_container_to_inrange( item );
    
        cont->on_insert_add_item( client->chr, UContainer::MT_PLAYER, item );
    }

	client->restart();
    return true;
}

bool do_place_item_in_secure_trade_container( Client* client, Item* item, UContainer* cont, Character* dropon, u16 x, u16 y );
bool place_item_in_secure_trade_container( Client* client, Item* item, u16 x, u16 y )
{
    UContainer* cont = client->chr->trade_container();
    Character* dropon = client->chr->trading_with.get();
    if (dropon == NULL || dropon->client == NULL)
    {
        send_sysmessage( client, "Unable to complete trade" );
        return false;
    }
    if (!cont->can_add( *item ))
	{
        send_sysmessage( client, "That's too heavy to trade." );
		return false;
	}

	// FIXME : Add Grid Index Default Location Checks here.
	// Remember, if index fails, move to the ground. That is, IF secure trade uses
	// grid index.

    return do_place_item_in_secure_trade_container( client, item, cont, dropon, x, y );
}

BObjectImp* place_item_in_secure_trade_container( Client* client, Item* item )
{
    UContainer* cont = client->chr->trade_container();
    Character* dropon = client->chr->trading_with.get();
    if (dropon == NULL || dropon->client == NULL)
    {
        return new BError( "Unable to complete trade" );
    }
    if (!cont->can_add( *item ))
	{
        return new BError( "That's too heavy to trade." );
	}
    
	// FIXME : Add Grid Index Default Location Checks here.
	// Remember, if index fails, move to the ground.

	if (do_place_item_in_secure_trade_container( client, item, cont, dropon, 5 + static_cast<u16>(random_int( 45 )), 5 + static_cast<u16>(random_int( 45 )) ))
        return new BLong(1);
    else
        return new BError( "Something went wrong with trade window." );

}

bool do_place_item_in_secure_trade_container( Client* client, Item* item, UContainer* cont, Character* dropon, u16 x, u16 y )
{
	client->pause();

    client->chr->trade_accepted = false;
    dropon->trade_accepted = false;
    send_trade_statuses( client->chr );

	ConstForEach( clients, send_remove_object_if_inrange, item );

	item->x = x;
	item->y = y;
    item->z = 9;

	cont->add( item );
	
	send_put_in_container( client, item );
	send_put_in_container( dropon->client, item );

	send_trade_statuses( client->chr );
	send_trade_statuses( dropon->client->chr );

	client->restart();
    return true;
}

bool add_item_to_stack( Client *client, Item *item, Item *target_item )
{
	// TJ 3/18/3: Only called (so far) from place_item(), so it's only ever from
	// a player; this means that there's no need to worry about passing
	// UContainer::MT_CORE_* constants to the can_insert function (yet). :)

	ItemRef itemref(item);
	if ( (!target_item->stackable()) || (!target_item->can_add_to_self( *item )) ||
        (target_item->container && !target_item->container->can_insert_increase_stack( client->chr, UContainer::MT_PLAYER, target_item, item->getamount(), item )))
	{
		send_sysmessage(client,"Could not add item to stack.");
		send_item_move_failure( client, MOVE_ITEM_FAILURE_UNKNOWN );
		
        return false;
	}

    if (item->orphan())
        return false;

	/* At this point, we know:
		 the object types match
		 the combined stack isn't 'too large'
	   We don't know: (FIXME)
	     if a container that the target_item is in will overfill from this
	*/

	ConstForEach( clients, send_remove_object_if_inrange, item );
	
    u16 amtadded = item->getamount();

	target_item->add_to_self( item );
	update_item_to_inrange( target_item );

    if (target_item->container)
	{
        target_item->container->on_insert_increase_stack( client->chr, UContainer::MT_PLAYER, target_item, amtadded );
		target_item->container->restart_decay_timer();
	}
    return true;
}

bool place_item( Client *client, Item *item, u32 target_serial, u16 x, u16 y, u8 slotIndex )
{
	Item *target_item = find_legal_item( client->chr, target_serial );
	
    if (target_item == NULL && client->chr->is_trading())
    {
        UContainer* cont = client->chr->trade_container();
        if (target_serial == cont->serial)
        {
            return place_item_in_secure_trade_container( client, item, x, y );
        }
    }
    
    if (target_item == NULL)
	{
		send_item_move_failure( client, MOVE_ITEM_FAILURE_UNKNOWN );
        return false;
	}

	if ( (pol_distance( client->chr, target_item ) > 2) && !client->chr->can_moveanydist() )
    {
		send_item_move_failure( client, MOVE_ITEM_FAILURE_TOO_FAR_AWAY );
        return false;
    }
    if (!client->chr->realm->has_los( *client->chr, *target_item->toplevel_owner() ))
    {
        send_item_move_failure( client, MOVE_ITEM_FAILURE_OUT_OF_SIGHT );
        return false;
    }


    
	if (target_item->isa(UObject::CLASS_ITEM))
	{
        return add_item_to_stack( client, item, target_item );
	}
	else if (target_item->isa(UObject::CLASS_CONTAINER))
	{
		return place_item_in_container( client, item, static_cast<UContainer *>(target_item), x, y, slotIndex );
	}
	else
	{
		// UNTESTED CLIENT_HOLE?
		send_item_move_failure( client, MOVE_ITEM_FAILURE_UNKNOWN );
		
        return false;
	}
}

bool drop_item_on_ground( Client *client, Item *item, u16 x, u16 y, s8 z )
{
	Character* chr = client->chr;

    UMulti* multi;
    short newz;
	if (!inrangex( chr, x, y, 2) && !client->chr->can_moveanydist())
    {
        Log("Client (Character %s) tried to drop an item out of range.\n",client->chr->name().c_str());
		send_item_move_failure( client, MOVE_ITEM_FAILURE_TOO_FAR_AWAY );
        return false;
    }

    if (!chr->realm->dropheight( x, y, z, client->chr->z, &newz, &multi ))
    {
        Log("Client (Character %s) tried to drop an item at (%d,%d,%d), which is a blocked location.\n",
                client->chr->name().c_str(),
                int(x),int(y),int(z));
        return false;
    }

    LosObj att(*client->chr);
    LosObj tgt(x,y,static_cast<s8>(newz));
    if (!chr->realm->has_los( att, tgt ))
    {
        send_item_move_failure( client, MOVE_ITEM_FAILURE_OUT_OF_SIGHT );
        return false;
    }

    item->set_dirty();
	item->restart_decay_timer();
	item->x = x;
	item->y = y;
	item->z = static_cast<s8>(newz);
	if (item->realm != chr->realm)
	{
		if (item->isa( UObject::CLASS_CONTAINER ))
		{
			UContainer* cont = static_cast<UContainer*>(item);
            cont->for_each_item(setrealm, (void*)chr->realm);
		}
        setrealm(item, (void*)chr->realm);
	}
	item->container = NULL;
	item->slot_index(0);
    add_item_to_world( item );
    if (multi != NULL)
    {
        multi->register_object( item );
    }

	send_item_to_inrange( item );
    return true;
}

UContainer* find_giveitem_container( Item* item_to_add, u8 slotIndex )
{   
    StorageArea* area = storage.create_area( "GivenItems" );
    passert( area != NULL );

    for( unsigned short i = 0; i < 500; ++i )
    {
        string name = "Cont" + decint(i);
        Item* item = NULL;
        item = area->find_root_item( name );
        if (item == NULL)
        {
            item = Item::create( UOBJ_BACKPACK );
            item->setname( name );
			item->realm = find_realm(string("britannia"));
            area->insert_root_item( item );
        }   
		// Changed this from a passert to return null. 
		if ( !(item->isa( UObject::CLASS_CONTAINER )) )
			return NULL;
        UContainer* cont = static_cast<UContainer*>(item);
		if (!cont->can_add_to_slot(slotIndex))
		{
			return NULL;
		}
		if ( !item_to_add->slot_index(slotIndex) )
		{
			return NULL;
		}
		if (cont->can_add(*item_to_add))
            return cont;
    }
    return NULL;
}

void send_trade_container( Client* client,
                           Character* whos,
                           UContainer* cont )
{
	if ( client->ClientType & CLIENTTYPE_6017 )
	{
		PKTOUT_25_V2 slot_buffer;
		slot_buffer.msgtype = PKTOUT_25_V2_ID;
		slot_buffer.serial = cont->serial_ext;
		slot_buffer.graphic = cont->graphic_ext;
		slot_buffer.unk7 = 0;
		slot_buffer.amount = ctBEu16(1);
		slot_buffer.x = 0;
		slot_buffer.y = 0;
		slot_buffer.slotindex = cont->slot_index();
		slot_buffer.container_serial = whos->serial_ext;
		slot_buffer.color = cont->color_ext;
		client->transmit(&slot_buffer, sizeof slot_buffer);
	}
	else
	{
		PKTOUT_25_V1 legacy_buffer;
		legacy_buffer.msgtype = PKTOUT_25_V1_ID;
		legacy_buffer.serial = cont->serial_ext;
		legacy_buffer.graphic = cont->graphic_ext;
		legacy_buffer.unk7 = 0;
		legacy_buffer.amount = ctBEu16(1);
		legacy_buffer.x = 0;
		legacy_buffer.y = 0;
		legacy_buffer.container_serial = whos->serial_ext;
		legacy_buffer.color = cont->color_ext;
		client->transmit(&legacy_buffer, sizeof legacy_buffer);
	}
}

bool do_open_trade_window( Client* client, Item* item, Character* dropon );
bool open_trade_window( Client* client, Item* item, Character* dropon )
{
    if (!config.enable_secure_trading)
    {
        send_sysmessage( client, "Secure trading is unavailable." );
        return false;
    }

    if (!ssopt.allow_secure_trading_in_warmode)
    {
        if (dropon->warmode)
        {
            send_sysmessage( client, "You cannot trade with someone in war mode." );
            return false;
        }
        if (client->chr->warmode)
        {
            send_sysmessage( client, "You cannot trade while in war mode." );
            return false;
        }
    }
    if (dropon->is_trading())
    {
        send_sysmessage( client, "That person is already involved in a trade." );
        return false;
    }
    if (client->chr->is_trading())
    {
        send_sysmessage( client, "You are already involved in a trade." );
        return false;
    }
    if (!dropon->client)
    {
        send_sysmessage( client, "That person is already involved in a trade." );
        return false;
    }
	if( client->chr->dead() || dropon->dead() )
	{
        send_sysmessage( client, "Ghosts cannot trade items." );
        return false;		
	}
    
    return do_open_trade_window( client, item, dropon );
}

BObjectImp* open_trade_window( Client* client, Character* dropon )
{
    if (!config.enable_secure_trading)
    {
        return new BError( "Secure trading is unavailable." );
    }

    if (!ssopt.allow_secure_trading_in_warmode)
    {
        if (dropon->warmode)
        {
            return new BError( "You cannot trade with someone in war mode." );
        }
        if (client->chr->warmode)
        {
            return new BError( "You cannot trade while in war mode." );
        }
    }
    if (dropon->is_trading())
    {
        return new BError( "That person is already involved in a trade." );
    }
    if (client->chr->is_trading())
    {
        return new BError( "You are already involved in a trade." );
    }
    if (!dropon->client)
    {
        return new BError( "That person is already involved in a trade." );
    }
	if( client->chr->dead() || dropon->dead() )
	{
        return new BError( "Ghosts cannot trade items." );
	}
    
    if (do_open_trade_window( client, NULL, dropon ))
        return new BLong(1);
    else
        return new BError( "Something goes wrong." );
}

bool do_open_trade_window( Client* client, Item* item, Character* dropon )
{
    dropon->create_trade_container();
    client->chr->create_trade_container();

    dropon->trading_with.set( client->chr );
    dropon->trade_accepted = false;
    client->chr->trading_with.set( dropon );
    client->chr->trade_accepted = false;

    send_trade_container( client,         dropon,      dropon->trade_container() );
    send_trade_container( dropon->client, dropon,      dropon->trade_container() );
    send_trade_container( client,         client->chr, client->chr->trade_container() );
    send_trade_container( dropon->client, client->chr, client->chr->trade_container() );


    PKTBI_6F msg;
    msg.msgtype = PKTBI_6F_ID;
    msg.msglen = ctBEu16(sizeof msg);
    msg.action = PKTBI_6F::ACTION_INIT;
    msg.chr_serial = dropon->serial_ext;
    msg.cont1_serial = client->chr->trade_container()->serial_ext;
    msg.cont2_serial = dropon->trade_container()->serial_ext;
    msg.havename = 1;
    strzcpy( msg.name, dropon->name().c_str(), sizeof msg.name );
    client->transmit( &msg, sizeof msg );

    msg.chr_serial = client->chr->serial_ext;
    msg.cont1_serial = dropon->trade_container()->serial_ext;
    msg.cont2_serial = client->chr->trade_container()->serial_ext;
    msg.havename = 1;
    strzcpy( msg.name, client->chr->name().c_str(), sizeof msg.name );
    dropon->client->transmit( &msg, sizeof msg );
    
    if (item != NULL)
        return place_item_in_secure_trade_container( client, item, 20, 20 );
    else
        return true;
}

bool drop_item_on_mobile( Client* client, Item* item, u32 target_serial, u8 slotIndex )
{
    Character* dropon = find_character( target_serial );
    
    if (dropon == NULL) 
    {
        send_item_move_failure( client, MOVE_ITEM_FAILURE_UNKNOWN );
        return false;
    }

	if (pol_distance( client->chr, dropon ) > 2 && !client->chr->can_moveanydist())
    {
		send_item_move_failure( client, MOVE_ITEM_FAILURE_TOO_FAR_AWAY );
        return false;
    }
    if (!client->chr->realm->has_los( *client->chr,*dropon ))
    {
        send_item_move_failure( client, MOVE_ITEM_FAILURE_OUT_OF_SIGHT );
        return false;
    }
    
	if (!dropon->isa( UObject::CLASS_NPC)) 
    {
        bool res = open_trade_window( client, item, dropon );
        if (!res)
	        send_item_move_failure( client, MOVE_ITEM_FAILURE_UNKNOWN );
        return res;
    }

    NPC* npc = static_cast<NPC*>(dropon);
    if (!npc->can_accept_event( EVID_ITEM_GIVEN )) 
    {
        send_item_move_failure( client, MOVE_ITEM_FAILURE_UNKNOWN );
        return false;
    }

    UContainer* cont = find_giveitem_container(item, slotIndex);
    if (cont == NULL)
    {
        send_item_move_failure( client, MOVE_ITEM_FAILURE_UNKNOWN );
        return false;
    }

	if ( !cont->can_add_to_slot(slotIndex) || !item->slot_index(slotIndex) )
	{
        send_item_move_failure( client, MOVE_ITEM_FAILURE_UNKNOWN );
        return false;
	}

	client->pause();

	ConstForEach( clients, send_remove_object_if_inrange, item );

	u16 rx, ry;
    cont->get_random_location( &rx, &ry );

    item->set_dirty();
    item->container = cont;
	item->x = rx;
	item->y = ry;

	cont->add_at_random_location( item );

    npc->send_event( new ItemGivenEvent( client->chr, item, npc ) );
	
	client->restart();

    return true;

}

// target_serial should indicate a character, or a container, but not a pile.
bool drop_item_on_object( Client *client, Item *item, u32 target_serial, u8 slotIndex )
{
	ItemRef itemref(item); 
	UContainer *cont = NULL;
	if (IsCharacter( target_serial )) 
	{
		if (target_serial == client->chr->serial) 
		{
			cont = client->chr->backpack();
		}
		else
		{
            return drop_item_on_mobile( client, item, target_serial, slotIndex );
		}
	}
	
	if (cont == NULL)
	{
		cont = find_legal_container( client->chr, target_serial );
	}

	if (cont == NULL)
	{
		send_item_move_failure( client, MOVE_ITEM_FAILURE_UNKNOWN );
        return false;
	}
	if (pol_distance( client->chr, cont ) > 2 && !client->chr->can_moveanydist())
    {
		send_item_move_failure( client, MOVE_ITEM_FAILURE_TOO_FAR_AWAY );
        return false;
    }
    if (!client->chr->realm->has_los( *client->chr, *cont->toplevel_owner() ))
    {
        send_item_move_failure( client, MOVE_ITEM_FAILURE_OUT_OF_SIGHT );
        return false;
    }

    if (item->stackable())
    {
        for( UContainer::const_iterator itr = cont->begin(); itr != cont->end(); ++itr )
        {
            Item* exitem = GET_ITEM_PTR( itr );
            if ( exitem->can_add_to_self( *item ) )
            {
				if (cont->can_add( *item ))
				{
                    if(cont->can_insert_increase_stack( client->chr, UContainer::MT_PLAYER, exitem, item->getamount(), item ))
					{
						if(item->orphan())
						{
							return true;
						}
                        u16 amtadded = item->getamount();
						exitem->add_to_self( item );
						update_item_to_inrange( exitem );

                        cont->on_insert_increase_stack( client->chr, UContainer::MT_PLAYER, exitem, amtadded );

                        return true;
					}
				}
				return false;
            }
        }
    }

    u16 rx, ry;
    cont->get_random_location( &rx, &ry );

	return place_item_in_container( client, item, cont, rx, ry, slotIndex ); 
}

/* DROP_ITEM messages come in a couple varieties:
	
	1)	Dropping an item on another object, or a person:
			item_serial: serial number of item to drop
			x: 0xFFFF
			y: 0xFFFF
			z: 0
			target_serial: serial number of item or character to drop on.

	2)  Dropping an item on the ground:
			item_serial: serial number of item to drop
			x,y,z: position
			target_serial: 0xFFFFFFFF

    3)	Placing an item in a container, or in an existing pile:
			item_serial: serial number of item to drop
			x: x-position
			y: y-position
			z: 0
			target_serial: serial number of item or character or pile to drop on.
*/

/*
	Name:       drop_item
	Details:    Original version of packet is supported by this function.
	Access:     public 
	Qualifier:	
	Parameter:	Client * client
	Parameter:	PKTIN_08_V1 * msg
*/
void drop_item( Client *client, PKTIN_08_V1 *msg )
{
	u32 item_serial = cfBEu32( msg->item_serial );
	u16 x = cfBEu16( msg->x );
	u16 y = cfBEu16( msg->y );
	s8 z = msg->z;
	u32 target_serial = cfBEu32( msg->target_serial );

	Item *item = client->chr->gotten_item;
	if (item == NULL)
	{
		Log( "Character %08lX tried to drop item %08lX, but had not gotten an item.\n",
			     client->chr->serial, 
				 item_serial);
		return;
	}
	if (item->serial != item_serial)
	{
		Log( "Character %08lX tried to drop item %08lX, but instead had gotten item %08lX.\n",
			     client->chr->serial, 
				 item_serial,
				 item->serial);
		item->gotten_by = NULL;
		return;
	}
    client->chr->gotten_item->inuse( false );
	client->chr->gotten_item->gotten_by = NULL;
	client->chr->gotten_item = NULL;


    bool res;
	if (target_serial == 0xFFffFFffLu)
	{
		res = drop_item_on_ground( client, item, x, y, z );
	}
	else if (x == 0xFFFF) 
	{
		res = drop_item_on_object( client, item, target_serial, 0 );
	}
	else
	{
		res = place_item( client, item, target_serial, x, y, 0);
	}

    if (!item->orphan())
    {
        if (!res)
        {
		    undo_get_item( client->chr, item );
        }
	    item->inuse(false);
	    item->is_gotten(false);
	    item->gotten_by = NULL;
    }
	send_full_statmsg( client, client->chr );
}

MESSAGE_HANDLER( PKTIN_08_V1, drop_item );


/*
	Name:       drop_item_v2
	Details:    This is used for the version of the packet introduced in clients 6.0.1.7 2D and UO:KR+ to support Slots
	Access:     public 
	Qualifier:	
	Parameter:	Client * client
	Parameter:	PKTIN_08_V2 * msg
*/
void drop_item_v2( Client *client, PKTIN_08_V2 *msg )
{
	u32 item_serial = cfBEu32( msg->item_serial );
	u16 x = cfBEu16( msg->x );
	u16 y = cfBEu16( msg->y );
	s8 z = msg->z;
	u8 slotIndex = msg->slotindex;
	u32 target_serial = cfBEu32( msg->target_serial );

	Item *item = client->chr->gotten_item;
	if (item == NULL)
	{
		Log( "Character %08lX tried to drop item %08lX, but had not gotten an item.\n",
			     client->chr->serial, 
				 item_serial);
		return;
	}
	if (item->serial != item_serial)
	{
		Log( "Character %08lX tried to drop item %08lX, but instead had gotten item %08lX.\n",
			     client->chr->serial, 
				 item_serial,
				 item->serial);
		item->gotten_by = NULL;
		return;
	}
	client->chr->gotten_item->inuse( false );
	client->chr->gotten_item->gotten_by = NULL;
	client->chr->gotten_item = NULL;

    bool res;
	if (target_serial == 0xFFffFFffLu)
	{
		res = drop_item_on_ground( client, item, x, y, z );
	}
	else if (x == 0xFFFF) 
	{
		res = drop_item_on_object( client, item, target_serial, slotIndex );
	}
	else
	{
		res = place_item( client, item, target_serial, x, y, slotIndex );
	}

    if (!item->orphan())
    {
        if (!res)
        {
		    undo_get_item( client->chr, item );
        }
	    item->inuse(false);
	    item->is_gotten(false);
	    item->gotten_by = NULL;
    }

	PKTOUT_29 drop_msg;
	drop_msg.msgtype = PKTOUT_29_ID;
	client->transmit(&drop_msg, sizeof drop_msg);

	send_full_statmsg( client, client->chr );
}
MESSAGE_HANDLER_V2( PKTIN_08_V2, drop_item_v2 );

void return_traded_items( Character* chr )
{
    UContainer* cont = chr->trade_container();
    UContainer* bp = chr->backpack();
    if (cont == NULL || bp == NULL)
        return;

    UContainer::Contents tmp;
    cont->extract( tmp );
    while (!tmp.empty())
    {
        Item* item = ITEM_ELEM_PTR( tmp.back() );
        tmp.pop_back();
        item->container = NULL;
        item->layer = 0;

		ItemRef itemref(item);
		if (bp->can_add( *item ) && bp->can_insert_add_item( chr, UContainer::MT_CORE_MOVED, item ) )
        {
			if(item->orphan()) 
			{
				continue;
			}
			u8 newSlot = 1;
			if ( !bp->can_add_to_slot(newSlot) || !item->slot_index(newSlot) )
			{
				item->set_dirty();
				item->x = chr->x;
				item->y = chr->y;
				item->z = chr->z;
				add_item_to_world( item );
				register_with_supporting_multi( item );
				move_item( item, item->x, item->y, item->z, NULL );
				return;
			}
			bp->add_at_random_location( item );
			if (chr->client)
				send_put_in_container( chr->client, item );
			bp->on_insert_add_item( chr, UContainer::MT_CORE_MOVED, item );
        }
        else
        {
            item->set_dirty();
            item->x = chr->x;
            item->y = chr->y;
            item->realm = chr->realm;
            add_item_to_world( item );
            register_with_supporting_multi( item );
            move_item( item, chr->x, chr->y, chr->z, NULL );
        }
    }

}


void cancel_trade( Character* chr1 )
{
    Character* chr2 = chr1->trading_with.get();

    return_traded_items( chr1 );
    chr1->trading_with.clear();

    if (chr1->client)
    {
        PKTBI_6F msg;
        msg.msgtype = PKTBI_6F_ID;
        msg.msglen = ctBEu16( offsetof(PKTBI_6F,name) );
        msg.action = PKTBI_6F::ACTION_CANCEL;
        msg.chr_serial = chr1->trade_container()->serial_ext;
        msg.cont1_serial = 0;
        msg.cont2_serial = 0;
        msg.havename = 0;
        transmit( chr1->client, &msg, offsetof(PKTBI_6F,name) );
		send_full_statmsg( chr1->client, chr1 );
	}

    if (chr2)
    {
        return_traded_items( chr2 );
        chr2->trading_with.clear();
        if (chr2->client)
        {
            PKTBI_6F msg;
            msg.msgtype = PKTBI_6F_ID;
            msg.msglen = ctBEu16( offsetof(PKTBI_6F,name) );
            msg.action = PKTBI_6F::ACTION_CANCEL;
            msg.chr_serial = chr2->trade_container()->serial_ext;
            msg.cont1_serial = 0;
            msg.cont2_serial = 0;
            msg.havename = 0;
            transmit( chr2->client, &msg, offsetof(PKTBI_6F,name) );
			send_full_statmsg( chr2->client, chr2 );
		}
    }
}

void send_trade_statuses( Character* chr )
{
    unsigned long stat1 = chr->trade_accepted?1:0;
    unsigned long stat2 = chr->trading_with->trade_accepted?1:0;

    PKTBI_6F msg;
    msg.msgtype = PKTBI_6F_ID;
    msg.msglen = ctBEu16( offsetof(PKTBI_6F,name) );
    msg.action = PKTBI_6F::ACTION_STATUS;

    msg.chr_serial = chr->trade_container()->serial_ext;
    msg.cont1_serial = ctBEu32( stat1 );
    msg.cont2_serial = ctBEu32( stat2 );
    msg.havename = 0;
    transmit( chr->client, &msg, offsetof(PKTBI_6F,name) );

    msg.chr_serial = chr->trading_with->trade_container()->serial_ext;
    msg.cont1_serial = ctBEu32( stat2 );
    msg.cont2_serial = ctBEu32( stat1 );
    transmit( chr->trading_with->client, &msg, offsetof(PKTBI_6F,name) );
}

void change_trade_status( Character* chr, bool set )
{
    chr->trade_accepted = set;
    send_trade_statuses( chr );
    if (chr->trade_accepted && 
        chr->trading_with->trade_accepted)
        
    {
        UContainer* cont0 = chr->trade_container();
        UContainer* cont1 = chr->trading_with->trade_container();
        if (cont0->can_swap( *cont1 ))
        {
            cont0->swap( *cont1 );
            cancel_trade( chr );
        }
        else
        {
            Log( "Can't swap trade containers: ic0=%ld,w0=%ld, ic1=%ld,w1=%ld\n",
                    cont0->item_count(),
                    cont0->weight(),
                    cont1->item_count(),
                    cont1->weight());
        }
    }
}

void handle_secure_trade_msg( Client* client, PKTBI_6F* msg )
{
    if (!client->chr->is_trading())
        return;
    switch( msg->action )
    {
        case PKTBI_6F::ACTION_CANCEL:
            dtrace(5) << "Cancel secure trade" << endl;
            cancel_trade( client->chr );
            break;

        case PKTBI_6F::ACTION_STATUS:
            bool set;
            set = msg->cont1_serial?true:false;
            if (set)
                dtrace(5) << "Set secure trade indicator" << endl;
            else
                dtrace(5) << "Clear secure trade indicator" << endl;
            change_trade_status( client->chr, set );
            break;
        
        default:
            dtrace(5) << "Unknown secure trade action: " << (int)msg->action << endl;
            break;
    }
}
MESSAGE_HANDLER_VARLEN( PKTBI_6F, handle_secure_trade_msg );

void cancel_all_trades()
{
    for( Clients::iterator itr = clients.begin(); itr != clients.end(); ++itr )
    {
        Client* client = (*itr);
        if (client->ready && client->chr)
        {
            Character* chr = client->chr;
            if (chr->is_trading())
                cancel_trade( chr );
        }
    }
}
