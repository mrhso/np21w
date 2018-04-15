#include	"compiler.h"
#if defined(OSLANG_UTF8) || defined(OSLANG_UCS2)
#include	"oemtext.h"
#endif
#include	"dosio.h"
#include	"newdisk.h"
#include	"diskimage/fddfile.h"
#include	"sxsi.h"
#include	"hddboot.res"

#ifdef SUPPORT_VPCVHD
#include "hdd_vpc.h"
#endif


// ---- fdd

void newdisk_fdd(const OEMCHAR *fname, REG8 type, const OEMCHAR *label) {

	_D88HEAD	d88head;
	FILEH		fh;

	ZeroMemory(&d88head, sizeof(d88head));
	STOREINTELDWORD(d88head.fd_size, sizeof(d88head));
#if defined(OSLANG_UTF8) || defined(OSLANG_UCS2)
	oemtext_oemtosjis((char *)d88head.fd_name, sizeof(d88head.fd_name),
															label, (UINT)-1);
#else
	milstr_ncpy((char *)d88head.fd_name, label, sizeof(d88head.fd_name));
#endif
	d88head.fd_type = type;
	fh = file_create(fname);
	if (fh != FILEH_INVALID) {
		file_write(fh, &d88head, sizeof(d88head));
		file_close(fh);
	}
}


// ---- hdd

static BRESULT writezero(FILEH fh, FILELEN size) {

	UINT8	work[256];
	FILELEN	wsize;

	ZeroMemory(work, sizeof(work));
	while(size) {
		wsize = min(size, sizeof(work));
		if (file_write(fh, work, (UINT)wsize) != wsize) {
			return(FAILURE);
		}
		size -= wsize;
	}
	return(SUCCESS);
}

static BRESULT writehddipl(FILEH fh, UINT ssize, FILELEN tsize) {

	UINT8	work[65536];
	FILELEN	size;

	ZeroMemory(work, sizeof(work));
	CopyMemory(work, hdddiskboot, sizeof(hdddiskboot));
	if (ssize < 1024) {
		work[ssize - 2] = 0x55;
		work[ssize - 1] = 0xaa;
	}
	if (file_write(fh, work, sizeof(work)) != sizeof(work)) {
		return(FAILURE);
	}
	if (tsize > sizeof(work)) {
		tsize -= sizeof(work);
		ZeroMemory(work, sizeof(work));
		while(tsize) {
			size = min(tsize, sizeof(work));
			tsize -= size;
			if (file_write(fh, work, (UINT)size) != size) {
				return(FAILURE);
			}
		}
	}
	return(SUCCESS);
}

void newdisk_thd(const OEMCHAR *fname, UINT hddsize) {
	
	FILEH	fh;
	UINT8	work[256];
	UINT	size;
	BRESULT	r;

	if ((fname == NULL) || (hddsize < 5) || (hddsize > 256)) {
		goto ndthd_err;
	}
	fh = file_create(fname);
	if (fh == FILEH_INVALID) {
		goto ndthd_err;
	}
	ZeroMemory(work, 256);
	size = hddsize * 15;
	STOREINTELWORD(work, size);
	r = (file_write(fh, work, 256) == 256) ? SUCCESS : FAILURE;
	r |= writehddipl(fh, 256, 0);
	file_close(fh);
	if (r != SUCCESS) {
		file_delete(fname);
	}

ndthd_err:
	return;
}

