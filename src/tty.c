#include <tty.h>

tty_t tty_create_device();


int install_tty(){
    fb_console_printf("Installing tty...\n");
    //well register at least 4 tty* devices here,or maybe just one for now
    device_t tty_dev0;
    memset(&tty_dev0, 0, sizeof(device_t));

    tty_dev0.name = strdup("tty0");
    tty_dev0.unique_id = 0;

    tty_t* tty0_data = kcalloc(1, sizeof(tty_t));
    tty_dev0.priv = tty0_data; //tty_t would go there

    tty0_data->id = 0;

    uint8_t* data_page = kpalloc(1); //4kB memory split in to two
    memset(data_page, 0, 4096);

    tty0_data->ib.max_size = 2048;
    tty0_data->ob.max_size = 2048;

    tty0_data->ib.base = data_page;
    tty0_data->ob.base = &data_page[2047];

    dev_register(&tty_dev0);
    return 0;
}