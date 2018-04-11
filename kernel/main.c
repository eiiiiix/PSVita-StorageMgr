/*
	StorageMgrKernel by CelesteBlue
	
	Credits:
	gamesd by motoharu / xyz
	usbmc by yifanlu / TheFloW
	VitaShell kernel plugin by TheFloW

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <string.h>
#include <psp2kern/kernel/modulemgr.h>
#include <psp2kern/kernel/sysmem.h>
#include <psp2kern/io/fcntl.h>

#include <taihen.h>


static void log_write(const char *buffer, size_t length, const char *folderpath, const char *fullpath);
const char *log_folder_ur0_path = "ur0:tai/";
const char *log_ur0_path = "ur0:tai/storagemgr_log.txt";

#define LOG(...) \
	do { \
		char buffer[256]; \
		snprintf(buffer, sizeof(buffer), ##__VA_ARGS__); \
		log_write(buffer, strlen(buffer), log_folder_ur0_path, log_ur0_path); \
	} while (0)

int module_get_export_func(SceUID pid, const char *modname, uint32_t libnid, uint32_t funcnid, uintptr_t *func);
int module_get_offset(SceUID pid, SceUID modid, int segidx, size_t offset, uintptr_t *addr);
const char* default_config_path = "ur0:tai/storage_config.txt";

typedef struct {
	const char *dev;
	const char *dev2;
	const char *blkdev;
	const char *blkdev2;
	int id;
} SceIoDevice;

typedef struct {
	int id;
	const char *dev_unix;
	int unk;
	int dev_major;
	int dev_minor;
	const char *dev_filesystem;
	int unk2;
	SceIoDevice *dev;
	int unk3;
	SceIoDevice *dev2;
	int unk4;
	int unk5;
	int unk6;
	int unk7;
} SceIoMountPoint;

#define SD0_DEV "sd0:"
#define SD0_DEV2 "exfatsd0"
#define SD0_ID 0x100
#define OS0_DEV "os0:"
#define OS0_DEV2 "exfatos0"
#define OS0_ID 0x200
#define TM0_DEV "tm0:"
#define TM0_DEV2 "exfattm0"
#define TM0_ID 0x500
#define UR0_DEV "ur0:"
#define UR0_DEV2 "exfatur0"
#define UR0_ID 0x600
#define UD0_DEV "ud0:"
#define UD0_DEV2 "exfatud0"
#define UD0_ID 0x700
#define UX0_DEV "ux0:"
#define UX0_DEV2 "exfatux0"
#define UX0_ID 0x800
#define GRO0_DEV "gro0:"
#define GRO0_DEV2 "exfatgro0"
#define GRO0_ID 0x900
#define GRW0_DEV "grw0:"
#define GRW0_DEV2 "exfatgrw0"
#define GRW0_ID 0xA00
#define SA0_DEV "sa0:"
#define SA0_DEV2 "exfatsa0"
#define SA0_ID 0xB00
#define PD0_DEV "pd0:"
#define PD0_DEV2 "exfatpd0"
#define PD0_ID 0xC00
#define IMC0_DEV "imc0:"
#define IMC0_DEV2 "exfatimc0"
#define IMC0_ID 0xD00
#define XMC0_DEV "xmc0:"
#define XMC0_DEV2 "exfatxmc0"
#define XMC0_ID 0xE00
#define UMA0_DEV "uma0:"
#define UMA0_DEV2 "exfatuma0"
#define UMA0_ID 0xF00
#define VD0_DEV "vd0:"
#define VD0_DEV2 "exfatvd0"
#define VD0_ID 0x400
#define VS0_DEV "vs0:"
#define VS0_DEV2 "exfatvs0"
#define VS0_ID 0x300
#define BLANK0_DEV "blank0:"
#define BLANK0_DEV2 "exfatblank0"
#define BLANK0_ID 0x000
#define MCD_BLKDEV "sdstor0:xmc-lp-ign-userext"
#define MCD_BLKDEV2 NULL
#define INT_BLKDEV "sdstor0:int-lp-ign-userext"
#define INT_BLKDEV2 NULL
#define UMA_BLKDEV "sdstor0:uma-pp-act-a"
#define UMA_BLKDEV2 "sdstor0:uma-lp-act-entire"
#define GCD_BLKDEV "sdstor0:gcd-lp-ign-entire"
#define GCD_BLKDEV2 NULL

static SceIoMountPoint *(* sceIoFindMountPoint)(int id) = NULL;

#define MOUNT_POINT_ID 0x800
static SceIoDevice uma_ux0_dev = { "ux0:", "exfatux0", "sdstor0:gcd-lp-ign-entire", "sdstor0:gcd-lp-ign-entire", MOUNT_POINT_ID };
static SceIoDevice *ori_dev = NULL, *ori_dev2 = NULL;

static SceIoDevice *tm0_ori_dev = NULL, *tm0_ori_dev2 = NULL;
static SceIoDevice *ur0_ori_dev = NULL, *ur0_ori_dev2 = NULL;
static SceIoDevice *ux0_ori_dev = NULL, *ux0_ori_dev2 = NULL;
static SceIoDevice *gro0_ori_dev = NULL, *gro0_ori_dev2 = NULL;
static SceIoDevice *grw0_ori_dev = NULL, *grw0_ori_dev2 = NULL;
static SceIoDevice *uma0_ori_dev = NULL, *uma0_ori_dev2 = NULL;
static SceIoDevice *imc0_ori_dev = NULL, *imc0_ori_dev2 = NULL;
static SceIoDevice *xmc0_ori_dev = NULL, *xmc0_ori_dev2 = NULL;
static SceIoDevice *sd0_ori_dev = NULL, *sd0_ori_dev2 = NULL;
static SceIoDevice *os0_ori_dev = NULL, *os0_ori_dev2 = NULL;
static SceIoDevice *pd0_ori_dev = NULL, *pd0_ori_dev2 = NULL;
static SceIoDevice *sa0_ori_dev = NULL, *sa0_ori_dev2 = NULL;
static SceIoDevice *ud0_ori_dev = NULL, *ud0_ori_dev2 = NULL;
static SceIoDevice *vd0_ori_dev = NULL, *vd0_ori_dev2 = NULL;
static SceIoDevice *vs0_ori_dev = NULL, *vs0_ori_dev2 = NULL;
static SceIoDevice *blank0_ori_dev = NULL, *blank0_ori_dev2 = NULL;

static SceIoDevice *tm0_prev_dev = NULL, *tm0_prev_dev2 = NULL;
static SceIoDevice *ur0_prev_dev = NULL, *ur0_prev_dev2 = NULL;
static SceIoDevice *ux0_prev_dev = NULL, *ux0_prev_dev2 = NULL;
static SceIoDevice *gro0_prev_dev = NULL, *gro0_prev_dev2 = NULL;
static SceIoDevice *grw0_prev_dev = NULL, *grw0_prev_dev2 = NULL;
static SceIoDevice *uma0_prev_dev = NULL, *uma0_prev_dev2 = NULL;
static SceIoDevice *imc0_prev_dev = NULL, *imc0_prev_dev2 = NULL;
static SceIoDevice *xmc0_prev_dev = NULL, *xmc0_prev_dev2 = NULL;
static SceIoDevice *sd0_prev_dev = NULL, *sd0_prev_dev2 = NULL;
static SceIoDevice *os0_prev_dev = NULL, *os0_prev_dev2 = NULL;
static SceIoDevice *pd0_prev_dev = NULL, *pd0_prev_dev2 = NULL;
static SceIoDevice *sa0_prev_dev = NULL, *sa0_prev_dev2 = NULL;
static SceIoDevice *ud0_prev_dev = NULL, *ud0_prev_dev2 = NULL;
static SceIoDevice *vd0_prev_dev = NULL, *vd0_prev_dev2 = NULL;
static SceIoDevice *vs0_prev_dev = NULL, *vs0_prev_dev2 = NULL;
static SceIoDevice *blank0_prev_dev = NULL, *blank0_prev_dev2 = NULL;

int UMAuma0 = 0;

int getFileSize(const char *file) {
	SceUID fd = ksceIoOpen(file, SCE_O_RDONLY, 0);
	if (fd < 0)
		return fd;
	int fileSize = ksceIoLseek(fd, 0, SCE_SEEK_END);
	ksceIoClose(fd);
	return fileSize;
}

int checkConfigLineReturnChar(const char *file) {
	int configFileSize = getFileSize(file);
	if (configFileSize < 0) {
		LOG("No config file found.\n");
		return -1;
	}
	char buffer[configFileSize];
	SceUID fd = ksceIoOpen(file, SCE_O_RDWR | SCE_O_APPEND, 0);
	if (fd < 0)
		return fd;
	ksceIoRead(fd, buffer, configFileSize);
	if (memcmp((char[1]){buffer[configFileSize-1]}, (char[2]){0x0A}, 1) != 0)
		ksceIoWrite(fd, (char[2]){'\n', '\0'}, 1);
	ksceIoClose(fd);
	return 0;
}

int config_read(const char *file) {
	LOG("Reading config...\n");
	int entriesNumber = 0;
	int configFileSize = getFileSize(file);
	if (configFileSize < 0) {
		LOG("No config file found.\n");
		return -1;
	}
	char buffer[configFileSize];
	SceUID fd = ksceIoOpen(file, SCE_O_RDONLY, 0);
	if (fd < 0)
		return fd;
	int read = ksceIoRead(fd, buffer, configFileSize);
	for (int i=0; i<configFileSize; i++) {
		if (memcmp((char[1]){buffer[i]}, (char[1]){0x0A}, 1) == 0)
			entriesNumber++;
	}
	ksceIoClose(fd);
	return entriesNumber;
}

int readLine(int lineId, char *lineBuffer) {
	LOG("Reading line...\n");
	int entriesNumber = config_read(default_config_path);
	int configFileSize = getFileSize(default_config_path);
	if (configFileSize < 0) {
		LOG("No config file found.\n");
		return -1;
	}
	char configBuffer[configFileSize];
	SceUID fd = ksceIoOpen(default_config_path, SCE_O_RDONLY, 0);
	if (fd < 0)
		return fd;
	int read = ksceIoRead(fd, configBuffer, configFileSize);
	int padding = 0, n = 0;
	for (int currentLine=0; currentLine<entriesNumber; currentLine++) {
		if (currentLine == lineId) {
			LOG("Reading line %i with padding %i...\n", currentLine, padding);
			for (n=0; memcmp((char[1]){configBuffer[padding+n]}, (char[1]){0x0A}, 1) != 0; n++) {}
			memcpy((uintptr_t)lineBuffer, configBuffer+padding, n);
			lineBuffer[n] = 0; // We write the '\0' null character.
			break;
		}
		for (; memcmp((char[1]){configBuffer[padding]}, (char[1]){0x0A}, 1) != 0; padding++) {}
		padding++; // We don't read the '\n' character.
	}
	ksceIoClose(fd);
	LOG("Line of size %i : %s\n", n, lineBuffer);
	return n;
}

int readDeviceByLine(int lineId, char *lineDevice) {
	char lineBuffer[32];
	int lineLength = readLine(lineId, lineBuffer);
	int i = 0;
	for (; memcmp((char[1]){lineBuffer[i]}, (char[1]){0x3D}, 1) != 0; i++) {
		lineDevice[i] = lineBuffer[i];
		if (i == lineLength-1) {
			LOG("Line %i is not valid.\n", lineId);
			return -1;
		}
	}
	lineDevice[i] = 0; // We write the '\0' null character.
	LOG("Current line device of string length %i : %s\n", i, lineDevice);
	return 0;
}

int isDeviceInConfig(const char *device) {
	int line = -1;
	int entriesNumber = config_read(default_config_path);
	for (int i=0; i<entriesNumber; i++) {
		char lineDevice[32];
		if (!readDeviceByLine(i, lineDevice)) {
			if (!memcmp(lineDevice, device, strlen(device))) {
				line = i;
				break;
			}
		}
	}
	return line;
}

int readMountPointByLine(int lineId, char *lineMountPoint) {
	LOG("Reading mount point for line %i...\n", lineId);
	char lineBuffer[32];
	int lineLength = readLine(lineId, lineBuffer);
	LOG("Line length : %i.\n", lineLength);
	int i = 0;
	for (; memcmp((char[1]){lineBuffer[i]}, (char[1]){0x3D}, 1) != 0; i++) {}
	i++; // We don't read the '=' character.
	LOG("Not read string length (device string) : %i.\n", i);
	int j = 0;
	for (; j<lineLength-i; j++) {
		lineMountPoint[j] = lineBuffer[i+j];
	}
	lineMountPoint[j] = 0; // We write the '\0' null character.
	LOG("Current line mount point of string length %i : %s\n", j, lineMountPoint);
	return 0;
}

int isMountPointInConfig(const char *mountPoint) {
	int line = -1;
	int entriesNumber = config_read(default_config_path);
	for (int i=0; i<entriesNumber; i++) {
		char lineMountPoint[16];
		if (!readMountPointByLine(i, lineMountPoint)) {
			LOG("Mount point string length : %i.\n", strlen(lineMountPoint));
			if (strlen(mountPoint) == strlen(lineMountPoint)) {
				if (!memcmp(lineMountPoint, mountPoint, strlen(mountPoint))) {
					line = i;
					break;
				}
			}
		}
	}
	return line;
}

static int exists(const char *path) {
	int fd = ksceIoOpen(path, SCE_O_RDONLY, 0);
	if (fd < 0)
		return 0;
	ksceIoClose(fd);
	return 1;
}

static void io_remount(int id) {
	ksceIoUmount(id, 0, 0, 0);
	ksceIoUmount(id, 1, 0, 0);
	ksceIoMount(id, NULL, 0, 0, 0, 0);
}

int shellKernelIsUx0Redirected() {
	SceIoMountPoint *mount = sceIoFindMountPoint(MOUNT_POINT_ID);
	if (!mount)
		return -1;
	if (mount->dev == &uma_ux0_dev && mount->dev2 == &uma_ux0_dev)
		return 1;
	return 0;
}
int shellKernelRedirectUx0() {
	SceIoMountPoint *mount = sceIoFindMountPoint(MOUNT_POINT_ID);
	if (!mount)
		return -1;
	if (mount->dev != &uma_ux0_dev && mount->dev2 != &uma_ux0_dev) {
		ori_dev = mount->dev;
		ori_dev2 = mount->dev2;
	}
	mount->dev = &uma_ux0_dev;
	mount->dev2 = &uma_ux0_dev;
	return 0;
}
int shellKernelUnredirectUx0() {
	LOG("Unredirect ux0 function.\n");
	SceIoMountPoint *mount = sceIoFindMountPoint(MOUNT_POINT_ID);
	if (!mount)
		return -1;
	if (ori_dev && ori_dev2) {
		mount->dev = ori_dev;
		mount->dev2 = ori_dev2;
		ori_dev = NULL;
		ori_dev2 = NULL;
	}
	return 0;
}

/*int shellKernelRedirectGcdToUx0() {
	SceIoMountPoint *mount = sceIoFindMountPoint(UX0_ID);
	if (!mount) return -1;
	static SceIoDevice gcd_ux0 = {UX0_DEV, UX0_DEV2, GCD_BLKDEV, GCD_BLKDEV2, UX0_ID};
	//if (mount->dev != &gcd_ux0 && mount->dev2 != &gcd_ux0) {
		ux0_prev_dev = mount->dev;
		ux0_prev_dev2 = mount->dev2;
	//}
	mount->dev = &gcd_ux0;
	mount->dev2 = &gcd_ux0;
	io_remount(UX0_ID);
	return 0;
}*/