void newdisk_nhd(const OEMCHAR *fname, UINT hddsize) {

	FILEH	fh;
	NHDHDR	nhd;
	FILELEN	size;
	BRESULT	r;
	
	if ((fname == NULL) || (hddsize < 5) || (hddsize > NHD_MAXSIZE2)) {
		goto ndnhd_err;
	}
	fh = file_create(fname);
	if (fh == FILEH_INVALID) {
		goto ndnhd_err;
	}
	ZeroMemory(&nhd, sizeof(nhd));
	CopyMemory(&nhd.sig, sig_nhd, 15);
	STOREINTELDWORD(nhd.headersize, sizeof(nhd));
#ifdef SUPPORT_LARGE_HDD
	if(hddsize <= 4000){
		size = hddsize * 15;
		STOREINTELDWORD(nhd.cylinders, (UINT32)size);
		STOREINTELWORD(nhd.surfaces, 8);
		STOREINTELWORD(nhd.sectors, 17);
		STOREINTELWORD(nhd.sectorsize, 512);
		r = (file_write(fh, &nhd, sizeof(nhd)) == sizeof(nhd)) ? SUCCESS : FAILURE;
		r |= writehddipl(fh, 512, size * 8 * 17 * 512);
	}else if(hddsize <= 32000){
		size = hddsize * 15 * 17 / 2 / 63;
		STOREINTELDWORD(nhd.cylinders, (UINT32)size);
		STOREINTELWORD(nhd.surfaces, 16);
		STOREINTELWORD(nhd.sectors, 63);
		STOREINTELWORD(nhd.sectorsize, 512);
		r = (file_write(fh, &nhd, sizeof(nhd)) == sizeof(nhd)) ? SUCCESS : FAILURE;
		r |= writehddipl(fh, 512, (UINT64)size * 16 * 63 * 512);
	}else{
		size = hddsize * 15 * 17 / 2 / 255;
		STOREINTELDWORD(nhd.cylinders, (UINT32)size);
		STOREINTELWORD(nhd.surfaces, 16);
		STOREINTELWORD(nhd.sectors, 255);
		STOREINTELWORD(nhd.sectorsize, 512);
		r = (file_write(fh, &nhd, sizeof(nhd)) == sizeof(nhd)) ? SUCCESS : FAILURE;
		r |= writehddipl(fh, 512, (UINT64)size * 16 * 255 * 512);
	}
#else
	size = hddsize * 15;
	STOREINTELDWORD(nhd.cylinders, size);
	STOREINTELWORD(nhd.surfaces, 8);
	STOREINTELWORD(nhd.sectors, 17);
	STOREINTELWORD(nhd.sectorsize, 512);
	r = (file_write(fh, &nhd, sizeof(nhd)) == sizeof(nhd)) ? SUCCESS : FAILURE;
	r |= writehddipl(fh, 512, size * 8 * 17 * 512);
#endif
	file_close(fh);
	if (r != SUCCESS) {
		file_delete(fname);
	}

ndnhd_err:
	return;
}

// hddtype = 0:5MB / 1:10MB / 2:15MB / 3:20MB / 5:30MB / 6:40MB
void newdisk_hdi(const OEMCHAR *fname, UINT hddtype) {

const SASIHDD	*sasi;
	FILEH		fh;
	HDIHDR		hdi;
	UINT32		size;
	BRESULT		r;

	hddtype &= 7;
	if ((fname == NULL) || (hddtype == 7)) {
		goto ndhdi_err;
	}
	sasi = sasihdd + hddtype;
	fh = file_create(fname);
	if (fh == FILEH_INVALID) {
		goto ndhdi_err;
	}
	ZeroMemory(&hdi, sizeof(hdi));
	size = 256 * sasi->sectors * sasi->surfaces * sasi->cylinders;
//	STOREINTELDWORD(hdi.hddtype, 0);
	STOREINTELDWORD(hdi.headersize, 4096);
	STOREINTELDWORD(hdi.hddsize, size);
	STOREINTELDWORD(hdi.sectorsize, 256);
	STOREINTELDWORD(hdi.sectors, sasi->sectors);
	STOREINTELDWORD(hdi.surfaces, sasi->surfaces);
	STOREINTELDWORD(hdi.cylinders, sasi->cylinders);
	r = (file_write(fh, &hdi, sizeof(hdi)) == sizeof(hdi)) ? SUCCESS : FAILURE;
	r |= writezero(fh, 4096 - sizeof(hdi));
	r |= writehddipl(fh, 256, size);
	file_close(fh);
	if (r != SUCCESS) {
		file_delete(fname);
	}

ndhdi_err:
	return;
}

void newdisk_vhd(const OEMCHAR *fname, UINT hddsize) {

	FILEH	fh;
	VHDHDR	vhd;
	UINT	tmp;
	BRESULT	r;

	if ((fname == NULL) || (hddsize < 2) || (hddsize > 512)) {
		goto ndvhd_err;
	}
	fh = file_create(fname);
	if (fh == FILEH_INVALID) {
		goto ndvhd_err;
	}
	ZeroMemory(&vhd, sizeof(vhd));
	CopyMemory(&vhd.sig, sig_vhd, 7);
	STOREINTELWORD(vhd.mbsize, (UINT16)hddsize);
	STOREINTELWORD(vhd.sectorsize, 256);
	vhd.sectors = 32;
	vhd.surfaces = 8;
	tmp = hddsize *	16;		// = * 1024 * 1024 / (8 * 32 * 256);
	STOREINTELWORD(vhd.cylinders, (UINT16)tmp);
	tmp *= 8 * 32;
	STOREINTELDWORD(vhd.totals, tmp);
	r = (file_write(fh, &vhd, sizeof(vhd)) == sizeof(vhd)) ? SUCCESS : FAILURE;
	r |= writehddipl(fh, 256, 0);
	file_close(fh);
	if (r != SUCCESS) {
		file_delete(fname);
	}

ndvhd_err:
	return;
}

