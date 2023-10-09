
#include <tonc_bios.h>
#include <tonc_core.h>
#include "multiboot.h"
#include "logging.h"




bool is_booted_from_multiboot()
{
    // returns True if the current gba was booted from multiboot (else if booted from "normal" cartridge)
	const void* rom = (const void*)MEM_EWRAM;
    return (
        (*((u8*)rom + MULTIBOOT_HEADER_FIXED_BYTE_ADDRESS) == MULTIBOOT_HEADER_FIXED_BYTE_VALUE) & // cartridge header is in EWRAM
        (*((u8*)rom + MULTIBOOT_HEADER_BOOT_MODE_ADDRESS)  != 0)  // MULTIBOOT BOOT MODE for client is 1 2 or 3
    );
}


enum OperationStatus startMultiBoot(const void* rom, u32 romSize, enum MultiplayerSessionMode sessionMode, CancelFunc isCanceled)
{
    // see https://www.problemkaputt.de/gbatek.htm#biosmultibootsinglegamepak
    // and https://retrocomputing.stackexchange.com/questions/14317/what-is-the-protocol-for-bootstrapping-a-game-boy-advance-over-a-link-cable

    enum OperationStatus op_s;
    MultiBootParam multiboot_parameters;
    multiboot_parameters.palette_data = MULTIBOOT_PALETTE_DATA;
    multiboot_parameters.boot_srcp = (u8*)rom + MULTIBOOT_HEADER_SIZE;
    multiboot_parameters.boot_endp = (u8*)rom + romSize;
    multiboot_parameters.client_data[0] = 0xff;  // fill clients
    multiboot_parameters.client_data[1] = 0xff;
    multiboot_parameters.client_data[2] = 0xff;

    set_multiplayer_communication_session_mode(sessionMode);
    TRY_OPS(detect_clients(&multiboot_parameters, sessionMode, isCanceled))
    TRY_OPS(confirm_clients(&multiboot_parameters, sessionMode, isCanceled))
    TRY_OPS(send_header((const void*)rom, sessionMode, isCanceled))
    TRY_OPS(complete_header_sending(&multiboot_parameters, sessionMode, isCanceled))
    TRY_OPS(send_palette(&multiboot_parameters, sessionMode, isCanceled))
    TRY_OPS(confirm_handshake(&multiboot_parameters, sessionMode, isCanceled))

    int result = MultiBoot(&multiboot_parameters,
                           sessionMode == MULTIPLAY_MODE ? 1: 0);

    set_multiplayer_communication_session_mode(GENERAL_PURPOSE_MODE);
    return result == 1 ? OPS_FAILURE : OPS_SUCCESS;    
}




enum OperationStatus detect_clients(MultiBootParam *multibootParameters, enum MultiplayerSessionMode sessionMode, CancelFunc isCanceled)
{
    int maxClients = MULTIBOOT_MAX_CLIENTS(sessionMode);
    for (u32 t = 0; t < MULTIBOOT_DETECTION_MAX_TRIES; t++) {
        // Leader tries to handshake to all clients
        Responses responses = multiboot_exchange(MULTIBOOT_LEADER_HANDSHAKE, sessionMode, isCanceled);
        RETURN_IF_CANCELED(isCanceled, OPS_ABORTED);

        for (u32 i = 0; i < maxClients; i++) {
            if ((responses.d[i] & MULTIBOOT_CLIENT_HANDSHAKE_RESPONSE_FILTER) == 
                MULTIBOOT_CLIENT_HANDSHAKE_FILTERED_RESPONSE)
            {
                u8 client_id = responses.d[i] & 0xf;  // first hex is the client id (0x720X, X is client id)

                switch (client_id)
                {
                case 0x1:
                case 0x2:
                case 0x3:
                    multibootParameters->client_bit |= client_id;
                    break;
                default:
                    return OPS_NEEDS_RETRY;
                }
            }
        }
    }

    if (multibootParameters->client_bit == 0)
    {
        // no client id were set
        wait_sync(COMM_WAIT_BEFORE_RETRY);
        return OPS_NEEDS_RETRY;
    }

    return OPS_SUCCESS;
}


