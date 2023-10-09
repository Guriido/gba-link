
#include <tonc_bios.h>
#include <tonc_core.h>
#include "logging.h"
#include "communication.h"


enum OperationStatus comm_init(CancelFunc isCanceled)
{
    // ref: https://www.problemkaputt.de/gbatek.htm#siomultiplayermode
    set_multiplayer_communication_session_mode(MULTIPLAY_MODE);

    // wait for all GBAs to be in Multiplay mode
    while (!isSIOCNTBitHigh(COMM_SIOCNT_SDTERMINAL_BIT))
    {
        wait_sync(COMM_WAIT_BEFORE_RETRY);
        RETURN_IF_CANCELED(isCanceled, OPS_ABORTED);
        // should return in general mode here
    }
    
    return OPS_SUCCESS;
}


MultiplayData comm_exchange(u16 data, CancelFunc isCanceled)
{
    MultiplayData received_data;
    received_data.d[0] = 0xffff;  // value for disconnected gba
    received_data.d[1] = 0xffff;  // value for disconnected gba
    received_data.d[2] = 0xffff;  // value for disconnected gba
    received_data.d[3] = 0xffff;  // value for disconnected gba
    

    wait_sync(COMM_WAIT_BEFORE_TRANSFER);
   
    // wait for start bit to be cleared
    while (isSIOCNTBitHigh(COMM_SIOCNT_START_BIT))
    { RETURN_IF_CANCELED(isCanceled, received_data); }

    // prepare data to send
    REG_SIOMLT_SEND = data;
    if (comm_is_parent())
    {
        // delay start transfer
        wait_sync(5);
        setSIOCNTBitHigh(COMM_SIOCNT_START_BIT);
    }

    // wait for start bit to be cleared (-> data send)
    while (isSIOCNTBitHigh(COMM_SIOCNT_START_BIT))
    { RETURN_IF_CANCELED(isCanceled, received_data); }

    // copy received data from register
    for (int i = 0; i < COMM_MULTIPLAY_MEMBERS; i++)
    {
        received_data.d[i] = REG_SIOMULTI[i];
    }

    return received_data;
}


int comm_get_multiplayer_id()
{
    return (REG_SIOCNT>>4) & 0b11;
}


enum OperationStatus comm_init_normal(bool leader)
{
    // ref https://www.problemkaputt.de/gbatek.htm#sionormalmode
    set_multiplayer_communication_session_mode(leader ? NORMAL_MODE_LEADER : NORMAL_MODE_FOLLOWER);
    setSIOCNTBitHigh(COMM_SIOCNT_NORMAL_SO_BIT);
    return OPS_SUCCESS;
}


u32 comm_exchange_normal(u32 data, bool leader, CancelFunc isCanceled)
{
    u32 received_data = 0xffffffff;  // error value

    // wait for release of start bit
    while (isSIOCNTBitHigh(COMM_SIOCNT_START_BIT))
    { RETURN_IF_CANCELED(isCanceled, received_data); }

    // prepare data to be send
    REG_SIODATA32 = data;
    setSIOCNTBitLow(COMM_SIOCNT_NORMAL_SO_BIT);

    if (leader)
    {
        wait_sync(COMM_WAIT_BEFORE_TRANSFER);

        // wait for release of start bit
        while (isSIOCNTBitHigh(COMM_SIOCNT_NORMAL_SI_BIT))
        { RETURN_IF_CANCELED(isCanceled, received_data); }
        
        // set start
        setSIOCNTBitHigh(COMM_SIOCNT_START_BIT);

        // wait for transfer completion
        while (isSIOCNTBitHigh(COMM_SIOCNT_START_BIT))
        { RETURN_IF_CANCELED(isCanceled, received_data); }
    } else {
        setSIOCNTBitHigh(COMM_SIOCNT_START_BIT);
        // // set start=0 and SO=0
        // REG_SIOCNT &= ~((1<<COMM_SIOCNT_NORMAL_SO_BIT) | (1<<COMM_SIOCNT_START_BIT));
        // // set start=1 and SO=1
        // REG_SIOCNT |= (1<<COMM_SIOCNT_NORMAL_SO_BIT) | (1<<COMM_SIOCNT_START_BIT);

        // wait for transfer completion
        while (isSIOCNTBitHigh(COMM_SIOCNT_START_BIT))
        { RETURN_IF_CANCELED(isCanceled, received_data); }

        // set SO=1
        setSIOCNTBitHigh(COMM_SIOCNT_NORMAL_SO_BIT);
    }

    received_data = REG_SIODATA32;

    return received_data;
}


bool comm_is_parent() { return !isSIOCNTBitHigh(COMM_SIOCNT_SITERMINAL_BIT); }
