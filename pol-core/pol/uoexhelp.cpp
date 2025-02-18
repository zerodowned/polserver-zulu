/*
History
=======
2005/11/26 Shinigami: changed "strcmp" into "stricmp" to suppress Script Errors
2009/12/02 Turley:    added config.max_tile_id - Tomi

Notes
=======

*/

#include "../clib/stl_inc.h"
#include "../clib/stlutil.h"

#include "../bscript/berror.h"
#include "../bscript/execmodl.h"
#include "../bscript/executor.h"
#include "../bscript/impstr.h"
#include "../bscript/objmethods.h"

#include "../clib/mlog.h"
#include "../clib/logfile.h"
#include "../clib/strutil.h"

#include "fnsearch.h"
#include "item/itemdesc.h"
#include "polcfg.h"
#include "ssopt.h"
#include "uoscrobj.h"
#include "ufunc.h"
#include "uoexhelp.h"

#include "../../lib/zlib/zlib.h"


bool getCharacterParam( Executor& exec, unsigned param, Character*& chrptr )
{
    BObjectImp* imp = exec.getParamImp( param );
    if (imp == NULL)
    {
        exec.setFunctionResult( new BError( "Missing parameter " + decint(param) ) );
        return false;
    }
    else if (imp->isa( BObjectImp::OTApplicObj ))
    {
	    BApplicObjBase* aob = explicit_cast<BApplicObjBase*, BObjectImp*>(imp);

	    if ((aob != NULL) && (aob->object_type() == &echaracterrefobjimp_type))
	    {
            ECharacterRefObjImp* chrref_imp = 
                    explicit_cast<ECharacterRefObjImp*,BApplicObjBase*>(aob);
            
            chrptr = chrref_imp->value().get();
            
            if (chrptr->orphan())
            {
                exec.setFunctionResult( new BError( "Mobile has been destroyed" ) );
                return false;
            }

            if (chrptr->logged_in ||
                chrref_imp->offline_access_ok())
            {
                return true;
            }
            else
            {
                exec.setFunctionResult( new BError( "Mobile is offline" ) );
                return false;
            }
	    }
	    else
	    {
            // FIXME: log error
		    return false;
	    }
    }
    else if (imp->isa( BObjectImp::OTLong ))
    {
        BLong* pchar_serial = explicit_cast<BLong*,BObjectImp*>(imp);

        unsigned long serial = pchar_serial->value();
        if (IsItem(serial) || serial == 0)
        {
            exec.setFunctionResult( new BError( "Serial refers to an Item, or is zero" ) );
            return false;
        }

        chrptr = system_find_mobile( serial );
    
        if (chrptr != NULL)
        {
            if (chrptr->logged_in)
            {
                return true;
            }
            else
            {
                exec.setFunctionResult( new BError( "Mobile is offline" ) );
                return false;
            }
        }
        else
        {
            exec.setFunctionResult( new BError( "Mobile does not exist" ) );
            return false;
        }
    }
    else
    {
        // FIXME: log error
        return false;
    }
}

bool getItemParam( Executor& exec, unsigned param, Item*& itemptr )
{
    BObjectImp* imp = exec.getParamImp( param );
    if (imp == NULL)
    {
        return false;
    }
    else if (imp->isa( BObjectImp::OTApplicObj ))
    {
	    BApplicObjBase* aob = explicit_cast<BApplicObjBase*, BObjectImp*>(imp);

	    if ((aob != NULL) && (aob->object_type() == &eitemrefobjimp_type))
	    {
            EItemRefObjImp* itemref_imp = 
                    explicit_cast<EItemRefObjImp*,BApplicObjBase*>(aob);
            
            itemptr = itemref_imp->value().get();
            return (!itemptr->orphan());
        }
	    else
	    {
            // FIXME: log error
		    return false;
	    }
    }
    else if (imp->isa( BObjectImp::OTLong ))
    {
        BLong* pitem_serial = explicit_cast<BLong*,BObjectImp*>(imp);
        unsigned long serial = pitem_serial->value();

        if ( IsCharacter( serial ) || serial == 0)
            return false;
        
        itemptr = system_find_item( serial );
    
        return (itemptr != NULL);
    }
    else
    {
        // FIXME: log error
        return false;
    }
}

