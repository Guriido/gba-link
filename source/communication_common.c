
#include <tonc_bios.h>
#include <tonc_core.h>
#include "communication_common.h"




void set_multiplayer_communication_session_mode(enum MultiplayerSessionMode mode)
{
    // details: https://www.problemkaputt.de/gbatek.htm#gbacommunicationports
    switch (mode)
    {
    case NORMAL_MODE_LEADER:
        SET_REG_BIT_LOW(REG_RCNT, COMM_RCNT_MODE_HBIT);
        REG_SIOCNT = 1;
        SET_REG_BIT_HIGH(REG_SIOCNT, COMM_SIOCNT_TRANSFER_LENGTH_BIT);
        break;
    case NORMAL_MODE_FOLLOWER:
        SET_REG_BIT_LOW(REG_RCNT, COMM_RCNT_MODE_HBIT);
        REG_SIOCNT = 0;
        SET_REG_BIT_HIGH(REG_SIOCNT, COMM_SIOCNT_TRANSFER_LENGTH_BIT);
        break;
    case MULTIPLAY_MODE:
        SET_REG_BIT_LOW(REG_RCNT, COMM_RCNT_MODE_HBIT);
        REG_SIOCNT = COMM_MULTIPLAY_BAUD_RATE;  // also set bit 12 to 0
        SET_REG_BIT_HIGH(REG_SIOCNT, COMM_SIOCNT_BIT_MULTIPLAY);
        break;
    case GENERAL_PURPOSE_MODE:
        SET_REG_BIT_HIGH(REG_RCNT, COMM_RCNT_MODE_LBIT);
        SET_REG_BIT_LOW(REG_RCNT, COMM_RCNT_MODE_HBIT);
        break;
    default:
        break;
    }
}

void wait_sync(u32 verticalLines)
{  
    // count the number of vertical lines update
    u32 count = 0;
    u32 vCount = REG_VCOUNT;

    while (count < verticalLines) {
        if (REG_VCOUNT != vCount) {
            count++;
            vCount = REG_VCOUNT;
        }
    };
}

bool isSIOCNTBitHigh(u8 bit) { return (REG_SIOCNT >> bit) & 1; }
void setSIOCNTBitHigh(u8 bit) { SET_REG_BIT_HIGH(REG_SIOCNT, bit); }
void setSIOCNTBitLow(u8 bit) { SET_REG_BIT_LOW(REG_SIOCNT, bit); }