int isDeviceValid(const char* device) {
	if (!memcmp(device, "UMA", strlen("UMA"))
	|| !memcmp(device, "GCD", strlen("GCD"))
	|| !memcmp(device, "INT", strlen("INT"))
	|| !memcmp(device, "MCD", strlen("MCD")))
		return 1;
	LOG("Invalid device : %s\n", device);
	return 0;
}

int isPartitionValid(const char* partition) {
	if (!memcmp(partition, TM0_DEV, strlen(TM0_DEV))
	|| !memcmp(partition, UR0_DEV, strlen(UR0_DEV))
	|| !memcmp(partition, UX0_DEV, strlen(UX0_DEV))
	|| !memcmp(partition, GRO0_DEV, strlen(GRO0_DEV))
	|| !memcmp(partition, GRW0_DEV, strlen(GRW0_DEV))
	|| !memcmp(partition, IMC0_DEV, strlen(IMC0_DEV))
	|| !memcmp(partition, XMC0_DEV, strlen(XMC0_DEV))
	|| !memcmp(partition, UMA0_DEV, strlen(UMA0_DEV)))
	|| !memcmp(partition, SD0_DEV, strlen(SD0_DEV)))
	|| !memcmp(partition, OS0_DEV, strlen(OS0_DEV)))
	|| !memcmp(partition, UD0_DEV, strlen(UD0_DEV)))
	|| !memcmp(partition, SA0_DEV, strlen(SA0_DEV)))
	|| !memcmp(partition, PD0_DEV, strlen(PD0_DEV)))
	|| !memcmp(partition, VD0_DEV, strlen(VD0_DEV)))
	|| !memcmp(partition, VS0_DEV, strlen(VS0_DEV)))
	|| !memcmp(partition, BLANK0_DEV, strlen(BLANK0_DEV)))
		return 1;
	LOG("Invalid partition : %s\n", partition);
	return 0;
}

