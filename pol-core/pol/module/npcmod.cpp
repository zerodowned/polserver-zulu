/*
History
=======
2009/09/14 MuadDib:   CreateItem() now has slot support.

Notes
=======

*/

#include "../../clib/stl_inc.h"

#ifdef _MSC_VER
#	pragma warning(disable:4786)
#endif

#include "../../clib/cfgelem.h"
#include "../../clib/clib.h"
#include "../../clib/endian.h"
#include "../../clib/fileutil.h"
#include "../../clib/logfile.h"
#include "../../clib/mlog.h"
#include "../../clib/passert.h"
#include "../../clib/random.h"
#include "../../clib/stlutil.h"
#include "../../clib/strutil.h"
#include "../../clib/unicode.h"

#include "../../bscript/berror.h"
#include "../../bscript/eprog.h"
#include "../../bscript/executor.h"
#include "../../bscript/execmodl.h"
#include "../../bscript/modules.h"
#include "../../bscript/impstr.h"

#include "../../plib/realm.h"

#include "../mobile/attribute.h"
#include "../mobile/boundbox.h"
#include "../mobile/ufacing.h"
#include "../network/client.h"
#include "../dice.h"
#include "../eventid.h"
#include "../fnsearch.h"
#include "../realms.h"
#include "../scrsched.h"
#include "../listenpt.h"
#include "../npc.h"
#include "npcmod.h"
#include "../npctmpl.h"
#include "../poltype.h"
#include "../pktout.h"
#include "../ufunc.h"
#include "../ufuncinl.h"
#include "../scrstore.h"
#include "../skilladv.h"
#include "../skills.h"
#include "../sockio.h"
#include "../ssopt.h"
#include "../uvars.h"
#include "osmod.h"
#include "../uoexec.h"
#include "uomod.h"
#include "../objtype.h"
#include "../ufunc.h"
#include "unimod.h"
#include "../uoexhelp.h"
#include "../uoscrobj.h"
#include "../watch.h"
#include "../item/weapon.h"
#include "../wrldsize.h"

NPCExecutorModule::NPCExecutorModule( Executor& ex, NPC& npc ) :
ExecutorModule( "NPC", ex ),
npcref( &npc ),
npc(npc)
{
	os_module = static_cast<OSExecutorModule*>( exec.findModule( "OS" ) );
	if (os_module == NULL)
		throw runtime_error( "NPCExecutorModule needs OS module!" );
}

NPCExecutorModule::~NPCExecutorModule()
{
	if (npc.ex == &exec)
		npc.ex = NULL;
}

NPCFunctionDef NPCExecutorModule::function_table[] =
{
	{ "Wander",                 &NPCExecutorModule::mf_Wander },
	{ "self",                   &NPCExecutorModule::mf_Self },
	{ "face",                   &NPCExecutorModule::face },
	{ "move",                   &NPCExecutorModule::move },
	{ "WalkToward",             &NPCExecutorModule::mf_WalkToward },
	{ "RunToward",              &NPCExecutorModule::mf_RunToward },
	{ "WalkAwayFrom",           &NPCExecutorModule::mf_WalkAwayFrom },
	{ "RunAwayFrom",            &NPCExecutorModule::mf_RunAwayFrom },
	{ "TurnToward",             &NPCExecutorModule::mf_TurnToward },
	{ "TurnAwayFrom",           &NPCExecutorModule::mf_TurnAwayFrom },

	{ "WalkTowardLocation",     &NPCExecutorModule::mf_WalkTowardLocation },
	{ "RunTowardLocation",      &NPCExecutorModule::mf_RunTowardLocation },
	{ "WalkAwayFromLocation",   &NPCExecutorModule::mf_WalkAwayFromLocation },
	{ "RunAwayFromLocation",    &NPCExecutorModule::mf_RunAwayFromLocation },
	{ "TurnTowardLocation",     &NPCExecutorModule::mf_TurnTowardLocation },
	{ "TurnAwayFromLocation",   &NPCExecutorModule::mf_TurnAwayFromLocation },


	{ "say",                    &NPCExecutorModule::say },
	{ "SayUC",                  &NPCExecutorModule::SayUC },
	{ "SetOpponent",            &NPCExecutorModule::mf_SetOpponent },
	{ "SetWarMode",             &NPCExecutorModule::mf_SetWarMode },
	{ "SetAnchor",              &NPCExecutorModule::mf_SetAnchor },
	{ "position",               &NPCExecutorModule::position },
	{ "facing",                 &NPCExecutorModule::facing },
	{ "IsLegalMove",            &NPCExecutorModule::IsLegalMove },
	{ "CanMove",                &NPCExecutorModule::CanMove },
	{ "getproperty",            &NPCExecutorModule::getproperty },
	{ "setproperty",            &NPCExecutorModule::setproperty },
	{ "makeboundingbox",        &NPCExecutorModule::makeboundingbox }
	// { "CreateBackpack", CreateBackpack },
	// { "CreateItem", CreateItem }
};

