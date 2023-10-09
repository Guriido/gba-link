
#ifndef __MULTIBOOT__
#define __MULTIBOOT__

#include <tonc_bios.h>
#include <tonc_core.h>
#include "communication_common.h"

#define MULTIBOOT_PALETTE_DATA 0b11010111 // 0b1CCCDSS1, where C=color, D=direction, and S=speed
#define MULTIBOOT_HEADER_SIZE 0xC0
#define MULTIBOOT_CLIENTS 3 // up to 3 other gbas
#define MULTIBOOT_DETECTION_MAX_TRIES 16
#define MULTIBOOT_LEADER_HANDSHAKE 0x6200
#define MULTIBOOT_CLIENT_HANDSHAKE_RESPONSE_FILTER 0xff00
#define MULTIBOOT_CLIENT_HANDSHAKE_FILTERED_RESPONSE 0x7200  // the first hex is expected to be 1, 2 or 3 (client id)
#define MULTIBOOT_LEADER_CONFIRM 0x6200
#define MULTIBOOT_CLIENT_RECOGNITION 0x6100
#define MULTIBOOT_CLIENT_HEADER_COMPLETE_FILTERED_RESPONSE 0x0000  // the first hex is expected to be 1, 2 or 3 (client id)
#define MULTIBOOT_PALETTE_MESSAGE 0x6300
#define MULTIBOOT_PALETTE_FILTERED_RESPONSE 0x7300
#define MULTIBOOT_HANDSHAKE_DATA 0x11
#define MULTIBOOT_FINAL_HANDSHAKE_MESSAGE 0x6400
#define MULTIBOOT_HEADER_BOOT_MODE_ADDRESS 0x0c4
#define MULTIBOOT_HEADER_FIXED_BYTE_ADDRESS 0x0b2
#define MULTIBOOT_HEADER_FIXED_BYTE_VALUE 0x96


#define MULTIBOOT_MAX_CLIENTS(session) (session) == MULTIPLAY_MODE ? 3 : 1

#define TRY_OPS(call)                                                           \
    do                                                                          \
    {                                                                           \
        op_s = call;                                                            \
    } while (op_s == OPS_NEEDS_RETRY);                                          \
    switch (op_s)                                                               \
    {                                                                           \
        case OPS_FAILURE:                                                       \
        case OPS_ABORTED:                                                       \
            set_multiplayer_communication_session_mode(GENERAL_PURPOSE_MODE);   \
            return op_s;                                                        \
            break;                                                              \
        default:                                                                \
            break;                                                              \
    }

typedef struct {
    u16 d[MULTIBOOT_CLIENTS];
} Responses;

bool is_booted_from_multiboot();
enum OperationStatus startMultiBoot(const void* rom, u32 romSize, enum MultiplayerSessionMode sessionMode, CancelFunc isCanceled);
enum OperationStatus detect_clients(MultiBootParam *multibootParameters, enum MultiplayerSessionMode sessionMode, CancelFunc isCanceled);
enum OperationStatus confirm_clients(MultiBootParam *multibootParameters, enum MultiplayerSessionMode sessionMode, CancelFunc isCanceled);
enum OperationStatus send_header(const void* rom, enum MultiplayerSessionMode sessionMode, CancelFunc isCanceled);
enum OperationStatus complete_header_sending(MultiBootParam *multibootParameters, enum MultiplayerSessionMode sessionMode, CancelFunc isCanceled);
enum OperationStatus send_palette(MultiBootParam *multibootParameters, enum MultiplayerSessionMode sessionMode, CancelFunc isCanceled);
enum OperationStatus confirm_handshake(MultiBootParam *multibootParameters, enum MultiplayerSessionMode sessionMode, CancelFunc isCanceled);

Responses multiboot_exchange(u16 data, enum MultiplayerSessionMode sessionMode, CancelFunc isCanceled);
void wait_sync(u32 verticalLines);

bool isSIOCNTBitHigh(u8 bit);
void setSIOCNTBitHigh(u8 bit);
void setSIOCNTBitLow(u8 bit);

#endif
