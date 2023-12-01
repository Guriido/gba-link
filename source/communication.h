
#ifndef __COMMUNICATION__
#define __COMMUNICATION__

#include "communication_common.h"

#define COMM_SIOCNT_SITERMINAL_BIT 2
#define COMM_SIOCNT_SDTERMINAL_BIT 3
#define COMM_SIOCNT_NORMAL_SI_BIT 2  // becomes low when follower is ready
#define COMM_SIOCNT_NORMAL_SO_BIT 3  // indicates to leader when follower is ready
#define COMM_MULTIPLAY_MEMBERS 4  // includes self

typedef struct {
    u16 d[COMM_MULTIPLAY_MEMBERS];
} MultiplayData;

enum OperationStatus comm_init(CancelFunc isCanceled);
MultiplayData comm_exchange(u16 data, CancelFunc isCanceled);
int comm_get_multiplayer_id();
enum OperationStatus comm_init_normal(bool leader);
u32 comm_exchange_normal(u32 data, bool leader, CancelFunc isCanceled);

#endif