bool getUBoatParam( Executor& exec, unsigned param, UBoat*& boatptr )
{
    BObjectImp* imp = exec.getParamImp( param );
    if (imp == NULL)
    {
        return false;
    }
    else if (imp->isa( BObjectImp::OTApplicObj ))
    {
	    BApplicObjBase* aob = explicit_cast<BApplicObjBase*, BObjectImp*>(imp);

	    if (aob->object_type() == &euboatrefobjimp_type)
	    {
            EUBoatRefObjImp* boatref_imp = 
                    explicit_cast<EUBoatRefObjImp*,BApplicObjBase*>(aob);
            
            boatptr = boatref_imp->value().get();
            return (!boatptr->orphan());
        }
        else if (aob->object_type() == &eitemrefobjimp_type)
        {
            EItemRefObjImp* itemref_imp = 
                    explicit_cast<EItemRefObjImp*,BApplicObjBase*>(aob);
            
            Item* item = itemref_imp->value().get();
            if (item->isa( UObject::CLASS_MULTI ))
            {
                UMulti* multi = static_cast<UMulti*>(item);
                boatptr = multi->as_boat();
                if (boatptr == NULL)
                    return false;
                else
                    return (!boatptr->orphan());

            }
            else
            {
                return false;
            }
            
        }
	    else
	    {
            // FIXME: log error
		    return false;
	    }
    }
    else if (imp->isa( BObjectImp::OTLong ))
    {
        BLong* pitem_serial = explicit_cast<BLong*,BObjectImp*>(imp);

        UMulti* multi = system_find_multi( pitem_serial->value() );
        if (multi)
            boatptr = multi->as_boat();
    
        return (boatptr != NULL);
    }
    else
    {
        // FIXME: log error
        return false;
    }
}




bool getMultiParam( Executor& exec, unsigned param, UMulti*& multiptr )
{
    BObjectImp* imp = exec.getParamImp( param );
    if (imp == NULL)
    {
        return false;
    }
    else if (imp->isa( BObjectImp::OTApplicObj ))
    {
	    BApplicObjBase* aob = explicit_cast<BApplicObjBase*, BObjectImp*>(imp);

	    if (aob->object_type() == &emultirefobjimp_type)
	    {
            EMultiRefObjImp* multiref_imp = 
                    explicit_cast<EMultiRefObjImp*,BApplicObjBase*>(aob);
            
            multiptr = multiref_imp->value().get();
            return (!multiptr->orphan());
        }
	    else if (aob->object_type() == &euboatrefobjimp_type)
	    {
            EUBoatRefObjImp* boatref_imp = 
                    explicit_cast<EUBoatRefObjImp*,BApplicObjBase*>(aob);
            
            multiptr = boatref_imp->value().get();
            return (!multiptr->orphan());
        }
	    else
	    {
            // FIXME: log error
		    return false;
	    }
    }
    else if (imp->isa( BObjectImp::OTLong ))
    {
        BLong* pitem_serial = explicit_cast<BLong*,BObjectImp*>(imp);

        multiptr = system_find_multi( pitem_serial->value() );
    
        return (multiptr != NULL);
    }
    else
    {
        // FIXME: log error
        return false;
    }
}

bool getUObjectParam( Executor& exec, unsigned param, UObject*& objptr )
{
    Item* item = NULL;
    Character* chr = NULL;
    UMulti* multi = NULL;

    if (getCharacterParam( exec, param, chr ))
    {
        objptr = chr;
        return true;
    }
    else if (getItemParam( exec, param, item ))
    {
        objptr = item;
        return true;
    }
    else if (getMultiParam( exec, param, multi ))
    {
        objptr = multi;
        return true;
    }
    else
    {
        return false;
    }
}

