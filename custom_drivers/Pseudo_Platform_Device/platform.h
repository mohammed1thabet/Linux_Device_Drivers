#ifndef  __PLF_CFG_
#define  __PLF_CFG_

/*number of platform devices*/
#define PLF_DEV_COUNT           2
#define MAX_NUMBER_OF_DEVICES   5

/*device permission*/
#define RONLY_PERMISSION        0b01
#define WONLY_PERMISSION        0b10
#define RW_PERMISSION           0b11

#define DEV0_MEM_SIZE           1024
#define DEV1_MEM_SIZE           512

/*device platform data*/
struct pseudo_platform_data{
    size_t size;
    const char* serial_number;
    /*first bit for read permission and second bit for write permission*/
    /* example: 0b11 means RW permission                               */
    int permission;
};


#endif