int NPCExecutorModule::functionIndex( const char* name )
{
	for( unsigned idx = 0; idx < arsize(function_table); idx++ )
	{
		if (stricmp( name, function_table[idx].funcname ) == 0)
			return idx;
	}
	return -1;
}

BObjectImp* NPCExecutorModule::execFunc( unsigned funcidx )
{
	return callMemberFunction(*this, function_table[funcidx].fptr)();
};

BApplicObjType bounding_box_type;

class BoundingBoxObjImp : public BApplicObj<BoundingBox>
{
public:
	BoundingBoxObjImp() : BApplicObj<BoundingBox>(&bounding_box_type) {}
	explicit BoundingBoxObjImp( const BoundingBox& b ) : BApplicObj<BoundingBox>(&bounding_box_type, b) {}
	virtual const char* typeOf() const { return "BoundingBox"; }
	virtual int typeOfInt() const { return OTBoundingBox; }	
	virtual BObjectImp* copy() const { return new BoundingBoxObjImp( value() ); }

};

/* IsLegalMove: parameters (move, bounding box)*/
BObjectImp* NPCExecutorModule::IsLegalMove()
{
	String* facing_str = static_cast<String*>(exec.getParamImp(0, BObjectImp::OTString));
	BApplicObjBase* appobj = static_cast<BApplicObjBase*>(exec.getParamImp(1, BObjectImp::OTApplicObj));
	if (facing_str == NULL || 
		appobj == NULL ||
		appobj->object_type() != &bounding_box_type)
	{
		return new BLong(0);
	}

	BApplicObj<BoundingBox>* ao_bbox = static_cast<BApplicObj<BoundingBox>*>(appobj);
	const BoundingBox& bbox = ao_bbox->value();

	UFACING facing;
	if (DecodeFacing( facing_str->value().c_str(), facing ) == false)
		return new BLong(0);

	unsigned short x, y;
	npc.getpos_ifmove( facing, &x, &y );

	return new BLong( bbox.contains( x, y ) );
}

/* CanMove: parameters (facing)*/
BObjectImp* NPCExecutorModule::CanMove()
{
	if (exec.fparams.size() == 1)
	{
		BObjectImp* param0 = exec.getParamImp(0);

		if (param0->isa( BObjectImp::OTString ))
		{
			const char* dir = exec.paramAsString(0);
			UFACING facing;

			if (DecodeFacing( dir, facing ) == false)
			{
				if (mlog.is_open())
					mlog << "Script Error in '" << scriptname() << "' PC=" << exec.PC << ": " << endl
					<< "\tCall to function npc::canmove():" << endl
					<< "\tParameter 0: Expected direction: N S E W NW NE SW SE, got " << dir << endl;
				return new BError( "Invalid facing value" );
			}

			return new BLong(npc.could_move(facing)?1:0);
		}
		else if (param0->isa( BObjectImp::OTLong ))
		{
			BLong* blong = static_cast<BLong*>(param0);
			UFACING facing = static_cast<UFACING>(blong->value() & PKTIN_02_FACING_MASK);
			return new BLong(npc.could_move(facing)?1:0);
		}
		else
		{
			if (mlog.is_open())
				mlog << "Script Error in '" << scriptname() << "' PC=" << exec.PC << ": " << endl
				<< "\tCall to function npc::canmove():" << endl
				<< "\tParameter 0: Expected direction, got datatype "
				<< BObjectImp::typestr(param0->type()) << endl;
			return new BError( "Invalid parameter type" );
		}
	}
	else
		return new BError( "Invalid parameter count" );
}