int getMountPointIdForPartition(const char* partition) {
	if (!isPartitionValid(partition))
		return -1;
	if (!memcmp(partition, "tm0:", strlen("tm0:")))
		return TM0_ID;
	if (!memcmp(partition, "ur0:", strlen("ur0:")))
		return UR0_ID;
	if (!memcmp(partition, "ux0:", strlen("ux0:")))
		return UX0_ID;
	if (!memcmp(partition, "gro0:", strlen("gro0:")))
		return GRO0_ID;
	if (!memcmp(partition, "grw0:", strlen("grw0:")))
		return GRW0_ID;
	if (!memcmp(partition, "imc0:", strlen("imc0:")))
		return IMC0_ID;
	if (!memcmp(partition, "xmc0:", strlen("xmc0:")))
		return XMC0_ID;
	if (!memcmp(partition, "uma0:", strlen("uma0:")))
		return UMA0_ID;
	if (!memcmp(partition, "sd0:", strlen("sd0:")))
		return SD0_ID;
	if (!memcmp(partition, "os0:", strlen("os0:")))
		return OS0_ID;
	if (!memcmp(partition, "ud0:", strlen("ud0:")))
		return UD0_ID;
	if (!memcmp(partition, "sa0:", strlen("sa0:")))
		return SA0_ID; //secretly a sony guy who was a weeb inserting a SAO name for a partition?
	if (!memcmp(partition, "pd0:", strlen("pd0:")))
		return PD0_ID;
	if (!memcmp(partition, "vd0:", strlen("vd0:")))
		return VD0_ID;
	if (!memcmp(partition, "vs0:", strlen("vs0:")))
		return VS0_ID;
	if (!memcmp(partition, "blank0:", strlen("blank0:")))
		return BLANK0_ID;
	return 0;
}

int getPartitionForMountPointId(int mount_point_id, char** partition) {
	if (mount_point_id == TM0_ID) {
		*partition = TM0_DEV;
		return 1;
	} else if (mount_point_id == UX0_ID) {
		*partition = UX0_DEV;
		return 1;
	} else if (mount_point_id == UX0_ID) {
		*partition = UX0_DEV;
		return 1;
	} else if (mount_point_id == UMA0_ID) {
		*partition = UMA0_DEV;
		return 1;
	} else if (mount_point_id == IMC0_ID) {
		*partition = IMC0_DEV;
		return 1;
	} else if (mount_point_id == XMC0_ID) {
		*partition = XMC0_DEV;
		return 1;
	} else if (mount_point_id == SD0_ID) {
		*partition = SD0_DEV;
		return 1;
	} else if (mount_point_id == OS0_ID) {
		*partition = OS0_DEV;
		return 1;
	} else if (mount_point_id == UD0_ID) {
		*partition = UD0_DEV;
		return 1;
	} else if (mount_point_id == SA0_ID) {
		*partition = SA0_DEV;
		return 1;
	} else if (mount_point_id == PD0_ID) {
		*partition = PD0_DEV;
		return 1;
	} else if (mount_point_id == VD0_ID) {
		*partition = VD0_DEV;
		return 1;
	} else if (mount_point_id == VS0_ID) {
		*partition = VS0_DEV;
		return 1;
	} else if (mount_point_id == BLANK0_ID) {
		*partition = BLANK0_DEV;
		return 1;
	}
	return 0;
}

