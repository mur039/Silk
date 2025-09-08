#ifndef __VIRTIO_H__
#define __VIRTIO_H__

#include <sys.h>
#include <stdint.h>
#include <pmm.h>
#include <pci.h>
#include <fb.h>
#include <g_list.h>
typedef struct{
    u32 device_features;
    u32  guest_features;
    u32   queue_address;
    u16      queue_size;
    u16    queue_select;
    u16    queue_notify;
    u8   device_status;
    u8      isr_status;
} virtio_registers_t;


enum virtio_device_type{
    VIRTIO_GPU_DEVICE = 16
};



/* Common configuration */
#define VIRTIO_PCI_CAP_COMMON_CFG 1
/* Notifications */
#define VIRTIO_PCI_CAP_NOTIFY_CFG 2
/* ISR Status */
#define VIRTIO_PCI_CAP_ISR_CFG 3
/* Device specific configuration */
#define VIRTIO_PCI_CAP_DEVICE_CFG 4
/* PCI configuration access */
#define VIRTIO_PCI_CAP_PCI_CFG 5

typedef struct {
    u8 cap_vndr; /* Generic PCI field: PCI_CAP_ID_VNDR */
    u8 cap_next; /* Generic PCI field: next ptr. */
    u8 cap_len; /* Generic PCI field: capability length */
    u8 cfg_type; /* Identifies the structure. */
    u8 bar; /* Where to find it. */
    u8 padding[3]; /* Pad to full dword. */
    u32 offset; /* Offset within bar. */
    u32 length; /* Length of the structure, in bytes. */
} __attribute__((packed)) virtio_pci_cap_t;


typedef struct {
/* About the whole device. */
    u32 device_feature_select; /* read-write */
    u32 device_feature; /* read-only for driver */
    u32 driver_feature_select; /* read-write */
    u32 driver_feature; /* read-write */
    u16 msix_config; /* read-write */
    u16 num_queues; /* read-only for driver */
    u8 device_status; /* read-write */
    u8 config_generation; /* read-only for driver */

    /* About a specific virtqueue. */
    u16 queue_select; /* read-write */
    u16 queue_size; /* read-write */
    u16 queue_msix_vector; /* read-write */
    u16 queue_enable; /* read-write */
    u16 queue_notify_off; /* read-only for driver */
    u64 queue_desc; /* read-write */
    u64 queue_driver; /* read-write */
    u64 queue_device; /* read-write */
} __attribute__((packed)) virtio_pci_common_cfg_t;




typedef struct{
    virtio_pci_cap_t cap;
    u32 notify_off_multiplier; /* Multiplier for queue_notify_off. */
} virtio_pci_notify_cap_t;



enum virtio_device_status_bit{
    DEVICE_ACKNOWLEDGED = 0x1,
    DRIVER_LOADED = 0x2,
    DRIVER_OK = 0x4,
    DRIVER_FEATURES_OK = 0x8,
    DEVICE_NEEDS_RESET= 0x40,
    DEVICE_FAILED = 0x80
};



/* This marks a buffer as continuing via the next field. */
#define VIRTQ_DESC_F_NEXT 1
/* This marks a buffer as device write-only (otherwise device read-only). */
#define VIRTQ_DESC_F_WRITE 2
/* This means the buffer contains a list of buffer descriptors. */
#define VIRTQ_DESC_F_INDIRECT 4

typedef struct {
    u64 addr; // Address (guest-physical). 
    u32 len; //Length.
    u16 flags; //The flags as indicated above.
    u16 next; // Next field if flags & NEXT 
}  virtq_desc_t;


#define VIRTQ_AVAIL_F_NO_INTERRUPT 1
typedef struct  {
    u16 flags;
    u16 idx;
    u16 ring;
    // u16 used_event; /* Only if VIRTIO_F_EVENT_IDX */
} virtq_avail_t;


typedef struct {
    u32 id; //Index of start of used descriptor chain.
    u32 len; // Total length of the descriptor chain which was used (written to) 
} virtq_used_elem_t ;

typedef struct {
#define VIRTQ_USED_F_NO_NOTIFY 1
    u16 flags;
    u16 idx;
    virtq_used_elem_t ring;
    // u16 avail_event; /* Only if VIRTIO_F_EVENT_IDX */
} __attribute__((packed)) virtq_used_t;




typedef struct{

    virtq_desc_t  *desc_table;
    virtq_avail_t *driver_area;
    virtq_used_t  *device_area;
    void * virt_queue_virtual_addr;
    u32 queue_size;
} virt_queue_t;






typedef struct{
    
    pci_device_t pci_dev;
    virt_queue_t  vq;
    virtio_pci_common_cfg_t* common_cfg;
    virtio_pci_notify_cap_t queue_notify;
    u16 * notify_base_offset_addr;
    u32  notify_multiplier;
} virtio_device_t;



i32 virtio_register_device( pci_device_t * dev);
virt_queue_t virtio_create_virtual_queue(unsigned int queue_entry_size);






/*VIRTIO_GPU SPECIFICS*/
#define VIRTIO_GPU_F_VIRGL 0  
#define VIRTIO_GPU_F_EDID  1

#define VIRTIO_GPU_CONTROLQ 0
#define VIRTIO_GPU_CURSORQ  1

enum virtio_gpu_ctrl_type {
    /* 2d commands */
    VIRTIO_GPU_CMD_GET_DISPLAY_INFO = 0x0100,
    VIRTIO_GPU_CMD_RESOURCE_CREATE_2D,
    VIRTIO_GPU_CMD_RESOURCE_UNREF,
    VIRTIO_GPU_CMD_SET_SCANOUT,
    VIRTIO_GPU_CMD_RESOURCE_FLUSH,
    VIRTIO_GPU_CMD_TRANSFER_TO_HOST_2D,
    VIRTIO_GPU_CMD_RESOURCE_ATTACH_BACKING,
    VIRTIO_GPU_CMD_RESOURCE_DETACH_BACKING,
    VIRTIO_GPU_CMD_GET_CAPSET_INFO,
    VIRTIO_GPU_CMD_GET_CAPSET,

    VIRTIO_GPU_CMD_GET_EDID,
    /* cursor commands */
    VIRTIO_GPU_CMD_UPDATE_CURSOR = 0x0300,
    VIRTIO_GPU_CMD_MOVE_CURSOR,
    /* success responses */
    VIRTIO_GPU_RESP_OK_NODATA = 0x1100,
    VIRTIO_GPU_RESP_OK_DISPLAY_INFO,
    VIRTIO_GPU_RESP_OK_CAPSET_INFO,
    VIRTIO_GPU_RESP_OK_CAPSET,
    VIRTIO_GPU_RESP_OK_EDID,
    /* error responses */
    VIRTIO_GPU_RESP_ERR_UNSPEC = 0x1200,
    VIRTIO_GPU_RESP_ERR_OUT_OF_MEMORY,
    VIRTIO_GPU_RESP_ERR_INVALID_SCANOUT_ID,
    VIRTIO_GPU_RESP_ERR_INVALID_RESOURCE_ID,
    VIRTIO_GPU_RESP_ERR_INVALID_CONTEXT_ID,
    VIRTIO_GPU_RESP_ERR_INVALID_PARAMETER,
};

#define VIRTIO_GPU_FLAG_FENCE (1 << 0)
typedef struct {
    u32 type;
    u32 flags;
    u64 fence_id;
    u32 ctx_id;
    u32 padding;
} virtio_gpu_ctrl_hdr_t;


#endif