#ifdef SUPPORT_VPCVHD
const char vpcvhd_sig[8] = "conectix";
const char vpcvhd_creator[4] = "vpc ";
const char vpcvhd_os[4] = "Wi2k";

void newdisk_vpcvhd(const OEMCHAR *fname, UINT hddsize) {

   FILEH   fh;
   VPCVHDFOOTER    vpcvhd;
   FILELEN size;
   BRESULT r;
   UINT64  origsize;
   UINT32  checksum;
   size_t footerlen;

   if ((fname == NULL) || (hddsize < 5) || (hddsize > NHD_MAXSIZE2)) {
       goto vpcvhd_err;
   }
   fh = file_create(fname);
   if (fh == FILEH_INVALID) {
       goto vpcvhd_err;
   }

   footerlen = sizeof(VPCVHDFOOTER);
   ZeroMemory(&vpcvhd, footerlen);

   CopyMemory(&vpcvhd.Cookie, vpcvhd_sig, 8);
   STOREMOTOROLADWORD(vpcvhd.Features, 2);
   STOREMOTOROLADWORD(vpcvhd.FileFormatVersion, 0x10000);
   CopyMemory(&vpcvhd.CreatorApplication, vpcvhd_creator, 4);
   STOREMOTOROLADWORD(vpcvhd.CreatorVersion, 0x50003);
   CopyMemory(&vpcvhd.CreatorHostOS, vpcvhd_os, 4);
   STOREMOTOROLAQWORD(vpcvhd.DataOffset, (SINT64)-1);
   STOREMOTOROLADWORD(vpcvhd.DiskType, 2);

#ifdef SUPPORT_LARGE_HDD
   if(hddsize <= 4000){
       size = hddsize * 15;
       STOREMOTOROLAWORD(vpcvhd.Cylinder, (UINT16)size);
       vpcvhd.Heads = 8;
       vpcvhd.SectorsPerCylinder = 17;
       origsize = size * 8 * 17 * 512;
       STOREMOTOROLAQWORD(vpcvhd.OriginalSize, origsize);
       CopyMemory(&vpcvhd.CurrentSize, &vpcvhd.OriginalSize, 8);
       r = writehddipl(fh, 512, origsize);
   }else if(hddsize <= 32000){
       size = hddsize * 15 * 17 / 2 / 63;
       STOREMOTOROLAWORD(vpcvhd.Cylinder, (UINT16)size);
       vpcvhd.Heads = 16;
       vpcvhd.SectorsPerCylinder = 63;
       origsize = (UINT64)size * 16 * 63 * 512;
       STOREMOTOROLAQWORD(vpcvhd.OriginalSize, origsize);
       CopyMemory(&vpcvhd.CurrentSize, &vpcvhd.OriginalSize, 8);
       r = writehddipl(fh, 512, origsize);
   }else{
       size = hddsize * 15 * 17 / 2 / 255;
       STOREMOTOROLAWORD(vpcvhd.Cylinder, (UINT16)size);
       vpcvhd.Heads = 16;
       vpcvhd.SectorsPerCylinder = 255;
       origsize = (UINT64)size * 16 * 255 * 512;
       STOREMOTOROLAQWORD(vpcvhd.OriginalSize, origsize);
       CopyMemory(&vpcvhd.CurrentSize, &vpcvhd.OriginalSize, 8);
       r = writehddipl(fh, 512, origsize);
   }
#else
   size = hddsize * 15;
   STOREMOTOROLAWORD(vpcvhd.Cylinder, (UINT16)size);
   vpcvhd.Heads = 8;
   vpcvhd.SectorsPerCylinder = 17;
   origsize = size * 8 * 17 * 512;
   STOREMOTOROLAQWORD(vpcvhd.OriginalSize, origsize);
   CopyMemory(&vpcvhd.CurrentSize, &vpcvhd.OriginalSize, 8);
   r = writehddipl(fh, 512, origsize);
#endif

   checksum = vpc_calc_checksum(&vpcvhd,footerlen);
   STOREMOTOROLADWORD(vpcvhd.CheckSum, checksum);

   r |= (file_write(fh, &vpcvhd, sizeof(vpcvhd)) == sizeof(vpcvhd)) ? SUCCESS : FAILURE;

   file_close(fh);
   if (r != SUCCESS) {
       file_delete(fname);
   }

vpcvhd_err:
   return;
}
#endif
