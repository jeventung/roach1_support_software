/*
 * (C) Copyright 2003-2007
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * Based on PRO Motion board config file by Andy Joseph, andy@promessdev.com
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef __CONFIG_H
#define __CONFIG_H


/*
 * High Level Configuration Options
 */


/* CPU and board */
#define CONFIG_MPC5xxx		1	/* This is an MPC5xxx CPU */
#define CONFIG_MPC5200		1	/* More exactly a MPC5200 */
#define CONFIG_MOTIONPRO	1	/* ... on Promess Motion-PRO board */


/*
 * Supported commands
 */
#define CONFIG_COMMANDS		(CONFIG_CMD_DFL	| \
				CFG_CMD_ASKENV	| \
				CFG_CMD_DHCP	| \
				CFG_CMD_REGINFO	| \
				CFG_CMD_IMMAP	| \
				CFG_CMD_ELF	| \
				CFG_CMD_MII	| \
				CFG_CMD_BEDBUG	| \
				CFG_CMD_NET	| \
				CFG_CMD_PING)

/* this must be included AFTER the definition of CONFIG_COMMANDS (if any) */
#include <cmd_confdefs.h>


/*
 * Serial console configuration
 */
#define CONFIG_PSC_CONSOLE	1	/* console is on PSC1 */
#define CONFIG_NETCONSOLE	1	/* network console */
#define CONFIG_BAUDRATE		115200
#define CFG_BAUDRATE_TABLE	{ 9600, 19200, 38400, 57600, 115200, 230400 }


/*
 * Ethernet configuration
 */
#define CONFIG_MPC5xxx_FEC	1
#define CONFIG_PHY_ADDR		0x2
#define CONFIG_PHY_TYPE		0x79c874


/*
 * Autobooting
 */
#define CONFIG_BOOTDELAY	2	/* autoboot after 2 seconds */
#define CONFIG_AUTOBOOT_KEYED
#define CONFIG_AUTOBOOT_STOP_STR	"\x1b\x1b"
#define DEBUG_BOOTKEYS		0
#undef CONFIG_AUTOBOOT_DELAY_STR
#undef CONFIG_BOOTARGS
#define CONFIG_AUTOBOOT_PROMPT	"Autobooting in %d seconds, "		\
					"press \"<Esc><Esc>\" to stop\n"

#define CONFIG_ETHADDR		00:50:C2:40:10:00
#define CONFIG_OVERWRITE_ETHADDR_ONCE	1
#define CONFIG_VERSION_VARIABLE	1	/* include version env variable */


/*
 * Default environment settings
 */
#define CONFIG_EXTRA_ENV_SETTINGS					\
	"sdram_test=0\0"						\
	"netdev=eth0\0"							\
	"hostname=motionpro\0"						\
	"netmask=255.255.0.0\0"						\
	"ipaddr=192.168.160.22\0"					\
	"serverip=192.168.1.1\0"					\
	"gatewayip=192.168.1.1\0"					\
	"kernel_addr=200000\0"						\
	"u-boot_addr=100000\0"						\
	"kernel_sector=20\0"						\
	"kernel_size=1000\0"						\
	"console=ttyS0,115200\0"					\
	"rootpath=/opt/eldk-4.1/ppc_6xx\0"				\
	"bootfile=/tftpboot/motionpro/uImage\0"				\
	"u-boot=/tftpboot/motionpro/u-boot.bin\0"			\
	"load=tftp $(u-boot_addr) $(u-boot)\0"				\
	"update=prot off fff00000 fff3ffff; era fff00000 fff3ffff; "	\
		"cp.b $(u-boot_addr) fff00000 $(filesize);"		\
		"prot on fff00000 fff3ffff\0"				\
	"ramargs=setenv bootargs root=/dev/ram rw\0"			\
	"addip=setenv bootargs $(bootargs) console=$(console) "		\
		"ip=$(ipaddr):$(serverip):$(gatewayip):"		\
		"$(netmask):$(hostname):$(netdev):off panic=1\0"	\
	"flash_nfs=run nfsargs addip;bootm $(kernel_addr)\0"		\
	"flash_self=run ramargs addip;bootm $(kernel_addr) "		\
		"$(ramdisk_addr)\0"					\
	"net_nfs=tftp $(kernel_addr) $(bootfile); run nfsargs addip; "	\
		"bootm $(kernel_addr)\0"				\
	"nfsargs=setenv bootargs root=/dev/nfs rw "			\
		"nfsroot=$(serverip):$(rootpath)\0"			\
	"fstype=ext3\0"							\
	"fatargs=setenv bootargs init=/linuxrc rw\0"			\
	""
#define CONFIG_BOOTCOMMAND	"run net_nfs"


/*
 * do board-specific init
 */
#define CONFIG_BOARD_EARLY_INIT_R	1


/*
 * Low level configuration
 */


/*
 * Clock configuration: SYS_XTALIN = 25MHz
 */
#define CFG_MPC5XXX_CLKIN	25000000


/*
 * Memory map
 */
/*
 * Warning!!! with the current BestComm Task, MBAR MUST BE set to 0xf0000000.
 * Setting MBAR to otherwise will cause system hang when using SmartDMA such
 * as network commands.
 */
#define CFG_MBAR		0xf0000000
#define CFG_SDRAM_BASE		0x00000000

/*
 * If building for running out of SDRAM, then MBAR has been set up beforehand
 * (e.g., by the BDI). Otherwise we must specify the default boot-up value of
 * MBAR, as given in the doccumentation.
 */
#if TEXT_BASE == 0x00100000
#define CFG_DEFAULT_MBAR	0xf0000000
#else /* TEXT_BASE != 0x00100000 */
#define CFG_DEFAULT_MBAR	0x80000000
#define CFG_LOWBOOT		1
#endif /* TEXT_BASE == 0x00100000 */