int getBlkdevForDevice(const char* device, char** blkdev, char** blkdev2) {
	if (!memcmp(device, "UMA", strlen("UMA"))) {
		*blkdev = UMA_BLKDEV;
		*blkdev2 = UMA_BLKDEV2;
		return 1;
	} else if (!memcmp(device, "GCD", strlen("GCD"))) {
		*blkdev = GCD_BLKDEV;
		*blkdev2 = GCD_BLKDEV2;
		return 1;
	} else if (!memcmp(device, "MCD", strlen("MCD"))) {
		*blkdev = MCD_BLKDEV;
		*blkdev2 = MCD_BLKDEV2;
		return 1;
	} else if (!memcmp(device, "INT", strlen("INT"))) {
		*blkdev = INT_BLKDEV;
		*blkdev2 = INT_BLKDEV2;
		return 1;
	}
	return 0;
}

int shellKernelGetCurrentBlkdevForMountPointId(int mount_point_id, char** blkdev, char** blkdev2) {
	LOG("Reading current device blkdev for mount point 0x%03X :\n", mount_point_id);
	SceIoMountPoint *mount = sceIoFindMountPoint(mount_point_id);
	if (!mount) return -1;
	LOG("%s\n", mount->dev->blkdev);
	if (mount->dev->blkdev != NULL)
		*blkdev = mount->dev->blkdev;
	if (mount->dev->blkdev2 != NULL)
		*blkdev2 = mount->dev->blkdev2;
	return 0;
}

int shellKernelGetOriginalBlkdevForMountPointId(int mount_point_id, char** blkdev, char** blkdev2) {
	if (mount_point_id == UX0_ID) {
		if (ux0_ori_dev->blkdev != NULL)
			*blkdev = ux0_ori_dev->blkdev;
		if (ux0_ori_dev->blkdev2 != NULL)
			*blkdev2 = ux0_ori_dev->blkdev2;
		return 1;
	}
	if (mount_point_id == UMA0_ID) {
		if (uma0_ori_dev->blkdev != NULL)
			*blkdev = uma0_ori_dev->blkdev;
		if (uma0_ori_dev->blkdev2 != NULL)
			*blkdev2 = uma0_ori_dev->blkdev2;
		return 1;
	}
	if (mount_point_id == IMC0_ID) {
		if (imc0_ori_dev->blkdev != NULL)
			*blkdev = imc0_ori_dev->blkdev;
		if (imc0_ori_dev->blkdev2 != NULL)
			*blkdev2 = imc0_ori_dev->blkdev2;
		return 1;
	}
	return 0;
}

int shellKernelIsPartitionRedirected(const char* partition, char** blkdev, char** blkdev2) {
	if (!isPartitionValid(partition))
		return -1;
	int mount_point_id = getMountPointIdForPartition(partition);
	LOG("mount point id : 0x%04X\n", mount_point_id);
	if (!shellKernelGetCurrentBlkdevForMountPointId(mount_point_id, blkdev, blkdev2)) {
		LOG("current blkdev : %s %s\n", *blkdev, *blkdev2);
		SceIoMountPoint *mount = sceIoFindMountPoint(mount_point_id);
		if (!mount) return -1;
		if (!memcmp(partition, UX0_DEV, strlen(UX0_DEV))) {
			if (mount->dev != ux0_ori_dev || mount->dev2 != ux0_ori_dev2)
				return 1;
			else return 0;
		} else if (!memcmp(partition, UMA0_DEV, strlen(UMA0_DEV))) {
			if (mount->dev != uma0_ori_dev || mount->dev2 != uma0_ori_dev2)
				return 1;
			else return 0;
		} else if (!memcmp(partition, GRO0_DEV, strlen(GRO0_DEV))) {
			if (mount->dev != gro0_ori_dev || mount->dev2 != gro0_ori_dev2)
				return 1;
			else return 0;
		} else if (!memcmp(partition, GRW0_DEV, strlen(GRW0_DEV))) {
			if (mount->dev != grw0_ori_dev || mount->dev2 != grw0_ori_dev2)
				return 1;
			else return 0;
		} else if (!memcmp(partition, IMC0_DEV, strlen(IMC0_DEV))) {
			if (mount->dev != imc0_ori_dev || mount->dev2 != imc0_ori_dev2)
				return 1;
			else return 0;
		} else if (!memcmp(partition, XMC0_DEV, strlen(XMC0_DEV))) {
			if (mount->dev != xmc0_ori_dev || mount->dev2 != xmc0_ori_dev2)
				return 1;
			else return 0;
		} else return -1;
	} else return -1;
	return 0;
}

int mountPointIdList[4];
int* isDeviceMounted(const char* blkdev, const char* blkdev2) {
	char* blkdev_local = NULL;
	char* blkdev2_local = NULL;
	int ret = -1;
	int j = 0;
	for (int i=0x100; i<=0xF00; i+=0x100) {
		ret = shellKernelGetCurrentBlkdevForMountPointId(i, &blkdev_local, &blkdev2_local);
		if (ret != -1) {
			LOG("found : %s\n", blkdev_local);
			if (!memcmp(blkdev_local, blkdev, strlen(blkdev))) {
				mountPointIdList[j] = i;
				//memcpy(((uintptr_t)mountPointIdList)+j*sizeof(int), i, sizeof(int));
				j++;
				LOG("working\n");
			}
		}
	}
	return mountPointIdList;
}

int shellKernelUnredirect(const char* partition, int isReady) {
	LOG("Unredirecting %s\n", partition);
	char* device = NULL;
	char* device2 = NULL;
	if (!isPartitionValid(partition))
		return -1;
	int mount_point_id = getMountPointIdForPartition(partition);
	SceIoMountPoint *mount = sceIoFindMountPoint(mount_point_id);
	if (!mount) return -1;
	LOG("Is ux0: redirected : %08X\n", shellKernelIsPartitionRedirected(partition, &device, &device2));
	LOG("ux0: current device : %s\noriginal device %s\n", device, ux0_ori_dev->blkdev);
	if (shellKernelIsPartitionRedirected(partition, &device, &device2) == 1) { 
		if (!isReady) {
			char* blkdev = NULL;
			char* blkdev2 = NULL;
			shellKernelGetOriginalBlkdevForMountPointId(mount_point_id, &blkdev, &blkdev2);
			int* mount_point_id_s_list = isDeviceMounted(blkdev, blkdev2);
			char *partition_s = NULL;
			for (int i=0; mount_point_id_s_list[i] != 0; i++) {
				LOG("%s is currently mounted on : %04X\n", blkdev, mount_point_id_s_list[i]);
				getPartitionForMountPointId(mount_point_id_s_list[i], &partition_s);
				shellKernelUnredirect(partition_s, 1);
			}
		}
		ux0_prev_dev = mount->dev;
		ux0_prev_dev2 = mount->dev2;
		mount->dev = ux0_ori_dev;
		mount->dev2 = ux0_ori_dev2;
		io_remount(mount_point_id);
		return 1;
	} else {
		LOG("%s is not currently redirected. Aborting unredirection.\n", partition);
		return 0;
	}
	return 0;
}