BObjectImp* NPCExecutorModule::mf_Self()
{
	return new ECharacterRefObjImp( &npc );
}

BObjectImp* NPCExecutorModule::mf_SetAnchor()
{
	xcoord x;
	ycoord y;
	long dstart, psub;
	if (getParam( 0, x ) &&
		getParam( 1, y ) &&
		getParam( 2, dstart ) &&
		getParam( 3, psub ))
	{
		if(!npc.realm->valid(x,y,0)) return new BError("Invalid Coordinates for Realm");
		if (dstart)
		{
			npc.anchor.enabled = true;
			npc.anchor.x = static_cast<unsigned short>(x);
			npc.anchor.y = static_cast<unsigned short>(y);
			npc.anchor.dstart = static_cast<unsigned short>(dstart);
			npc.anchor.psub = static_cast<unsigned short>(psub);
			return new BLong(1);
		}
		else
		{
			npc.anchor.enabled = false;
			return new BLong(1);
		}
	}
	else
	{
		return new BError( "Invalid parameter type" );
	}
}



bool NPCExecutorModule::_internal_move(UFACING facing, int run, bool is_wandering)
{
	bool success = false;
	int dir = facing;
	if (run) 
		dir |= 0x80; // FIXME HARDCODE

	if (npc.could_move(facing, is_wandering))
	{
		if (npc.move(static_cast<unsigned char>(dir))) // this could still fail, if paralyzed or frozen
		{
			npc.tellmove();
			//move was successful
			success = true;
		}
		// else, npc could move, but move failed.
	}
	//else npc could not move

	return success;
}

BObjectImp* NPCExecutorModule::move_self( UFACING facing, bool run, bool adjust_ok, bool is_wandering )
{
	bool success = false;
	int dir = facing;

	if (run)
		dir |= 0x80; // FIXME HARDCODE

	if (adjust_ok)
	{
		if(npc.use_adjustments)
		{
			for( int i = 0; i < N_ADJUST; ++i )
			{
				facing = static_cast<UFACING>((dir + adjustments[i]) & 7);

				success = _internal_move(facing, run, is_wandering);
				if(success == true)
					break;
			}
		}
		else
		{
			success = _internal_move(facing, run, is_wandering);
		}
	}
	else
	{
		if (npc.anchor_allows_move(facing) && npc.move(static_cast<unsigned char>(dir)))
		{
			npc.tellmove();
			success = true;
		}
	}

	//int base = 1000 - npc.dexterity() * 3;
	int base = 1000 - npc.run_speed * 3;
	if (base < 250) base = 250;
	os_module->SleepForMs( run ? (base/2) : base );

	//return new String( FacingStr(facing) );
	return new BLong(success?1:0);
}

BObjectImp* NPCExecutorModule::mf_Wander()
{
	u8 newfacing = 0;
	bool adjust_ok = true;

	switch( random_int( 8 ) )
	{
	case 0: case 1: case 2: case 3: case 4: case 5:
		newfacing = npc.facing;
		break;
	case 6:
		newfacing = (static_cast<int>(npc.facing)-1) & PKTIN_02_FACING_MASK;
		adjust_ok = false;
		break;
	case 7:
		newfacing = (static_cast<int>(npc.facing)+1) & PKTIN_02_FACING_MASK;
		adjust_ok = false;
		break;
	}
	return move_self( static_cast<UFACING>(newfacing), false, adjust_ok, true );
}