enum OperationStatus send_header(const void* rom, enum MultiplayerSessionMode sessionMode, CancelFunc isCanceled)
{
    u16* headerPointer = (u16*)rom;
    for (int i = 0; i < MULTIBOOT_HEADER_SIZE; i += 2) // sending header 2 bytes by 2 bytes
    {
        multiboot_exchange(*headerPointer, sessionMode, isCanceled);
        headerPointer++;
        RETURN_IF_CANCELED(isCanceled, OPS_ABORTED);
    }
    return OPS_SUCCESS;
}

enum OperationStatus complete_header_sending(MultiBootParam *multibootParameters, enum MultiplayerSessionMode sessionMode, CancelFunc isCanceled)
{
    int maxClients = MULTIBOOT_MAX_CLIENTS(sessionMode);
    // Leader let client know header sending is complete
    Responses responses = multiboot_exchange(MULTIBOOT_LEADER_CONFIRM, sessionMode, isCanceled);
    RETURN_IF_CANCELED(isCanceled, OPS_ABORTED);
    u8 confirm_client_bit = 0;
    for (u32 i = 0; i < maxClients; i++) {
        if ((responses.d[i] & MULTIBOOT_CLIENT_HANDSHAKE_RESPONSE_FILTER) == 
            MULTIBOOT_CLIENT_HEADER_COMPLETE_FILTERED_RESPONSE)
        {
            u8 client_id = responses.d[i] & 0xf;  // first hex is the client id (0x720X, X is client id)

            switch (client_id)
            {
            case 0x1:
            case 0x2:
            case 0x3:
                confirm_client_bit |= client_id;
                break;
            default:
                log_to_screen("fail in header send completion");
                return OPS_FAILURE;
            }
        }
    }
    if (multibootParameters->client_bit != confirm_client_bit)
    {
        // error in completion of header send
        log_to_screen("error in header send completion");
        return OPS_FAILURE;
    }

    // exchange leader-client info again
    responses = multiboot_exchange(MULTIBOOT_LEADER_CONFIRM | multibootParameters->client_bit, sessionMode, isCanceled);
    RETURN_IF_CANCELED(isCanceled, OPS_ABORTED);
    confirm_client_bit = 0;
    for (u32 i = 0; i < maxClients; i++) {
        if ((responses.d[i] & MULTIBOOT_CLIENT_HANDSHAKE_RESPONSE_FILTER) == 
            MULTIBOOT_CLIENT_HANDSHAKE_FILTERED_RESPONSE)
        {
            u8 client_id = responses.d[i] & 0xf;  // first hex is the client id (0x720X, X is client id)

            switch (client_id)
            {
            case 0x1:
            case 0x2:
            case 0x3:
                confirm_client_bit |= client_id;
                break;
            default:
                log_to_screen("fail in post header handshake");
                return OPS_FAILURE;
            }
        }
    }

    if (multibootParameters->client_bit != confirm_client_bit)
    {
        log_to_screen("error in hanshake post header sending");
        return OPS_FAILURE;
    }
    return OPS_SUCCESS;
}


enum OperationStatus send_palette(MultiBootParam *multibootParameters, enum MultiplayerSessionMode sessionMode, CancelFunc isCanceled)
{
    // try to send palette to clients until each of them responds with client data
    u8 confirmed_clients = 0;
    int maxClients = MULTIBOOT_MAX_CLIENTS(sessionMode);
    Responses responses;
    
    do
    {
        responses = multiboot_exchange(MULTIBOOT_PALETTE_MESSAGE | MULTIBOOT_PALETTE_DATA, sessionMode, isCanceled);
        RETURN_IF_CANCELED(isCanceled, OPS_ABORTED);
        for (u8 i = 0; i < maxClients; i++) {
            if ((responses.d[i] & MULTIBOOT_CLIENT_HANDSHAKE_RESPONSE_FILTER) ==
                MULTIBOOT_PALETTE_FILTERED_RESPONSE)
            {
                confirmed_clients |= 1<<(i+1);
                multibootParameters->client_data[i] = responses.d[i] & 0xff;
            }
        }
    } while (confirmed_clients != multibootParameters->client_bit);