static SceIoDevice new_dev;
static SceIoDevice ux0_new_dev;
static SceIoDevice gro0_new_dev;
static SceIoDevice grw0_new_dev;
static SceIoDevice imc0_new_dev;
static SceIoDevice xmc0_new_dev;
static SceIoDevice uma0_new_dev;
int shellKernelRedirect(const char* partition, const char* device) {
	if (!isPartitionValid(partition))
		return -1;
	int mount_point_id = getMountPointIdForPartition(partition);
	if (!isDeviceValid(device))
		return -1;
	SceIoMountPoint *mount = sceIoFindMountPoint(mount_point_id);
	if (!mount) return -1;
	if (!memcmp(device, "GCD", strlen("GCD"))) {
		new_dev.blkdev = GCD_BLKDEV;
		new_dev.blkdev2 = GCD_BLKDEV2;
	} else if (!memcmp(device, "UMA", strlen("UMA"))) {
		new_dev.blkdev = UMA_BLKDEV;
		new_dev.blkdev2 = UMA_BLKDEV2;
	} else if (!memcmp(device, "INT", strlen("INT"))) {
		new_dev.blkdev = INT_BLKDEV;
		new_dev.blkdev2 = INT_BLKDEV2;
	} else if (!memcmp(device, "MCD", strlen("MCD"))) {
		new_dev.blkdev = MCD_BLKDEV;
		new_dev.blkdev2 = MCD_BLKDEV2;
	} else return -1;
	if (mount_point_id == UX0_ID) {
		ux0_new_dev.blkdev = new_dev.blkdev;
		ux0_new_dev.blkdev2 = new_dev.blkdev2;
		ux0_new_dev.dev2 = UX0_DEV2;
		ux0_new_dev.dev = UX0_DEV;
		ux0_new_dev.dev2 = UX0_DEV2;
		ux0_new_dev.id = UX0_ID;
		ux0_prev_dev = mount->dev;
		ux0_prev_dev2 = mount->dev2;
		mount->dev = &ux0_new_dev;
		mount->dev2 = &ux0_new_dev;
	} else if (mount_point_id == GRO0_ID) {
		gro0_new_dev.blkdev = new_dev.blkdev;
		gro0_new_dev.blkdev2 = new_dev.blkdev2;
		gro0_new_dev.dev = GRO0_DEV;
		gro0_new_dev.dev2 = GRO0_DEV2;
		gro0_new_dev.id = GRO0_ID;
		gro0_prev_dev = mount->dev;
		gro0_prev_dev2 = mount->dev2;
		mount->dev = &gro0_new_dev;
		mount->dev2 = &gro0_new_dev;
	} else if (mount_point_id == GRW0_ID) {
		grw0_new_dev.blkdev = new_dev.blkdev;
		grw0_new_dev.blkdev2 = new_dev.blkdev2;
		grw0_new_dev.dev = GRW0_DEV;
		grw0_new_dev.dev2 = GRW0_DEV2;
		grw0_new_dev.id = GRW0_ID;
		grw0_prev_dev = mount->dev;
		grw0_prev_dev2 = mount->dev2;
		mount->dev = &grw0_new_dev;
		mount->dev2 = &grw0_new_dev;
	} else if (mount_point_id == IMC0_ID) {
		imc0_new_dev.blkdev = new_dev.blkdev;
		imc0_new_dev.blkdev2 = new_dev.blkdev2;
		imc0_new_dev.dev = IMC0_DEV;
		imc0_new_dev.dev2 = IMC0_DEV2;
		imc0_new_dev.id = IMC0_ID;
		imc0_prev_dev = mount->dev;
		imc0_prev_dev2 = mount->dev2;
		mount->dev = &imc0_new_dev;
		mount->dev2 = &imc0_new_dev;
	} else if (mount_point_id == XMC0_ID) {
		xmc0_new_dev.blkdev = new_dev.blkdev;
		xmc0_new_dev.blkdev2 = new_dev.blkdev2;
		xmc0_new_dev.dev = XMC0_DEV;
		xmc0_new_dev.dev2 = XMC0_DEV2;
		xmc0_new_dev.id = XMC0_ID;
		xmc0_prev_dev = mount->dev;
		xmc0_prev_dev2 = mount->dev2;
		mount->dev = &xmc0_new_dev;
		mount->dev2 = &xmc0_new_dev;
	} else if (mount_point_id == UMA0_ID) {
		uma0_new_dev.blkdev = new_dev.blkdev;
		uma0_new_dev.blkdev2 = new_dev.blkdev2;
		uma0_new_dev.dev = UMA0_DEV;
		uma0_new_dev.dev2 = UMA0_DEV2;
		uma0_new_dev.id = UMA0_ID;
		uma0_prev_dev = mount->dev;
		uma0_prev_dev2 = mount->dev2;
		mount->dev = &uma0_new_dev;
		mount->dev2 = &uma0_new_dev;
	} else return -1;
	LOG("mount is now : %s\n", mount->dev->blkdev);
	io_remount(mount_point_id);
	return 0;
}

