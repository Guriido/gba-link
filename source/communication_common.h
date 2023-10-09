
#ifndef __COMMUNICATION_COMMON__
#define __COMMUNICATION_COMMON__

#include <tonc_bios.h>
#include <tonc_core.h>




#define COMM_SIOCNT_START_BIT 7  // bit of SIOCNT reg
#define COMM_MULTIPLAY_BAUD_RATE 3 // 3 is for the highest rate: 115200 bps
#define COMM_SIOCNT_TRANSFER_LENGTH_BIT 12
#define COMM_SIOCNT_BIT_MULTIPLAY 13
#define COMM_RCNT_MODE_LBIT 14
#define COMM_RCNT_MODE_HBIT 15
#define COMM_WAIT_BEFORE_RETRY ((160 + 68) * 60)  // unit is vline updates 
#define COMM_WAIT_BEFORE_TRANSFER 50

#define SET_REG_BIT_HIGH(REG, BIT) REG |= 1<<(BIT)
#define SET_REG_BIT_LOW(REG, BIT) REG &= ~(1<<(BIT))

#define RETURN_IF_CANCELED(cancel_fun, result)  \
    if(cancel_fun())                            \
    {                                           \
        return result;                          \
    }

enum MultiplayerSessionMode
{
    GENERAL_PURPOSE_MODE=0, NORMAL_MODE_LEADER, NORMAL_MODE_FOLLOWER, MULTIPLAY_MODE
};
enum OperationStatus 
{
    OPS_FAILURE=0, OPS_ABORTED, OPS_NEEDS_RETRY, OPS_SUCCESS
};

typedef bool (*CancelFunc)();  // function that returns 1 if canceled else 0

void set_multiplayer_communication_session_mode(enum MultiplayerSessionMode mode);
void wait_sync(u32 verticalLines);
bool isSIOCNTBitHigh(u8 bit);
void setSIOCNTBitHigh(u8 bit);
void setSIOCNTBitLow(u8 bit);

#endif
