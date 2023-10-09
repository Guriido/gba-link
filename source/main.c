#include <string.h>
#include <tonc.h>
#include "logging.h"
#include "multiboot.h"
#include "communication.h"

#include "multibootsuccess.h"


bool cancelOnSelect()
{
	// cancel function to stop transfer
	// cancel on SELECT press
	key_poll();
	bool cancel = key_hit(KEY_SELECT);
	key_poll();
	return cancel;
}

void exchange_keys()
{
	log_to_screen("init comm GBA cable");
	comm_init(*cancelOnSelect);
	log_to_screen("start loop, press A");
	while (1)
	{
		vid_vsync();
		key_poll();

		MultiplayData data = comm_exchange(key_hit(KEY_A), *cancelOnSelect);
		for (int i = 0; i < COMM_MULTIPLAY_MEMBERS; i++)
		{
			if ((data.d[i] & 0xff) == 1)
			{
				LOG_TO_SCREENF("A press from %d", i);
			}
		}
	}
}

void exchange_keys_normal(bool leader)
{
	log_to_screen("init comm GBC cable");
	comm_init_normal(leader);
	log_to_screen("start loop, press A");
	while (1)
	{
		vid_vsync();
		key_poll();
		u32 data = comm_exchange_normal(key_hit(KEY_A), leader, *cancelOnSelect);
		if ((data & 0xff) == 1)
		{
			log_to_screen("A press from neighbor!");
		}
	}
}

bool get_start_or_select()
{
	// return True for start, false for select
	// and wait otherwise
	while (1)
	{
		vid_vsync();
		key_poll();
		if (key_is_down(KEY_START | KEY_SELECT))
		{
			key_poll();
			return key_is_down(KEY_START);
		}
	}
}


int main()
{
	// oam_init(obj_buffer, 128);
	REG_DISPCNT= DCNT_OBJ | DCNT_OBJ_1D | DCNT_BG0 | DCNT_BG1;

    // Init BG 0 for text on screen entries.
    tte_init_se_default(0, BG_CBB(2)|BG_SBB(31));

	// REG_BG1HOFS= 16;
	REG_BG1HOFS= 0;
	REG_BG1VOFS= 0;

    // Enable TTE's console functionality
    tte_init_con();

	bool using_multiplay;
	if (!is_booted_from_multiboot())
	{
		log_to_screen("try to multiboot");
		const void* rom = (const void*)MEM_EWRAM;

		u32 romSize = 71000;
		log_to_screen("Ready to start multiboot !");
		log_to_screen("Press start for multiplay mode (GBA link cable)");
		log_to_screen("Press select for normal mode (GBC link cable)");
		using_multiplay = get_start_or_select();
		log_to_screen(using_multiplay ? "using multiplay mode" : "using normal mode");
		int res = startMultiBoot(rom, romSize, using_multiplay ? MULTIPLAY_MODE : NORMAL_MODE_LEADER, *cancelOnSelect);
		log_to_screen(res == OPS_SUCCESS ? "MULITBOOT SUCCESS" : "MULTIBOOT FAILURE");
		log_to_screen(is_booted_from_multiboot() ? "CLIENT!" : "LEADER!");
	} else {
		// Load palette
		memcpy(pal_bg_mem, multibootsuccessPal, multibootsuccessPalLen);
		// Load tiles into CBB 0
		memcpy(&tile_mem[0][0], multibootsuccessTiles, multibootsuccessTilesLen);
		// Load map into SBB 30
		memcpy(&se_mem[29][0], multibootsuccessMap, multibootsuccessMapLen);
		REG_BG1CNT= BG_CBB(0) | BG_SBB(29) | BG_8BPP | BG_REG_32x32;

		log_to_screen("Press start for multiplay mode (GBA link cable)");
		log_to_screen("Press select for normal mode (GBC link cable)");
		using_multiplay = get_start_or_select();
	}

	if (using_multiplay)
	{
		exchange_keys();
	} else {
		exchange_keys_normal(!is_booted_from_multiboot());
	}

	while(1);

	return 0;
}