bool getObjtypeParam( Executor& exec, unsigned param, unsigned short& objtype )
{
    BObjectImp* imp = exec.getParamImp( param );
    if (imp == NULL)
    {
        return false;
    }
    unsigned long objtype_long = 0;

    if (imp->isa( BObjectImp::OTLong ))
    {
        BLong* plong = explicit_cast<BLong*,BObjectImp*>(imp);
        objtype_long = plong->value();
    }
    else if (imp->isa( BObjectImp::OTString ))
    {
        // this could be an objtypename, or an objtype in string form.  Cope with either.
        String* pstring = explicit_cast<String*,BObjectImp*>(imp);
        const char* ot_str = pstring->data();
        if (!isdigit( ot_str[0] ))
        {
            objtype = get_objtype_byname( pstring->data() );
            if (objtype != 0)
            {
                return true;
            }
            else
            {
                exec.setFunctionResult( new BError( string("Objtype not defined: ") + pstring->data() ) );

                return false;
            }
        }
        else
        {
            // a number passed...process below as if passed as a BLong
            objtype_long = strtol( ot_str, NULL, 0 );
        }
    }
    else
    {
        if (mlog.is_open())
            mlog << "Script Error in '" << exec.scriptname() << "' PC=" << exec.PC << ": " << endl
                 << "\tCall to function " << exec.current_module_function->name << ":" << endl
                 << "\tParameter " << param << ": Expected Long or String, got datatype "
                 << BObjectImp::typestr(imp->type()) << endl;
        return false;
    }

    if (objtype_long >= (config.max_tile_id+0x1001uL) && objtype_long <= EXTOBJ__HIGHEST) 
    {
        objtype = static_cast<unsigned short>(objtype_long);
        if (&find_itemdesc(objtype) != &empty_itemdesc)
        {
            return true;
        }
        else
        {
            exec.setFunctionResult( new BError( "Objtype " + hexint( objtype_long ) + " is not defined." ) );
            return false;
        }

    }
    else if (objtype_long >= 0 && objtype_long < (config.max_tile_id+0x1001uL) )
    {
        objtype = static_cast<unsigned short>(objtype_long);
        return true;
    }
    else
    {
        if (mlog.is_open())
            mlog << "Script Error in '" << exec.scriptname() << "' PC=" << exec.PC << ": " << endl
                    << "\tCall to function " << exec.current_module_function->name << ":" << endl
                    << "\tParameter " << param << ": Value " << objtype_long << " is out of range for an objtype"
                    << endl;
        exec.setFunctionResult( new BError( "Objtype is out of range (acceptable: 0-0xffff)" ) );
        return false;
    }
}

