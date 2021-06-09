#define MOXA_HOTSWAP_MINOR 115
#define DEVICE_NAME "hotswap"

/*
 * Supersit bits
 */
#define	LED_BTN_ADDR	0x307
#define	BIT_DISK1_LED	(1<<4)
#define	BIT_DISK1_BTN	(1<<5)
#define	BIT_DISK2_LED	(1<<6)
#define	BIT_DISK2_BTN	(1<<7)

/*
 * Hardware specific part
 */
/* DMI table Type 12 Option 1 */
#define BUFF_SZ 32
#define NAME_LEN 6
#define NAME_START 5
#define NAME_END 10

/* SATA controller: Intel 100 Corporation Sunrise Point-LP SATA Controller */
/* on Kaby Lak platform */
#define KBL_DEVICE_ID	0x9d03
#define KBL_CPU_NAME	"KBL-U"

/* SATA controller: Intel 300 Corporation Sunrise Point-LP SATA Controller */
/* https://www.intel.com/content/dam/www/public/us/en/documents/technical-specifications/300-series-chipset-on-package-pch-datasheet-vol-1.pdf  */
/* on Whiskey Lake platform */
#define WHL_DEVICE_ID	0x9dd3
#define WHL_CPU_NAME	"WHL-U"

/* Ref Intel 300 Series Chipset Family Platform Controller Hub */
/* https://www.intel.com/content/dam/www/public/us/en/documents/technical-specifications/serial-ata-ahci-spec-rev1_3.pdf */
/* Offset 28h: PxSSTS – Port x Serial ATA Status */
#define SATA_P1SSTS	0x1A8	/* Port 1 Serial ATA Status */
#define SATA_P2SSTS	0x228	/* Port 2 Serial ATA Status */

#define moxainb inb_p
#define moxaoutb outb_p

typedef struct _disk_info {
	unsigned int status;
	int busy;
	int idle_cnt;
} disk_info;