BObjectImp* NPCExecutorModule::face()
{
	//int params = exec.numParams();
	BObjectImp* param0 = exec.getParamImp(0);
	long flags;
	exec.getParam(1, flags);
	UFACING i_facing;

	if (param0->isa( BObjectImp::OTString ))
	{
		const char* dir = exec.paramAsString(0);

		if (DecodeFacing( dir, i_facing ) == false)
		{
			if (mlog.is_open())
				mlog << "Script Error in '" << scriptname() << "' PC=" << exec.PC << ": " << endl
				<< "\tCall to function npc::face():" << endl
				<< "\tParameter 0: Expected direction: N S E W NW NE SW SE, got " << dir << endl;
			return NULL;
		}
	}
	else if (param0->isa( BObjectImp::OTLong ))
	{
		BLong* blong = static_cast<BLong*>(param0);
		i_facing = static_cast<UFACING>(blong->value() & PKTIN_02_FACING_MASK);
	}
	else
	{
		if (mlog.is_open())
			mlog << "Script Error in '" << scriptname() << "' PC=" << exec.PC << ": " << endl
			<< "\tCall to function npc::face():" << endl
			<< "\tParameter 0: Expected direction, "
			<< ", got datatype " << BObjectImp::typestr(param0->type()) << endl;

		return NULL;
	}

	if(!npc.face(i_facing, flags))
		return new BLong(0);

	npc.on_facing_changed();
	return new BLong(i_facing);
}

BObjectImp* NPCExecutorModule::move()
{
	BObjectImp* param0 = exec.getParamImp(0);

	if (param0->isa( BObjectImp::OTString ))
	{
		const char* dir = exec.paramAsString(0);
		UFACING facing;

		if (DecodeFacing( dir, facing ) == false)
		{
			if (mlog.is_open())
				mlog << "Script Error in '" << scriptname() << "' PC=" << exec.PC << ": " << endl
				<< "\tCall to function npc::move():" << endl
				<< "\tParameter 0: Expected direction: N S E W NW NE SW SE, got " << dir << endl;
			return NULL;
		}

		return move_self( facing, false );

	}
	else if (param0->isa( BObjectImp::OTLong ))
	{
		BLong* blong = static_cast<BLong*>(param0);
		UFACING facing = static_cast<UFACING>(blong->value() & PKTIN_02_FACING_MASK);
		return move_self( facing, false );
	}
	else if (param0->isa( BObjectImp::OTApplicObj ))
	{
		BApplicObjBase* appobj = static_cast<BApplicObjBase*>(param0);
		if (appobj->object_type() == &bounding_box_type)
		{
			BApplicObj<BoundingBox>* ao_bbox = static_cast<BApplicObj<BoundingBox>*>(appobj);
			const BoundingBox& bbox = ao_bbox->value();
			UFACING facing = GetRandomFacing();

			unsigned short x, y;
			npc.getpos_ifmove( facing, &x, &y );
			if (bbox.contains( x, y ) || !bbox.contains( npc.x, npc.y ))
			{
				npc.move( static_cast<unsigned char>(facing) );
				npc.tellmove();
				os_module->SleepFor( 1 );
				return new String( FacingStr( facing ) );
			}
			else
			{
				return new String( "" );
			}
		}
		else
		{
			if (mlog.is_open())
				mlog << "Script Error in '" << scriptname() << "' PC=" << exec.PC << ": " << endl
				<< "\tCall to function npc::move():" << endl
				<< "\tParameter 0: Expected direction or bounding box, "
				<< ", got datatype " << appobj->object_type() << endl;
			return NULL;
		}
	}
	else
	{
		if (mlog.is_open())
			mlog << "Script Error in '" << scriptname() << "' PC=" << exec.PC << ": " << endl
			<< "\tCall to function npc::move():" << endl
			<< "\tParameter 0: Expected direction or bounding box, "
			<< ", got datatype " << BObjectImp::typestr(param0->type()) << endl;

		return NULL;
	}
}

BObjectImp* NPCExecutorModule::mf_WalkToward()
{
	UObject* obj;
	if (getUObjectParam( exec,0, obj ))
	{
		if (obj->ismobile())
		{
			Character* chr = static_cast<Character*>(obj);
			if (!npc.is_visible_to_me( chr ))
				return new BError( "Mobile specified cannot be seen" );
		}
		UFACING fac = direction_toward( &npc, obj );
		return move_self( fac, false, true );
	}
	else
	{
		return new BError( "Invalid parameter type" );
	}
}