bool getObjtypeParam( Executor& exec, unsigned param, const ItemDesc*& itemdesc_out )
{
    BObjectImp* imp = exec.getParamImp( param );
    if (imp == NULL)
    {
        return false;
    }
    long objtype_long = 0;

    if (imp->isa( BObjectImp::OTLong ))
    {
        BLong* plong = explicit_cast<BLong*,BObjectImp*>(imp);
        objtype_long = plong->value();
    }
    else if (imp->isa( BObjectImp::OTString ))
    {
        // this could be an objtypename, or an objtype in string form.  Cope with either.
        String* pstring = explicit_cast<String*,BObjectImp*>(imp);
        const char* ot_str = pstring->data();
        if (!isdigit( ot_str[0] ))
        {
            unsigned short objtype = get_objtype_byname( pstring->data() );
            if (objtype != 0)
            {
                itemdesc_out = &find_itemdesc( objtype );
                return true;
            }
            else
            {
                exec.setFunctionResult( new BError( string("Objtype not defined: ") + pstring->data() ) );

                return false;
            }
        }
        else
        {
            // a number passed...process below as if passed as a BLong
            objtype_long = strtol( ot_str, NULL, 0 );
        }
    }
    else if (imp->isa( BObjectImp::OTStruct ))
    {
        BStruct* itemdesc_struct = explicit_cast<BStruct*,BObjectImp*>(imp);
        try
        {
            itemdesc_out = CreateItemDescriptor( itemdesc_struct );
            return true;
        }
        catch( std::exception& ex )
        {
            string message = string("Unable to create item descriptor: ") + ex.what();
            exec.setFunctionResult( new BError( message ) );
            return false;
        }
    }
    else
    {
        if (mlog.is_open())
            mlog << "Script Error in '" << exec.scriptname() << "' PC=" << exec.PC << ": " << endl
                 << "\tCall to function " << exec.current_module_function->name << ":" << endl
                 << "\tParameter " << param << ": Expected Long, String, or Struct, got datatype "
                 << BObjectImp::typestr(imp->type()) << endl;
        return false;
    }

    // we get here if the value passed was an integer - either a BLong, or a String containing a number.
    if (objtype_long > (config.max_tile_id+0x1000) && objtype_long <= EXTOBJ__HIGHEST)
    {
        const ItemDesc* itemdesc = &find_itemdesc( static_cast<unsigned short>(objtype_long) );

        if (itemdesc != &empty_itemdesc)
        {
            itemdesc_out = itemdesc;
            return true;
        }
        else
        {
            exec.setFunctionResult( new BError( "Objtype " + hexint( objtype_long ) + " is not defined." ) );
            return false;
        }

    }
    else if (objtype_long >= 0 && objtype_long < (config.max_tile_id+0x1001))
    {
        unsigned short objtype = static_cast<unsigned short>(objtype_long);
        itemdesc_out = &find_itemdesc( objtype );
        if (itemdesc_out == &empty_itemdesc)
        {
            // return a temporary item descriptor initialized with the objtype and graphic.
            itemdesc_out = &temp_itemdesc;
            temp_itemdesc.objtype = objtype;
            temp_itemdesc.graphic = objtype;
            temp_itemdesc.decay_time = ssopt.default_decay_time;
			temp_itemdesc.doubleclick_range = ssopt.default_doubleclick_range;
        }

        return true;
    }
    else
    {

        if (mlog.is_open())
            mlog << "Script Error in '" << exec.scriptname() << "' PC=" << exec.PC << ": " << endl
                    << "\tCall to function " << exec.current_module_function->name << ":" << endl
                    << "\tParameter " << param << ": Value " << objtype_long << " is out of range for an objtype"
                    << endl;
        exec.setFunctionResult( new BError( "Objtype is out of range (acceptable: 0-0xffff)" ) );
        return false;
    }
}

bool getSkillIdParam( Executor& exec, unsigned param, USKILLID& skillid )
{
    long skillval;
    if (exec.getParam( param, skillval, SKILLID__LOWEST, uoclient_general.maxskills ))
    {
        skillid =  static_cast<USKILLID>(skillval);
        return true;
    }
    else
    {
        return false;
    }
}

#include "mobile/attribute.h"
bool getAttributeParam( Executor& exec, unsigned param, const Attribute*& attr )
{
    const String* attrname;
    if (!exec.getStringParam( param, attrname ))
        return false;

    attr = FindAttribute( attrname->value() );
    if (!attr)
    {
        exec.setFunctionResult( new BError( "Attribute not defined: " + attrname->value() ) );
        return false; // new BError( "Attribute not found" );
    }
    
    return true;
}

#include "vital.h"
bool getVitalParam( Executor& exec, unsigned param, const Vital*& vital )
{
    const String* vitalname;
    if (!exec.getStringParam( param, vitalname ))
        return false;

    vital = FindVital( vitalname->value() );
    if (!vital)
    {
        exec.setFunctionResult( new BError( "Vital not defined: " + vitalname->value() ) );
        return false;
    }
    
    return true;
}

