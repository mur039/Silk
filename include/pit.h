#ifndef __PIT_H__
#define __PIT_H__

#include <sys.h>
//I/O port     Usage
#define PIT_CHANNEL_1_DATA_PORT 0x40         //Channel 0 data port (read/write)
#define PIT_CHANNEL_2_DATA_PORT 0x41         //Channel 1 data port (read/write)
#define PIT_CHANNEL_3_DATA_PORT 0x42         //Channel 2 data port (read/write)
/*Each 8 bit data port is the same, and is used to set the counter's 16 bit reload value or read the channel's 16 bit current count (more on this later). 
The PIT channel's current count and reload value should not be confused. In general, when the current count reaches zero the PIT channel's output is changed 
and the current count is reloaded with the reload value, however this isn't always the case. 
How the current count and reload value are used and what they contain dependson which mode the PIT channel is configured to use.
*/

#define PIT_MODE_COMMAND_REGISTER 0x43        // Mode/Command register (write only, a read is ignored)

void disable_pit();


#endif