BObjectImp* NPCExecutorModule::mf_RunToward()
{
	UObject* obj;
	if (getUObjectParam( exec,0, obj ))
	{
		if (obj->ismobile())
		{
			Character* chr = static_cast<Character*>(obj);
			if (!npc.is_visible_to_me( chr ))
				return new BError( "Mobile specified cannot be seen" );
		}
		return move_self( direction_toward( &npc, obj ), true, true );
	}
	else
	{
		return new BError( "Invalid parameter type" );
	}
}

BObjectImp* NPCExecutorModule::mf_WalkAwayFrom()
{
	UObject* obj;
	if (getUObjectParam( exec,0, obj ))
	{
		if (obj->ismobile())
		{
			Character* chr = static_cast<Character*>(obj);
			if (!npc.is_visible_to_me( chr ))
				return new BError( "Mobile specified cannot be seen" );
		}
		return move_self( direction_away( &npc, obj ), false, true );
	}
	else
	{
		return new BError( "Invalid parameter type" );
	}
}

BObjectImp* NPCExecutorModule::mf_RunAwayFrom()
{
	UObject* obj;
	if (getUObjectParam( exec,0, obj ))
	{
		if (obj->ismobile())
		{
			Character* chr = static_cast<Character*>(obj);
			if (!npc.is_visible_to_me( chr ))
				return new BError( "Mobile specified cannot be seen" );
		}
		return move_self( direction_away( &npc, obj ), true, true );
	}
	else
	{
		return new BError( "Invalid parameter type" );
	}
}

BObjectImp* NPCExecutorModule::mf_TurnToward()
{
	UObject* obj;
	if (getUObjectParam( exec,0, obj ))
	{
		long flags;
		exec.getParam(1, flags);
		if (obj->ismobile())
		{
			Character* chr = static_cast<Character*>(obj);
			if (!npc.is_visible_to_me( chr ))
				return new BError( "Mobile specified cannot be seen" );
		}
		UFACING facing = direction_toward( &npc, obj );
		if (facing != npc.facing)
		{
			if(!npc.face( facing, flags ))
				return new BLong(0);
			npc.on_facing_changed();
			return new BLong(1);
		}
		else
			return new BLong(0);
	}
	else
	{
		return new BError( "Invalid parameter type" );
	}
}

BObjectImp* NPCExecutorModule::mf_TurnAwayFrom()
{
	UObject* obj;
	if (getUObjectParam( exec,0, obj ))
	{
		long flags;
		exec.getParam(1, flags);
		if (obj->ismobile())
		{
			Character* chr = static_cast<Character*>(obj);
			if (!npc.is_visible_to_me( chr ))
				return new BError( "Mobile specified cannot be seen" );
		}
		UFACING facing = direction_away( &npc, obj );
		if (facing != npc.facing)
		{
			if(!npc.face( facing, flags ))
				return new BLong(0);
			npc.on_facing_changed();
			return new BLong(1);
		}
		else
			return new BLong(0);
	}
	else
	{
		return new BError( "Invalid parameter type" );
	}
}

BObjectImp* NPCExecutorModule::mf_WalkTowardLocation()
{
	xcoord x;
	ycoord y;
	if (exec.getParam( 0, x ) &&
		exec.getParam( 1, y ))
	{
		if(!npc.realm->valid(x,y,npc.z)) return new BError("Invalid Coordinates for Realm");
		UFACING fac = direction_toward( &npc, x, y );
		return move_self( fac, false, true );
	}
	else
	{
		return new BError( "Invalid parameter type" );
	}
}

BObjectImp* NPCExecutorModule::mf_RunTowardLocation()
{
	xcoord x;
	ycoord y;

	if (exec.getParam( 0, x ) &&
		exec.getParam( 1, y ))
	{
		if(!npc.realm->valid(x,y,npc.z)) return new BError("Invalid Coordinates for Realm");
		UFACING fac = direction_toward( &npc, x, y );
		return move_self( fac, true, true );
	}
	else
	{
		return new BError( "Invalid parameter type" );
	}
}

