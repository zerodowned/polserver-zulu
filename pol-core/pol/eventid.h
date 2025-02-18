/*
History
=======


Notes
=======

*/

#ifndef EVENTID_H
#define EVENTID_H

enum EVENTID 
{
    EVID_SPOKE          = 0x0001,
    
    EVID_ENGAGED        = 0x0002,
    EVID_DISENGAGED     = 0x0004,
    
    EVID_DAMAGED        = 0x0008,
    
    EVID_ENTEREDAREA    = 0x0010,
    EVID_LEFTAREA       = 0x0020,

    EVID_OPPONENT_MOVED = 0x0040,
    EVID_HOSTILE_MOVED  = 0x0080,

    EVID_MERCHANT_BOUGHT= 0x0100,
    EVID_MERCHANT_SOLD  = 0x0200,

    EVID_ITEM_GIVEN     = 0x0400,
    EVID_DOUBLECLICKED  = 0x0800,
    EVID_GHOST_SPEECH   = 0x1000,
	EVID_GONE_CRIMINAL  = 0x2000
};


#endif