int saveOriginalDevicesForMountPoints() {
	SceIoMountPoint *mount = sceIoFindMountPoint(TM0_ID);
	if (!mount) LOG("No tm0: mount point");
	else {
		tm0_ori_dev = mount->dev;
		tm0_ori_dev2 = mount->dev2;
		LOG("tm0 : %s\n%s\n%s\n%s\n%08X\n\n", tm0_ori_dev->dev, tm0_ori_dev->dev2, tm0_ori_dev->blkdev, tm0_ori_dev->blkdev2, tm0_ori_dev->id);
	}
	mount = sceIoFindMountPoint(UR0_ID);
	if (!mount) LOG("No ur0: mount point");
	else {
		ur0_ori_dev = mount->dev;
		ur0_ori_dev2 = mount->dev2;
		LOG("ur0 : %s\n%s\n%s\n%s\n%08X\n\n", ur0_ori_dev->dev, ur0_ori_dev->dev2, ur0_ori_dev->blkdev, ur0_ori_dev->blkdev2, ur0_ori_dev->id);
	}
	mount = sceIoFindMountPoint(UX0_ID);
	if (!mount) LOG("No ux0: mount point");
	else {
		ux0_ori_dev = mount->dev;
		ux0_ori_dev2 = mount->dev2;
		LOG("ux0 : %s\n%s\n%s\n%s\n%08X\n\n", ux0_ori_dev->dev, ux0_ori_dev->dev2, ux0_ori_dev->blkdev, ux0_ori_dev->blkdev2, ux0_ori_dev->id);
	}
	mount = sceIoFindMountPoint(GRO0_ID);
	if (!mount) LOG("No gro0: mount point");
	else {
		gro0_ori_dev = mount->dev;
		gro0_ori_dev2 = mount->dev2;
		LOG("gro0 : %s\n%s\n%s\n%s\n%08X\n\n", gro0_ori_dev->dev, gro0_ori_dev->dev2, gro0_ori_dev->blkdev, gro0_ori_dev->blkdev2, gro0_ori_dev->id);
	}
	mount = sceIoFindMountPoint(GRW0_ID);
	if (!mount) LOG("No grw0: mount point");
	else {
		grw0_ori_dev = mount->dev;
		grw0_ori_dev2 = mount->dev2;
		LOG("grw0 : %s\n%s\n%s\n%s\n%08X\n\n", grw0_ori_dev->dev, grw0_ori_dev->dev2, grw0_ori_dev->blkdev, grw0_ori_dev->blkdev2, grw0_ori_dev->id);
	}
	mount = sceIoFindMountPoint(IMC0_ID);
	if (!mount) LOG("No imc0: mount point");
	else {
		imc0_ori_dev = mount->dev;
		imc0_ori_dev2 = mount->dev2;
		LOG("imc0 : %s\n%s\n%s\n%s\n%08X\n\n", imc0_ori_dev->dev, imc0_ori_dev->dev2, imc0_ori_dev->blkdev, imc0_ori_dev->blkdev2, imc0_ori_dev->id);
	}
	mount = sceIoFindMountPoint(XMC0_ID);
	if (!mount) LOG("No gro0: mount point");
	else {
		xmc0_ori_dev = mount->dev;
		xmc0_ori_dev2 = mount->dev2;
		LOG("xmc0 : %s\n%s\n%s\n%s\n%08X\n\n", xmc0_ori_dev->dev, xmc0_ori_dev->dev2, xmc0_ori_dev->blkdev, xmc0_ori_dev->blkdev2, xmc0_ori_dev->id);
	}
	mount = sceIoFindMountPoint(UMA0_ID);
	if (!mount) LOG("No uma0: mount point");
	else {
		uma0_ori_dev = mount->dev;
		uma0_ori_dev2 = mount->dev2;
		LOG("uma0 : %s\n%s\n%s\n%s\n%08X\n\n", uma0_ori_dev->dev, uma0_ori_dev->dev2, uma0_ori_dev->blkdev, uma0_ori_dev->blkdev2, uma0_ori_dev->id);
	}
	return 0;
}

// allow microSD cards, patch by motoharu
int GCD_patch_scesdstor() {
	tai_module_info_t scesdstor_info;
	scesdstor_info.size = sizeof(tai_module_info_t);
	if (taiGetModuleInfoForKernel(KERNEL_PID, "SceSdstor", &scesdstor_info) >= 0) {
		// patch for proc_initialize_generic_2 - so that SD card type is not ignored
		char zeroCallOnePatch[4] = {0x01, 0x20, 0x00, 0xBF};
		int gen_init_2_patch_uid = taiInjectDataForKernel(KERNEL_PID, scesdstor_info.modid, 0, 0x2498, zeroCallOnePatch, 4); // patch (BLX) to (MOVS R0, #1 ; NOP)
	} else return -1;
	return 0;
}

int GCD_poke() {
	tai_module_info_t scesdstor_info;
	scesdstor_info.size = sizeof(tai_module_info_t);
	if (taiGetModuleInfoForKernel(KERNEL_PID, "SceSdstor", &scesdstor_info) < 0)
		return -1;

	void *args = 0;
	int (*SceSdstorCardInsert)() = 0;
	int (*SceSdstorCardRemove)() = 0;

	module_get_offset(KERNEL_PID, scesdstor_info.modid, 0, 0x3BD5, (uintptr_t *)&SceSdstorCardInsert);
	module_get_offset(KERNEL_PID, scesdstor_info.modid, 0, 0x3BC9, (uintptr_t *)&SceSdstorCardRemove);
	module_get_offset(KERNEL_PID, scesdstor_info.modid, 1, 0x1B20 + 40 * 1, (uintptr_t *)&args);

	SceSdstorCardRemove(0, args);
	ksceKernelDelayThread(200 * 1000);
	SceSdstorCardInsert(0, args);
	ksceKernelDelayThread(200 * 1000);

	return 0;
}

int suspend_workaround_callback(int resume, int eventid, void *args, void *opt) {
	if (eventid != 0x100000)
		return 0;
	LOG("suspend_workaround_callback.\n");
	if (!UMAuma0) {
		ksceKernelDelayThread(10 * 1000); // This delay is needed else the remount fails.
		LOG("Remounting uma0: %08X.\n", ksceIoMount(UMA0_ID, NULL, 0, 0, 0, 0));
	} else {
		// wait ~5 seconds max for USB mass to be detected
		// this may look bad but the PSVita does this to detect ux0: so ¯\_(ツ)_/¯
		for (int i=0; i <= 25; i++) {
			LOG("Remounting uma0: %08X.\n", ksceIoMount(UMA0_ID, NULL, 0, 0, 0, 0));
			LOG("USB mass detection...\n");
			if (exists("uma0:")) {// try to detect uma0: 25 times for 0.2s each
				LOG("USB mass detected.\n");
				break;
			} else ksceKernelDelayThread(200 * 1000);
		}
	}
	return 0;
}

int suspend_workaround_callback_id = -1;
int suspend_workaround(void) {
	suspend_workaround_callback_id = ksceKernelRegisterSysEventHandler("suspend_workaround_callback", suspend_workaround_callback, NULL);
	LOG("suspend_workaround_callback_id : %08X\n", suspend_workaround_callback_id);
	return 0;
}

int GCD_suspend_callback(int resume, int eventid, void *args, void *opt) {
	if (eventid != 0x100000)
		return 0;
	GCD_poke();
	return 0;
}

int GCD_suspend_callback_id = -1;
int GCD_register_callback() {
	GCD_suspend_callback_id = ksceKernelRegisterSysEventHandler("GCD_suspend_callback", GCD_suspend_callback, NULL);
	LOG("GCD_suspend_callback_id : %08X\n", GCD_suspend_callback_id);
	return 0;
}

int GCD_workaround(void) {
	if (GCD_patch_scesdstor() != 0)
		return -1;
	GCD_poke();
	GCD_register_callback();
	return 0;
}

static SceUID ksceSysrootIsSafeMode_hookid = -1;
static tai_hook_ref_t ksceSysrootIsSafeMode_hookref;
static int ksceSysrootIsSafeMode_patched() {
	return 1;
}