BObjectImp* CallPropertyListMethod_id( PropertyList& proplist, const int id, Executor& ex, bool& changed )
{
    switch(id)
    {
    case MTH_GETPROP:
        {
            if (!ex.hasParams(1))
                return new BError( "Not enough parameters" );
            const String* propname_str;
            if (!ex.getStringParam( 0, propname_str ))
                return new BError( "Invalid parameter type" );    
            std::string val;
            if (!proplist.getprop( propname_str->value(), val ))
                return new BError( "Property not found" );

            return BObjectImp::unpack( val.c_str() );
        }

    case MTH_SETPROP:
        {
            if (!ex.hasParams(2))
                return new BError( "Not enough parameters" );
            const String* propname_str;
            if (!ex.getStringParam( 0, propname_str ))
                return new BError( "Invalid parameter type" );    

            BObjectImp* propval = ex.getParamImp( 1 );
            if (propval->isa( BObjectImp::OTError ))
            {
			    Log( "wtf, setprop w/ an error '%s' PC:%d\n",ex.scriptname().c_str(),ex.PC );
            }
            string propname = propname_str->value();

			/*
			std::string propval_packed = propval->pack();

			if(propname.length()>1) {
				if(propname[1] == '@') { // compressed
					unsigned long compr_size = propval_packed.length() * 5;
					Bytef* compr_val = new Bytef[compr_size];
					int result = compress(compr_val, &compr_size, 
							reinterpret_cast<const Bytef*>(propval_packed.c_str()), propval_packed.length());
					if(result==Z_BUF_ERROR)
					{
						delete[] compr_val;
						return new BError("Buffer error while compressing");
					} else if (result==Z_MEM_ERROR)
					{
						delete[] compr_val;
						return new BError("Memory error while compressing");
					}
					propval_packed = "";
					propval_packed.append( reinterpret_cast<const char*>(compr_val), compr_size );
				}
			}
			proplist.setprop( propname, propval_packed );
			*/

            proplist.setprop( propname, propval->pack() );
            if (propname[0] != '#')
                changed = true;
            return new BLong(1);
        }

    case MTH_ERASEPROP:
        {
            if (!ex.hasParams(1))
                return new BError( "Not enough parameters" );
            const String* propname_str;
            if (!ex.getStringParam( 0, propname_str ))
                return new BError( "Invalid parameter type" );    
            string propname = propname_str->value();
            proplist.eraseprop( propname );
            if (propname[0] != '#')
                changed = true;
            return new BLong(1);
        }
    
    case MTH_PROPNAMES:
        {
            vector<string> propnames;
            proplist.getpropnames( propnames );
            ObjArray* arr = new ObjArray;
            for( unsigned i = 0; i < propnames.size(); ++i )
            {
                arr->addElement( new String(propnames[i]) );
            }
            return arr;
        }

    default:
        return NULL;
    }
}

BObjectImp* CallPropertyListMethod( PropertyList& proplist, const char* methodname, Executor& ex, bool& changed )
{
	ObjMethod* objmethod = getKnownObjMethod(methodname);
	if ( objmethod != NULL )
		return CallPropertyListMethod_id(proplist, objmethod->id, ex, changed);
	else
		return NULL;
	/*
    if (stricmp( methodname, "getprop") == 0)
    {
        if (!ex.hasParams(1))
            return new BError( "Not enough parameters" );

        const String* propname_str;
        if (!ex.getStringParam( 0, propname_str ))
            return new BError( "Invalid parameter type" );    

        std::string val;
        if (!proplist.getprop( propname_str->value(), val ))
            return new BError( "Property not found" );

        return BObjectImp::unpack( val.c_str() );
    }
    else if (stricmp( methodname, "setprop" ) == 0)
    {
        if (!ex.hasParams(2))
            return new BError( "Not enough parameters" );

        const String* propname_str;
        if (!ex.getStringParam( 0, propname_str ))
            return new BError( "Invalid parameter type" );    

        BObjectImp* propval = ex.getParamImp( 1 );
        if (propval->isa( BObjectImp::OTError ))
        {
            Log( "wtf, setprop w/ an error\n" );
        }
        const string& propname = propname_str->value();
        proplist.setprop( propname, propval->pack() );
        if (propname[0] != '#')
            changed = true;
        return new BLong(1);
    }
    else if (stricmp( methodname, "eraseprop" ) == 0)
    {
        if (!ex.hasParams(1))
            return new BError( "Not enough parameters" );

        const String* propname_str;
        if (!ex.getStringParam( 0, propname_str ))
            return new BError( "Invalid parameter type" );    
        const string& propname = propname_str->value();
        proplist.eraseprop( propname );
        if (propname[0] != '#')
            changed = true;
        return new BLong(1);
    }
    else if (stricmp( methodname, "propnames" ) == 0)
    {
        vector<string> propnames;
        proplist.getpropnames( propnames );
        ObjArray* arr = new ObjArray;
        for( unsigned i = 0; i < propnames.size(); ++i )
        {
            arr->addElement( new String(propnames[i]) );
        }
        return arr;
    }
    else
    {
        return NULL;
    }
	*/
}