    u8 handshake_data = MULTIBOOT_HANDSHAKE_DATA;
    for (u8 i = 0; i < MULTIBOOT_CLIENTS; i++) {
        // compute even for not connected clients (yes, for normal mode too)
        handshake_data += multibootParameters->client_data[i];
    }
    multibootParameters->handshake_data = handshake_data;

    return OPS_SUCCESS;
}

enum OperationStatus confirm_handshake(MultiBootParam *multibootParameters, enum MultiplayerSessionMode sessionMode, CancelFunc isCanceled)
{
    int maxClients = MULTIBOOT_MAX_CLIENTS(sessionMode);
    // try to send palette to clients until each of them responds with client data
    u8 confirmed_clients = 0;
    Responses responses = multiboot_exchange(MULTIBOOT_FINAL_HANDSHAKE_MESSAGE | multibootParameters->handshake_data, sessionMode, isCanceled);
    RETURN_IF_CANCELED(isCanceled, OPS_ABORTED);
    for (u8 i = 0; i < maxClients; i++) {
        if ((responses.d[i] & MULTIBOOT_CLIENT_HANDSHAKE_RESPONSE_FILTER) ==
            MULTIBOOT_PALETTE_FILTERED_RESPONSE)
        {
            confirmed_clients |= 1<<(i+1);
        }
    }
    if (confirmed_clients != multibootParameters->client_bit)
    {
        return OPS_FAILURE;
    }

    return OPS_SUCCESS;
}


enum OperationStatus confirm_clients(MultiBootParam *multibootParameters, enum MultiplayerSessionMode sessionMode, CancelFunc isCanceled)
{
    int maxClients = MULTIBOOT_MAX_CLIENTS(sessionMode);
    // Leader sends clients infos to all clients
    Responses responses = multiboot_exchange(MULTIBOOT_CLIENT_RECOGNITION | multibootParameters->client_bit, sessionMode, isCanceled);
    RETURN_IF_CANCELED(isCanceled, OPS_ABORTED);

    u8 confirm_client_bit = 0;
    for (u32 i = 0; i < maxClients; i++) {
        if ((responses.d[i] & MULTIBOOT_CLIENT_HANDSHAKE_RESPONSE_FILTER) == 
            MULTIBOOT_CLIENT_HANDSHAKE_FILTERED_RESPONSE)
        {
            u8 client_id = responses.d[i] & 0xf;  // first hex is the client id (0x720X, X is client id)

            switch (client_id)
            {
            case 0x1:
            case 0x2:
            case 0x3:
                confirm_client_bit |= client_id;
                break;
            default:
                return OPS_NEEDS_RETRY;
            }
        }
    }

    if (multibootParameters->client_bit != confirm_client_bit)
    {
        log_to_screen("error in hanshake confirm");
        return OPS_FAILURE;
    }

    return OPS_SUCCESS;
}

Responses multiboot_exchange(u16 data, enum MultiplayerSessionMode sessionMode, CancelFunc isCanceled)
{
    int maxClients = MULTIBOOT_MAX_CLIENTS(sessionMode);
    Responses responses;
    responses.d[0] = 0xffff;  // value for disconnected client
    responses.d[1] = 0xffff;  // value for disconnected client
    responses.d[2] = 0xffff;  // value for disconnected client

    wait_sync(COMM_WAIT_BEFORE_TRANSFER);

    // wait for start bit to be cleared
    while (isSIOCNTBitHigh(COMM_SIOCNT_START_BIT))
    { RETURN_IF_CANCELED(isCanceled, responses); }

    switch (sessionMode)
    {
    case MULTIPLAY_MODE:
        REG_SIOMLT_SEND = data;
        break;
    default:
        // case for normal mode
        REG_SIOMULTI1 = 0x0000;  // reset upper 16 bits
        REG_SIODATA32 = data;    // set lower 16 bits
        break;
    }
    setSIOCNTBitHigh(COMM_SIOCNT_START_BIT);

    // wait for start bit to be cleared (-> data send)
    while (isSIOCNTBitHigh(COMM_SIOCNT_START_BIT))
    { RETURN_IF_CANCELED(isCanceled, responses); }

    for (u32 i = 0; i < maxClients; i++)
        responses.d[i] = REG_SIOMULTI[1 + i];

    return responses;
}