/* Use SRAM until RAM will be available */
#define CFG_INIT_RAM_ADDR	MPC5XXX_SRAM
#define CFG_INIT_RAM_END	MPC5XXX_SRAM_SIZE

#define CFG_GBL_DATA_SIZE	128	/* size in bytes for initial data */
#define CFG_GBL_DATA_OFFSET	(CFG_INIT_RAM_END - CFG_GBL_DATA_SIZE)
#define CFG_INIT_SP_OFFSET	CFG_GBL_DATA_OFFSET

#define CFG_MONITOR_BASE	TEXT_BASE
#if (CFG_MONITOR_BASE < CFG_FLASH_BASE)
#define CFG_RAMBOOT		1
#endif

#define CFG_MONITOR_LEN		(256 << 10)	/* 256 kB for Monitor */
#define CFG_MALLOC_LEN		(128 << 10)	/* 128 kB for malloc() */
#define CFG_BOOTMAPSZ		(8 << 20)	/* initial mem map for Linux */


/*
 * Chip selects configuration
 */
/* Boot Chipselect */
#define CFG_BOOTCS_START	CFG_FLASH_BASE
#define CFG_BOOTCS_SIZE		CFG_FLASH_SIZE
#define CFG_BOOTCS_CFG		0x03035D00

/* Flash memory addressing */
#define CFG_CS0_START		CFG_FLASH_BASE
#define CFG_CS0_SIZE		CFG_FLASH_SIZE
#define CFG_CS0_CFG		CFG_BOOTCS_CFG

/* Dual Port SRAM -- Kollmorgen Drive memory addressing */
#define CFG_CS1_START		0x50000000
#define CFG_CS1_SIZE		0x10000
#define CFG_CS1_CFG		0x05055800

/* Local register access */
#define CFG_CS2_START		0x50010000
#define CFG_CS2_SIZE		0x10000
#define CFG_CS2_CFG		0x05055800

/* Anybus CompactCom Module memory addressing */
#define CFG_CS3_START		0x50020000
#define CFG_CS3_SIZE		0x10000
#define CFG_CS3_CFG		0x05055800

/* No burst and dead cycle = 2 for all CSs */
#define CFG_CS_BURST		0x00000000
#define CFG_CS_DEADCYCLE	0x22222222


/*
 * SDRAM configuration
 */
/* 2 x MT48LC16M16A2BG-75 IT:D, CASL 2, 32 bit data bus */
#define SDRAM_CONFIG1		0x52222600
#define SDRAM_CONFIG2		0x88b70000
#define SDRAM_CONTROL		0x50570000
#define SDRAM_MODE		0x008d0000


/*
 * Flash configuration
 */
#define CFG_FLASH_CFI		1	/* Flash is CFI conformant */
#define CFG_FLASH_CFI_DRIVER	1
#define CFG_FLASH_BASE		0xff000000
#define CFG_FLASH_SIZE		0x01000000
#define CFG_MAX_FLASH_BANKS	1	/* max num of memory banks */
#define CFG_FLASH_BANKS_LIST	{ CFG_FLASH_BASE }
#define CFG_MAX_FLASH_SECT	256	/* max num of sects on one chip */
#define CONFIG_FLASH_16BIT		/* Flash is 16-bit */


/*
 * Environment settings
 */
#define CFG_ENV_IS_IN_FLASH	1
/* This has to be a multiple of the Flash sector size */
#define CFG_ENV_ADDR		(CFG_MONITOR_BASE + CFG_MONITOR_LEN)
#define CFG_ENV_SIZE		0x1000
#define CFG_ENV_SECT_SIZE	0x10000


/*
 * Pin multiplexing configuration
 */

/* PSC1: UART1
 * PSC2: GPIO (default)
 * PSC3: GPIO (default)
 * USB: 2xUART4/5
 * Ethernet: Ethernet 100Mbit with MD
 * Timer: CAN2/GPIO
 * PSC6/IRDA: GPIO (default)
 */
#define CFG_GPS_PORT_CONFIG	0x1105a004


/*
 * Miscellaneous configurable options
 */
#define CFG_LONGHELP			/* undef to save memory    */
#define CFG_PROMPT		"=> "	/* Monitor Command Prompt   */
#define CFG_CBSIZE		256	/* Console I/O Buffer Size  */
#define CFG_PBSIZE (CFG_CBSIZE+sizeof(CFG_PROMPT)+16)	/* Print Buffer Size */
#define CFG_MAXARGS		16		/* max number of command args */
#define CFG_BARGSIZE		CFG_CBSIZE	/* Boot Argument Buffer Size */

#define CFG_MEMTEST_START	0x00100000	/* memtest works on */
#define CFG_MEMTEST_END		0x03f00000	/* 1 ... 64 MiB in DRAM */

#define CFG_LOAD_ADDR		0x200000	/* default kernel load addr */

#define CFG_HZ			1000	/* decrementer freq: 1 ms ticks */


/*
 * Various low-level settings
 */
#define CFG_HID0_INIT		HID0_ICE | HID0_ICFI
#define CFG_HID0_FINAL		HID0_ICE

#define BOOTFLAG_COLD		0x01	/* Normal Power-On: Boot from FLASH  */
#define BOOTFLAG_WARM		0x02	/* Software reboot */

#define CFG_CACHELINE_SIZE	32	/* For MPC5xxx CPUs */


/* Not needed for MPC 5xxx U-Boot, but used by tools/updater */
#define CFG_RESET_ADDRESS	0xfff00100

#endif /* __CONFIG_H */
