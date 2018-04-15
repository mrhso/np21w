#include	"compiler.h"
#include	"dosio.h"
#include	"fdd/sxsi.h"

#ifdef SUPPORT_PHYSICAL_CDDRV

#include	<winioctl.h>
#include	<api/ntddcdrm.h>

#include	"DiskImage/cddfile.h"
#include	"DiskImage/CD/cdd_real.h"

UINT64 msf2lba(UINT64 msf){
	UINT64 m,s,f;
	m = (msf >> 16) & 0xffffffffffff;
	s = (msf >>  8) & 0xff;
	f = (msf >>  0) & 0xff;
	return (((m * 60) + s) * 75) + f;
}

//	実ドライブを開く
BRESULT openrealcdd(SXSIDEV sxsi, const OEMCHAR *path) {

	_CDTRK	trk[99];
	UINT	trks;
	FILEH	fh;
	UINT16	sector_size;
	FILELEN	totals;
	DISK_GEOMETRY dgCDROM;
	CDROM_TOC tocCDROM;
	DWORD dwNotUsed;
	int i;

	ZeroMemory(trk, sizeof(trk));
	trks = 0;

	fh = file_open_rb(path);
	if (fh == FILEH_INVALID) {
		goto openiso_err1;
	}

	//	セクタサイズが2048byte、2352byte、2448byteのどれかをチェック
	DeviceIoControl(fh, IOCTL_CDROM_GET_DRIVE_GEOMETRY,
				NULL, 0, &dgCDROM, sizeof(DISK_GEOMETRY),
				&dwNotUsed, NULL);

	if(dgCDROM.MediaType != 11){ // Removable Media Check
		goto openiso_err2;
	}
	sector_size = dgCDROM.BytesPerSector;
	totals = dgCDROM.SectorsPerTrack*dgCDROM.TracksPerCylinder*dgCDROM.Cylinders.QuadPart;
	switch(sector_size){
	case 2048:
		sxsi->read = sec2048_read;
		break;
	case 2352:
		sxsi->read = sec2352_read;
		break;
	case 2448:
		sxsi->read = sec2448_read;
		break;
	default:
		goto openiso_err2;
	}
	
	//	トラック情報を拾う
	DeviceIoControl(fh, IOCTL_CDROM_READ_TOC,
				NULL, 0, &tocCDROM, sizeof(tocCDROM),
				&dwNotUsed, NULL);

	trks = tocCDROM.LastTrack - tocCDROM.FirstTrack + 1;
	for(i=0;i<trks;i++){
		if((tocCDROM.TrackData[i].Control & 0xC) == 0x4){
			trk[i].adr_ctl		= TRACKTYPE_DATA;
		}else{
			trk[i].adr_ctl		= TRACKTYPE_AUDIO;
		}
		trk[i].point			= tocCDROM.TrackData[i].TrackNumber;
		trk[i].pos				= (msf2lba(LOADMOTOROLADWORD(tocCDROM.TrackData[i].Address)) - 150);
		trk[i].pos0				= trk[i].pos;

		trk[i].sector_size	= sector_size;

		trk[i].pregap_sector	= trk[i].pos;
		//trk[i].start_sector		= trk[i].pos;
		if(i==trks-1){
			trk[i].end_sector		= totals;
		}else{
			trk[i].end_sector		= (msf2lba(LOADMOTOROLADWORD(tocCDROM.TrackData[i+1].Address)) - 150 - 1);
		}

		trk[i].img_pregap_sec	= trk[i].pregap_sector;
		trk[i].img_start_sec	= trk[i].start_sector;
		trk[i].img_end_sec		= trk[i].end_sector;

		trk[i].pregap_sectors	= 0;
		trk[i].track_sectors	= trk[i].end_sector - trk[i].start_sector + 1;
		
		trk[i].str_sec		= trk[i].start_sector;
		trk[i].end_sec		= trk[i].end_sector;
		trk[i].sectors		= trk[i].track_sectors;
		
		trk[i].pregap_offset	= trk[i].start_sector * trk[i].sector_size;
		trk[i].start_offset		= trk[i].start_sector * trk[i].sector_size;
		trk[i].end_offset		= trk[i].end_sector * trk[i].sector_size;
	}

	//trk[0].adr_ctl			= TRACKTYPE_DATA;
	//trk[0].point			= 1;
	//trk[0].pos				= 0;
	//trk[0].pos0				= 0;

	//trk[0].sector_size		= sector_size;

	//trk[0].pregap_sector	= 0;
	//trk[0].start_sector		= 0;
	//trk[0].end_sector		= totals;

	//trk[0].img_pregap_sec	= 0;
	//trk[0].img_start_sec	= 0;
	//trk[0].img_end_sec		= totals;

	//trk[0].pregap_offset	= 0;
	//trk[0].start_offset		= 0;
	//trk[0].end_offset		= totals * sector_size;

	//trk[0].pregap_sectors	= 0;
	//trk[0].track_sectors	= totals;
	//trks = 1;

	sxsi->totals = trk[trks-1].end_sector;

	file_close(fh);

	return(setsxsidev(sxsi, path, trk, trks));

openiso_err2:
	file_close(fh);

openiso_err1:
	return(FAILURE);
}

#endif