BObjectImp* NPCExecutorModule::mf_WalkAwayFromLocation()
{
	xcoord x;
	ycoord y;
	if (exec.getParam( 0, x ) &&
		exec.getParam( 1, y ))
	{
		if(!npc.realm->valid(x,y,npc.z)) return new BError("Invalid Coordinates for Realm");
		UFACING fac = direction_away( &npc, x, y );
		return move_self( fac, false, true );
	}
	else
	{
		return new BError( "Invalid parameter type" );
	}
}

BObjectImp* NPCExecutorModule::mf_RunAwayFromLocation()
{
	xcoord x;
	ycoord y;
	if (exec.getParam( 0, x ) &&
		exec.getParam( 1, y ))
	{
		if(!npc.realm->valid(x,y,npc.z)) return new BError("Invalid Coordinates for Realm");
		UFACING fac = direction_away( &npc, x, y );
		return move_self( fac, true, true );
	}
	else
	{
		return new BError( "Invalid parameter type" );
	}
}

BObjectImp* NPCExecutorModule::mf_TurnTowardLocation()
{
	xcoord x;
	ycoord y;
	if (exec.getParam( 0, x ) &&
		exec.getParam( 1, y ))
	{
		long flags;
		exec.getParam(2, flags);
		if(!npc.realm->valid(x,y,npc.z)) return new BError("Invalid Coordinates for Realm");
		UFACING fac = direction_toward( &npc, x, y );
		if (npc.facing != fac)
		{
			if(!npc.face( fac, flags ))
				return new BLong(0);
			npc.on_facing_changed();
			return new BLong(1);
		}
		else
			return new BLong(0);
	}
	else
	{
		return new BError( "Invalid parameter type" );
	}
}

BObjectImp* NPCExecutorModule::mf_TurnAwayFromLocation()
{
	xcoord x;
	ycoord y;
	if (exec.getParam( 0, x ) &&
		exec.getParam( 1, y ))
	{
		long flags;
		exec.getParam(2, flags);
		if(!npc.realm->valid(x,y,npc.z)) return new BError("Invalid Coordinates for Realm");
		UFACING fac = direction_away( &npc, x, y );
		if (npc.facing != fac)
		{
			if(!npc.face( fac, flags ))
				return new BLong(0);
			npc.on_facing_changed();
			return new BLong(1);
		}
		else
			return new BLong(0);
	}
	else
	{
		return new BError( "Invalid parameter type" );
	}
}


BObjectImp* NPCExecutorModule::say()
{
	if ( npc.squelched() )
		return new BError("NPC is squelched");
	else if ( npc.hidden() )
		npc.unhide();

	const char* text = exec.paramAsString(0);
	string texttype_str = strlower(exec.paramAsString(1));
	long doevent; exec.getParam(2, doevent);

	PKTOUT_1C talkmsg;
	talkmsg.msgtype = PKTOUT_1C_ID;
	talkmsg.source_serial = npc.serial_ext;
	talkmsg.source_graphic = npc.graphic_ext;
	talkmsg.color = ctBEu16( npc.speech_color_ );
	talkmsg.font = ctBEu16( npc.speech_font_ );
	strzcpy( talkmsg.speaker_name, npc.name().c_str(), sizeof talkmsg.speaker_name );
	strzcpy( talkmsg.text, text, sizeof talkmsg.text );
	unsigned short msglen = static_cast<unsigned short>(offsetof( PKTOUT_1C, text ) + strlen( talkmsg.text ) + 1);
	talkmsg.msglen = ctBEu16( msglen );

	if ( texttype_str == "default" )
		talkmsg.type = TEXTTYPE_NORMAL;
	else if ( texttype_str == "whisper" )
		talkmsg.type = TEXTTYPE_WHISPER;
	else if ( texttype_str == "yell" )
		talkmsg.type = TEXTTYPE_YELL;
	else
		return new BError("texttype string param must be either 'default', 'whisper', or 'yell'");

	//cout << npc.name() << " ["<<texttype_str<<"] ["<<talkmsg.textdef.type<<"] ["<<doevent<<"] \n";

	// send to those nearby
	for( unsigned cli = 0; cli < clients.size(); cli++ )
	{
		Client* client = clients[cli];
		if ( !client->ready ) 
			continue;
		else if ( !client->chr->is_visible_to_me(&npc) )
			continue;

		bool rangeok;
		if ( talkmsg.type == TEXTTYPE_WHISPER )
			rangeok = in_whisper_range(&npc, client->chr);
		else if ( talkmsg.type == TEXTTYPE_YELL )
			rangeok = in_yell_range(&npc, client->chr);
		else
			rangeok = in_say_range(&npc, client->chr);

		if ( rangeok )
			transmit( client, &talkmsg, msglen );
	}

	if ( doevent >= 1 )
		for_nearby_npcs(npc_spoke, &npc, text, strlen(text), talkmsg.type);

	return NULL;
}

