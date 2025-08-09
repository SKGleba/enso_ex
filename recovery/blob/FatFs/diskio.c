/*-----------------------------------------------------------------------*/
/* Low level disk I/O module SKELETON for FatFs     (C)ChaN, 2025        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "ff.h"			/* Basic definitions of FatFs */
#include "diskio.h"		/* Declarations FatFs MAI */

/* Example: Declarations of the platform and disk functions in the project */
#include "stor.h"
#include "bm_ext.h"

/* Example: Mapping of physical drive number for each drive */
#define DEV_MNT0 0
#define DEV_MNT1 1
#define DEV_MNT2 2


/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber to identify the drive */
)
{
	DSTATUS stat;

	if (!stor_ff_init_mount(pdrv))
		return RES_OK;

	return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber to identify the drive */
)
{
    DSTATUS stat;
	int result;

    if (!stor_ff_init_mount(pdrv))
        return RES_OK;

    return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	LBA_t sector,	/* Start sector in LBA */
	UINT count		/* Number of sectors to read */
)
{
	DRESULT res;
	int result;

	if (stor_read_mount(pdrv, sector, buff, count) >= 0)
		return RES_OK;

	return RES_PARERR;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0

DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber to identify the drive */
	const BYTE *buff,	/* Data to be written */
	LBA_t sector,		/* Start sector in LBA */
	UINT count			/* Number of sectors to write */
)
{
	DRESULT res;
	int result;

	if (stor_write_mount(pdrv, sector, buff, count) >= 0)
		return RES_OK;

	return RES_PARERR;
}

#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	DRESULT res;
	int result;

	switch (cmd) {
	case CTRL_SYNC: // no cache here
	case CTRL_TRIM: // trim disabled
		return RES_OK;
	case GET_SECTOR_SIZE:
		if (buff) {
			*(WORD *)buff = SECTOR_SIZE; // return sector size
			return RES_OK;
		}
		return RES_PARERR;
	case GET_SECTOR_COUNT:
		{
        	struct mount_ctx *ctx = stor_get_validate_mctx(pdrv);
        	if (ctx) {
                if (buff) {
                    *(DWORD *)buff = ctx->params->sz;
                    return RES_OK;
                }
                return RES_PARERR;
            }
            return RES_NOTRDY;
        }
	case GET_BLOCK_SIZE:
        if (buff) {
            *(DWORD *)buff = 1;
            return RES_OK;
        }
		return RES_PARERR;
    }

    return RES_PARERR;
}

char *ff_strchr(const char *s, int c) {
    while (*s) {
        if (*s == (char)c)
            return (char *)s;
        s++;
    }
    return NULL;
}

static struct todays_fattime_s {
	uint32_t fattime; // fattime for today at 00:00:00
	uint32_t tick; // tick value for today at 00:00:00
} todays_fattime = {0, 0};
#define SECONDS_PER_DAY 86400
#define YEAR_IS_LEAP(year) (((year) % 4 == 0 && (year) % 100 != 0) || (year) % 400 == 0)
DWORD get_fattime(void) {
    uint32_t t = bmx_get_time(NULL);  // seconds since 1970-01-01 00:00:00
    int year = 1970, mon = 0, day, hour, min, sec;
    uint32_t secs;
	secs = t % SECONDS_PER_DAY;
	hour = secs / 3600;
	min = (secs % 3600) / 60;
	sec = secs % 60;
    if (!todays_fattime.tick || ((t - todays_fattime.tick) >= SECONDS_PER_DAY)) {
    	uint32_t days;

        days = t / SECONDS_PER_DAY;

        // Calculate year
        while (1) {
            int yd = YEAR_IS_LEAP(year) ? 366 : 365;
            if (days >= yd) {
                days -= yd;
                year++;
            } else {
                break;
            }
    	}

        static const uint8_t days_in_month[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

        // Calculate month
    	for (mon = 0; mon < 12; mon++) {
        	int md = (int)days_in_month[mon];
            if (mon == 1 && (YEAR_IS_LEAP(year))) {
                md++;  // February in leap year
            }
        	if (days < md)
            	break;
        	days -= md;
    	}
    	day = days + 1;
	} else {
		year = 1980 + (todays_fattime.fattime >> 25);
		mon = ((todays_fattime.fattime >> 21) & 0x0F) - 1; // 0-11
		day = (todays_fattime.fattime >> 16) & 0x1F; // 1-31
	}

    // FAT time: YYYYYYYMMMMDDDDDHHHHHMMMMMMSSSSS
    // Year since 1980, Month 1-12, Day 1-31, Hour 0-23, Min 0-59, Sec/2 0-29
    DWORD fattime = ((year - 1980) << 25) | ((mon + 1) << 21) | (day << 16) | (hour << 11) | (min << 5) | (sec / 2);

    return fattime;
}