int UMA_workaround(void) {
	int (* _ksceKernelMountBootfs)(const char *bootImagePath);
	int (* _ksceKernelUmountBootfs)(void);
	int ret;
	
	// Fake SAFE mode in SceUsbServ
	ksceSysrootIsSafeMode_hookid = taiHookFunctionImportForKernel(KERNEL_PID, &ksceSysrootIsSafeMode_hookref, "SceUsbServ", 0x2ED7F97A, 0x834439A7, ksceSysrootIsSafeMode_patched);
	
	ret = module_get_export_func(KERNEL_PID, "SceKernelModulemgr", 0xC445FA63, 0x01360661, (uintptr_t *)&_ksceKernelMountBootfs);
	if (ret < 0)
		ret = module_get_export_func(KERNEL_PID, "SceKernelModulemgr", 0x92C9FFC2, 0x185FF1BC, (uintptr_t *)&_ksceKernelMountBootfs);
	if (ret < 0)
		return SCE_KERNEL_START_NO_RESIDENT;

	ret = module_get_export_func(KERNEL_PID, "SceKernelModulemgr", 0xC445FA63, 0x9C838A6B, (uintptr_t *)&_ksceKernelUmountBootfs);
	if (ret < 0)
		ret = module_get_export_func(KERNEL_PID, "SceKernelModulemgr", 0x92C9FFC2, 0xBD61AD4D, (uintptr_t *)&_ksceKernelUmountBootfs);
	if (ret < 0)
		return SCE_KERNEL_START_NO_RESIDENT;
	
	// Load SceUsbMass kernel module
	SceUID sceusbmass_modid;
	LOG("Loading SceUsbMass from os0:.\n");
	if (_ksceKernelMountBootfs("os0:kd/bootimage.skprx") >= 0) {
		sceusbmass_modid = ksceKernelLoadModule("os0:kd/umass.skprx", 0x800, NULL);
		LOG("Unmounting bootfs: : %i.\n", _ksceKernelUmountBootfs());
	} else {
		LOG("Error mounting bootfs\n");
		return -1;
	}
	LOG("SceUsbMass module id : %08X.\n", (int)sceusbmass_modid);
	
	// Hook module_start
	// TODO (not necessary) : add support to taiHEN so we don't need to hard code this address
	SceUID tmp1, tmp2;
	const char check_patch[] = {0x01, 0x20, 0x01, 0x20};
	tmp1 = taiInjectDataForKernel(KERNEL_PID, sceusbmass_modid, 0, 0x1546, check_patch, sizeof(check_patch));
	tmp2 = taiInjectDataForKernel(KERNEL_PID, sceusbmass_modid, 0, 0x154c, check_patch, sizeof(check_patch));
	if (sceusbmass_modid >= 0) ret = ksceKernelStartModule(sceusbmass_modid, 0, NULL, 0, NULL, NULL); 
	else ret = sceusbmass_modid;
	if (tmp1 >= 0) taiInjectReleaseForKernel(tmp1);
	if (tmp2 >= 0) taiInjectReleaseForKernel(tmp2);

	if (ret < 0)// Check result
		return SCE_KERNEL_START_NO_RESIDENT;

	return 0;
}

int isEnsoLaunched(void) {
	if (getFileSize("ur0:tai/boot_config.txt") <= 0 && getFileSize("vs0:tai/boot_config.txt") <= 0) {
		LOG("No enso bootconfig file found.\n");
		return 0;
	} else {
		LOG("Enso bootconfig file found.\n");
		if (ksceSblACMgrIsShell()) {
			LOG("SceShell is loaded.\n");
			return 0;
		} else
			LOG("SceShell is not loaded.\n");
	}
	return 1;
}