BObjectImp* NPCExecutorModule::SayUC()
{
	if ( npc.squelched() )
		return new BError("NPC is squelched");
	else if ( npc.hidden() )
		npc.unhide();

	ObjArray* oText;
	const String* lang;
	long doevent;

	if (getObjArrayParam( 0, oText ) &&
		getStringParam( 2, lang ) &&
		getParam( 3, doevent))
	{
		string texttype_str = strlower(exec.paramAsString(1));
		if ( texttype_str != "default" &&
			texttype_str != "whisper" &&
			texttype_str != "yell" )
		{
			return new BError("texttype string param must be either 'default', 'whisper', or 'yell'");
		}

		unsigned textlenucc = oText->ref_arr.size();
		if ( textlenucc > 200 )
			return new BError( "Unicode array exceeds maximum size." );
		if ( lang->length() != 3 )
			return new BError( "langcode must be a 3-character code." );
		if ( !convertArrayToUC(oText, gwtext, textlenucc) )
			return new BError( "Invalid value in Unicode array." );

		const char* languc;
		languc = strupper(lang->value()).c_str();
		static PKTOUT_AE msg;
		unsigned textlen = 0, msglen;

		//textlen = wcslen((const wchar_t*)wtext) + 1;
		while( gwtext[textlen] != L'\0' )
			++textlen;
		textlen += 1; //include null terminator
		if (textlen > (sizeof msg.wtext / sizeof(msg.wtext[0])))
			textlen = (sizeof msg.wtext / sizeof(msg.wtext[0]));
		msglen = offsetof( PKTOUT_AE, wtext ) + textlen*sizeof(msg.wtext[0]);

		if (msglen > sizeof msg)
			return false;

		msg.msgtype = PKTOUT_AE_ID;
		msg.msglen = ctBEu16( msglen );
		msg.source_serial = npc.serial_ext;
		msg.source_graphic = npc.graphic_ext;
		if ( texttype_str == "yell" )
		{
			msg.type = TEXTTYPE_YELL;
		}
		else if ( texttype_str == "whisper" )
		{
			msg.type = TEXTTYPE_WHISPER;
		}
		else
		{
			msg.type = TEXTTYPE_NORMAL;
		}
		msg.color = ctBEu16( npc.speech_color_ );
		msg.font = ctBEu16( npc.speech_font_ );
		memcpy( &msg.lang, languc, sizeof msg.lang );
		memset( msg.speaker_name, '\0', sizeof msg.speaker_name );
		strzcpy( msg.speaker_name, npc.description().c_str(), sizeof msg.speaker_name );
		for(unsigned i=0; i < textlen; i++)
			msg.wtext[i] = ctBEu16(gwtext[i]);
		msg.wtext[textlen] = (u16)0L;

		for( Clients::iterator itr = clients.begin(), end = clients.end(); itr != end; ++itr )
		{
			Client* client = *itr;
			if ( !client->ready ) 
				continue;

			bool rangeok;
			if ( msg.type == TEXTTYPE_WHISPER )
				rangeok = in_whisper_range(&npc, client->chr);
			else if ( msg.type == TEXTTYPE_YELL )
				rangeok = in_yell_range(&npc, client->chr);
			else
				rangeok = in_say_range(&npc, client->chr);

			if ( rangeok )
				transmit( client, &msg, msglen );

		}

	}
	else
	{
		return new BError( "A parameter was invalid" );
	}

	//	if ( doevent >= 1 )
	//		for_nearby_npcs( npc_spoke, &npc, ntext, ntextlen, msgin->textdef.type,
	//						 wtext, msgin->lang, wtextlen);

	return NULL;
}

