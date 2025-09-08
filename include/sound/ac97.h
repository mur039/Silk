#ifndef __AC97_H__
#define __AC97_H__

#include <pci.h>

#define AC97_PCI_VENDOR_ID 0x8086
#define AC97_PCI_DEVICE_ID 0x2415

int ac97_pci_initialize(pci_device_t* pdev);


//NATIVE AUDIO MIXER
#define AC97_RESET_CAP_REG           0x00 //Reset Register/Capabilities 	word
#define AC97_MASTER_OUT_VOL_REG      0x02 //Set Master Output Volume 	word
#define AC97_AUX_OUT_VOL_REG         0x04 //Set AUX Output Volume 	word
#define AC97_MIC_VOL_REG             0x0E //Set Microphone Volume 	word
#define AC97_PCM_OUT_VOL_REG         0x18 //Set PCM Output Volume 	word
#define AC97_SEL_IN_DEV_REG          0x1A //Select Input Device 	word
#define AC97_IN_GAIN_REG             0x1C //Set Input Gain 	word
#define AC97_GAIN_MIC_REG            0x1E //Set Gain of Microphone 	word
#define AC97_EXT_CAP_REG             0x28 //Extended capabilities 	word
#define AC97_CTRL_EXT_CAP_REG        0x2A //Control of extended capabilities 	word
#define AC97_SMPLRT_PCM_FR_DAC_REG   0x2C //Sample rate of PCM Front DAC 	word
#define AC97_SMPLRT_PCM_SURR_DAC_REG 0x2E //Sample rate of PCM Surr DAC 	word
#define AC97_SMPLRT_PCM_LFE_DAC_REG  0x30 //Sample rate of PCM LFE DAC 	word
#define AC97_SMPLRT_PCM_L_R_ADC_REG  0x32 //Sample rate of PCM L/R ADC


//NATIVE AUDIO BUS MIXER
#define AC97_NABM_PCM_IN    0x00 	//NABM register box for PCM IN 	below
#define AC97_NABM_PCM_OUT   0x10 	//NABM register box for PCM OUT 	below
#define AC97_NABM_MIC_REG   0x20 	//NABM register box for Microphone 	below
#define AC97_NABM_GC_REG    0x2C 	//Global Control Register 	dword
#define AC97_NABM_GS_REG    0x30 	//Global Status Register 	dword 

//NABM CONTENTS
#define AC97_NABM_PHYS_BUFF_DESC_REG        0x00 	//Physical Address of Buffer Descriptor List 	dword
#define AC97_NABM_N_PROCESSED_DESC_REG      0x04 	//Number of Actual Processed Buffer Descriptor Entry 	byte
#define AC97_NABM_N_BUFF_DESC_ENTRY_REG     0x05 	//Number of all Descriptor Entries 	byte
#define AC97_NABM_STAT_TRANFERRING_REG      0x06 	//Status of transferring Data 	word
#define AC97_NABM_TRANSFERRED_SAMPLES_REG   0x08 	//Number of transferred Samples in Actual Processed Entry 	word
#define AC97_NABM_N_NEXT_PROCESSED_REG      0x0A 	//Number of next processed Buffer Entry 	byte
#define AC97_NABM_TRANSFER_CTRL_REG         0x0B 	//Transfer Control 	byte 
#endif
