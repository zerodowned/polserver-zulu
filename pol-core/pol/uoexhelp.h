/*
History
=======


Notes
=======

*/

#ifndef UOEXHELP_H
#define UOEXHELP_H

class Attribute;
class BObjectImp;
class Character;
class Executor;
class Item;
class ItemDesc;
class PropertyList;
class UBoat;
class UMulti;
class UObject;
class Vital;

#include "skillid.h"

bool getCharacterParam( Executor& exec, unsigned param, Character*& chr );
bool getItemParam( Executor& exec, unsigned param, Item*& item );
bool getUObjectParam( Executor& exec, unsigned param, UObject*& objptr );
bool getObjtypeParam( Executor& exec, unsigned param, unsigned short& objtype );
bool getObjtypeParam( Executor& exec, unsigned param, const ItemDesc*& itemdesc );
bool getSkillIdParam( Executor& exec, unsigned param, USKILLID& skillid );
bool getUBoatParam( Executor& exec, unsigned param, UBoat*& boat );
bool getMultiParam( Executor& exec, unsigned param, UMulti*& multi );
bool getAttributeParam( Executor& exec, unsigned param, const Attribute*& attr );
bool getVitalParam( Executor& exec, unsigned param, const Vital*& vital );

BObjectImp* CallPropertyListMethod( PropertyList& proplist, 
                                    const char* methodname, 
                                    Executor& ex,
                                    bool& changed );
BObjectImp* CallPropertyListMethod_id( PropertyList& proplist, 
                                      const int id, 
                                      Executor& ex, 
                                      bool& changed );
#endif