void _start() __attribute__ ((weak, alias("module_start")));
int module_start(SceSize args, void *argp) {
	
	ksceIoRemove(log_ur0_path);
	LOG("StorageMgrKernel started.\n");
	
	int ensoLaunched = isEnsoLaunched();
	if (ensoLaunched)
		LOG("Enso is launched.\n");
	else
		LOG("Enso is not launched.\n");
	
	// Get SceIofilemgr module info
	tai_module_info_t sceiofilemgr_modinfo;
	sceiofilemgr_modinfo.size = sizeof(tai_module_info_t);
	if (taiGetModuleInfoForKernel(KERNEL_PID, "SceIofilemgr", &sceiofilemgr_modinfo) < 0)
		return -1;
	
	// Get important function
	switch (sceiofilemgr_modinfo.module_nid) {
		case 0x9642948C: // 3.60 retail
			module_get_offset(KERNEL_PID, sceiofilemgr_modinfo.modid, 0, 0x138C1, (uintptr_t *)&sceIoFindMountPoint);
			break;
		case 0xA96ACE9D: // 3.65 retail
		case 0x3347A95F: // 3.67 retail
			module_get_offset(KERNEL_PID, sceiofilemgr_modinfo.modid, 0, 0x182F5, (uintptr_t *)&sceIoFindMountPoint);
			break;
		default:
			return -1;
	}
	
	UMAuma0 = 0;
	suspend_workaround(); // To keep uma0: mounted after PSVita exit suspend mode
	
	saveOriginalDevicesForMountPoints();
	
	char *device = NULL;
	char *device2 = NULL;
	LOG("Is ux0: redirected : %i\n", shellKernelIsPartitionRedirected(UX0_DEV, &device, &device2));
	LOG("ux0: current device : %s %s\n", device, device2);
	
	if (checkConfigLineReturnChar(default_config_path) != 0)
		return -1;
	int entriesNumber = config_read(default_config_path);
	LOG("Config entries : %i.\n", entriesNumber);
	if (entriesNumber > 0) {
		LOG("Checking if UMA is in config.\n");
		int UMAline = isDeviceInConfig("UMA");
		LOG("Checking if GCD is in config.\n");
		int GCDline = isDeviceInConfig("GCD");
		LOG("Checking if MCD is in config.\n");
		int MCDline = isDeviceInConfig("MCD");
		LOG("Checking if INT is in config.\n");
		int INTline = isDeviceInConfig("INT");
		if (UMAline != -1) {
			LOG("UMA config found at line %i.\n", UMAline);
			UMA_workaround();
			// wait ~5 seconds max for USB mass to be detected
			// this may look bad but the PSVita does this to detect ux0: so ¯\_(ツ)_/¯
			for (int i = 0; i <= 25; i++) { // try to detect USB mass 25 times for 0.2s each
				LOG("USB mass detection...\n");
				if (exists(UMA_BLKDEV) || exists(UMA_BLKDEV2) || !ensoLaunched) {
					LOG("USB mass detected.\n");
					char UMAmountPoint[16];
					if (readMountPointByLine(UMAline, UMAmountPoint) == 0) {
						if (memcmp(UMAmountPoint, "uma0", strlen("uma0")) == 0) {
							ksceIoMount(UMA0_ID, NULL, 0, 0, 0, 0);
							UMAuma0 = 1;
							break;
						} else if (memcmp(UMAmountPoint, "ux0", strlen("ux0")) == 0) {
							if (shellKernelRedirect(UX0_DEV, "UMA") == -1) {
								LOG("No ux0: mount point.\n");
								return SCE_KERNEL_START_FAILED;
							}
							break;
						} else if (memcmp(UMAmountPoint, "grw0", strlen("grw0")) == 0) {
							if (shellKernelRedirect(GRW0_DEV, "UMA") == -1) {
								LOG("No grw0: mount point.\n");
								return SCE_KERNEL_START_FAILED;
							}
							break;
						} else if (memcmp(UMAmountPoint, "imc0", strlen("imc0")) == 0) {
							if (shellKernelRedirect(IMC0_DEV, "UMA") == -1) {
								LOG("No imc0: mount point.\n");
								return SCE_KERNEL_START_FAILED;
							}
							break;
						}
					}
				} else ksceKernelDelayThread(200000);
			}
			if (!exists(UMA_BLKDEV) && !exists(UMA_BLKDEV2))
				LOG("USB mass still not detected. Aborting USB mass detection.\n");
		} else LOG("No UMA config found.\n");
		if (GCDline != -1) {
			LOG("GCD config found at line %i.\n", GCDline);
			GCD_workaround();
			LOG("GC2SD detection...\n");
			if (exists(GCD_BLKDEV))
				LOG("GC2SD detected.\n");
			else
				LOG("GC2SD not detected.\n");
			char GCDmountPoint[16];
			if (readMountPointByLine(GCDline, GCDmountPoint) == 0) {
				if (memcmp(GCDmountPoint, "ux0", strlen("ux0")) == 0 && (exists(GCD_BLKDEV) || !ensoLaunched)) {
					if (shellKernelRedirect(UX0_DEV, "GCD") == -1) {
						LOG("No ux0: mount point.\n");
						return SCE_KERNEL_START_FAILED;
					}
				} else if (memcmp(GCDmountPoint, "grw0", strlen("grw0")) == 0) {
					if (shellKernelRedirect(GRW0_DEV, "GCD") == -1) {
						LOG("No grw0: mount point.\n");
						return SCE_KERNEL_START_FAILED;
					}
				} else if (memcmp(GCDmountPoint, "uma0", strlen("uma0")) == 0) {
					if (shellKernelRedirect(UMA0_DEV, "GCD") == -1) {
						LOG("No uma0: mount point.\n");
						return SCE_KERNEL_START_FAILED;
					}
				} else if (memcmp(GCDmountPoint, "imc0", strlen("imc0")) == 0) {
					if (shellKernelRedirect(IMC0_DEV, "GCD") == -1) {
						LOG("No imc0: mount point.\n");
						return SCE_KERNEL_START_FAILED;
					}
				}
			}
		} else LOG("No GCD config found.\n");
		if (INTline != -1) {
			LOG("INT config found at line %i.\n", INTline);
			LOG("Internal storage detection...\n");
			if (exists(INT_BLKDEV))// {
				LOG("Internal storage detected.\n");
				char INTmountPoint[16];
				if (readMountPointByLine(INTline, INTmountPoint) == 0) {
					if (memcmp(INTmountPoint, "imc0", strlen("imc0")) == 0) {
						ksceIoMount(IMC0_ID, NULL, 0, 0, 0, 0);
					} else if (memcmp(INTmountPoint, "ux0", strlen("ux0")) == 0) {
						if (shellKernelRedirect(UX0_DEV, "INT") == -1) {
							LOG("No ux0: mount point.\n");
							return SCE_KERNEL_START_FAILED;
						}
					} else if (memcmp(INTmountPoint, "grw0", strlen("grw0")) == 0) {
						if (shellKernelRedirect(GRW0_DEV, "INT") == -1) {
							LOG("No grw0: mount point.\n");
							return SCE_KERNEL_START_FAILED;
						}
					} else if (memcmp(INTmountPoint, "uma0", strlen("uma0")) == 0) {
						if (shellKernelRedirect(UMA0_DEV, "INT") == -1) {
							LOG("No uma0: mount point.\n");
							return SCE_KERNEL_START_FAILED;
						}
					}
				}
			//} else LOG("Internal storage not detected.\n");
		} else LOG("No INT config found.\n");
		if (MCDline != -1) {
			LOG("MCD config found at line %i.\n", MCDline);
			LOG("MCD detection...\n");
			if (exists(MCD_BLKDEV))
				LOG("MCD detected.\n");
			else
				LOG("MCD not detected.\n");
			if (exists(MCD_BLKDEV) || !ensoLaunched) {
				char MCDmountPoint[16];
				if (readMountPointByLine(MCDline, MCDmountPoint) == 0) {
					if (memcmp(MCDmountPoint, "xmc0", strlen("xmc0")) == 0) {
						ksceIoMount(XMC0_ID, NULL, 0, 0, 0, 0);
					} else if (memcmp(MCDmountPoint, "imc0", strlen("imc0")) == 0) {
						if (shellKernelRedirect(IMC0_DEV, "MCD") == -1) {
							LOG("No imc0: mount point.\n");
							return SCE_KERNEL_START_FAILED;
						}
					} else if (memcmp(MCDmountPoint, "ux0", strlen("ux0")) == 0) {
						if (shellKernelRedirect(UX0_DEV, "MCD") == -1) {
							LOG("No ux0: mount point.\n");
							return SCE_KERNEL_START_FAILED;
						}
					} else if (memcmp(MCDmountPoint, "grw0", strlen("grw0")) == 0) {
						if (shellKernelRedirect(GRW0_DEV, "MCD") == -1) {
							LOG("No grw0: mount point.\n");
							return SCE_KERNEL_START_FAILED;
						}
					} else if (memcmp(MCDmountPoint, "uma0", strlen("uma0")) == 0) {
						if (shellKernelRedirect(UMA0_DEV, "MCD") == -1) {
							LOG("No uma0: mount point.\n");
							return SCE_KERNEL_START_FAILED;
						}
					}
				}
			}
		} else LOG("No MCD config found.\n");
	} else LOG("No device config found.\n");

	LOG("Is ux0: redirected : %i\n", shellKernelIsPartitionRedirected(UX0_DEV, &device, &device2));
	LOG("ux0: current device : %s %s\n", device, device2);
	LOG("Is uma0: redirected : %i\n", shellKernelIsPartitionRedirected(GRW0_DEV, &device, &device2));
	LOG("grw0: current device : %s %s\n", device, device2);
	
	if (!ensoLaunched && shellKernelIsPartitionRedirected(UX0_DEV, &device, &device2))
		kscePowerRequestSoftReset(); // this way we can exit HENkaku bootstrap.self
	
	LOG("StorageMgrKernel finished with success.\n");
	return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize args, void *argp) {
	if (ksceSysrootIsSafeMode_hookid >= 0) taiHookReleaseForKernel(ksceSysrootIsSafeMode_hookid, ksceSysrootIsSafeMode_hookref);
	LOG("ksceSysrootIsSafeMode hook released.\n");
	
	if (GCD_suspend_callback_id != -1) LOG("GCD unreg : %08X\n", ksceKernelUnregisterSysEventHandler(GCD_suspend_callback_id));
	
	if (suspend_workaround_callback_id != -1) LOG("workaround unreg : %08X\n", ksceKernelUnregisterSysEventHandler(suspend_workaround_callback_id));
	
	LOG("StorageMgrKernel stopped with success.\n");
	return SCE_KERNEL_STOP_SUCCESS;
}

void log_write(const char *buffer, size_t length, const char *folderpath, const char *fullpath) {
	extern int ksceIoMkdir(const char *, int);
	ksceIoMkdir(folderpath, 6);
	SceUID fd = ksceIoOpen(fullpath, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_APPEND, 6);
	if (fd < 0)
		return;
	ksceIoWrite(fd, buffer, length);
	ksceIoClose(fd);
}