BObjectImp* NPCExecutorModule::position()
{
	BStruct* oa = new BStruct;

	oa->addMember( "x", new BLong( npc.x ) );
	oa->addMember( "y", new BLong( npc.y ) );
	oa->addMember( "z", new BLong( npc.z ) );

	return oa;
}

BObjectImp* NPCExecutorModule::facing()
{
	return new String( FacingStr( static_cast<UFACING>(npc.facing) ) );
}

BObjectImp* NPCExecutorModule::getproperty()
{
	const String* propname_str;
	if (exec.getStringParam( 0, propname_str ))
	{
		string val;
		if (npc.getprop( propname_str->value(), val ))
		{
			return BObjectImp::unpack( val.c_str() );
		}
		else
		{
			return new BError( "Property not found" );
		}
	}
	else
	{
		return new BError( "Invalid parameter type" );
	}
}

BObjectImp* NPCExecutorModule::setproperty()
{
	const String* propname_str;
	if (exec.getStringParam( 0, propname_str ))
	{
		BObjectImp* propval = getParamImp( 1 );
		npc.setprop( propname_str->value(), propval->pack() );
		return new BLong(1);
	}
	else
	{
		return new BError( "Invalid parameter type" );

	}
}

BObjectImp* NPCExecutorModule::CreateBackpack()
{
	// UNTESTED
	if (!npc.layer_is_equipped( LAYER_BACKPACK ))
	{
		Item* i = Item::create( UOBJ_BACKPACK );
		i->realm = npc.realm;
		auto_ptr<Item> item( i );
		item->layer = LAYER_BACKPACK;
		if (npc.equippable(item.get()))
		{
			npc.equip( item.release() );
		}
	}
	return new BLong(1);
}

BObjectImp* NPCExecutorModule::CreateItem()
{
	// UNTESTED
	const BLong* objtype = exec.getLongParam( 0 );
	if (objtype == NULL)
		return new BLong(0);

	UContainer* backpack = npc.backpack();
	if (backpack == NULL)
		return new BLong(0);

	Item* i = Item::create( static_cast<unsigned short>(objtype->value()) );
	i->realm = npc.realm;
	auto_ptr<Item> item( i );
	if (item.get() == NULL)
		return new BLong(0);

	if (!backpack->can_add( *item ))
		return new BLong(0);

	u8 slotIndex = item->slot_index();
	if ( !backpack->can_add_to_slot(slotIndex) )
		return new BLong(0);

	if ( !item->slot_index(slotIndex) )
		return new BLong(0);

	u32 serial = item->serial;

	backpack->add( item.release() );

	return new BLong(serial);
}

BObjectImp* NPCExecutorModule::makeboundingbox( /* areastring */ )
{
	String* arealist = EXPLICIT_CAST(String*, BObjectImp*)(getParamImp(0,BObjectImp::OTString));
	if (arealist == NULL)
		return new String( "" );

	BoundingBoxObjImp* bbox = new BoundingBoxObjImp;
	auto_ptr< BoundingBoxObjImp > bbox_owner( bbox );

	//    const std::string& areas = arealist->value();

	ISTRINGSTREAM is(arealist->value());

	Area a;
	// FIXME this is a terrible data format.
	while (is >> a.topleft.x >> a.topleft.y >> a.bottomright.x >> a.bottomright.y)
	{
		(*bbox)->addarea(a);
	}

	return bbox_owner.release();
}

BObjectImp* NPCExecutorModule::mf_SetOpponent()
{
	Character* chr;
	if (getCharacterParam( exec, 0, chr ) &&
		chr != &npc)
	{
		npc.set_opponent( chr );
		return new BLong(1);
	}
	else
	{
		npc.set_opponent( NULL );
		return new BLong(0);
	}
}

BObjectImp* NPCExecutorModule::mf_SetWarMode()
{
	long warmode;
	if (exec.getParam( 0, warmode ))
	{
		npc.set_warmode( warmode != 0 );
		return new BLong(1);
	}
	else
	{
		return new BLong(0);
	}
}

