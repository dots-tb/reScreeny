// reScreeny by dots_tb - screenshots into game named folders
// Structure cleanup by Princess of Sleeping
//
// With help from folks at the CBPS: https://discord.gg/2nDCbxJ
// Idea by cuevavirus
//
// Testing team:
// 	cuevavirus
//	Nkekev
// 	Yoti

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <taihen.h>
#include <stdlib.h>
#include <sys/syslimits.h>

#include <vitasdk.h>

#define printf sceClibPrintf
#define HOOKS_NUM 1

tai_hook_ref_t hook_ref[HOOKS_NUM];
static int hook_uid[HOOKS_NUM];

typedef struct SceAVImeParam2 {
	char *outpath;		// ex:photo0:/SCREENSHOT/kh/2019-11-14-195233.bmp
	uint32_t path_len;	// strlen(outpath)
	uint32_t unk_0x08[2];	// ex:0x0, 0xFFFFFFFF
	uint32_t type;		// off:0x10, ex:2
	uint32_t padding2;

	char *img_ext;		// ex:".bmp"
	uint32_t img_ext_len;
	uint32_t padding3;	// off:0x20
	char *filename;		// ex:"2019-11-14-190417"
	uint32_t fn_len;
	uint32_t padding4;
	char *titlename;	// off:0x30
	uint32_t title_len;

	void *unk_0x38;
	// more...?
} ImgParam2;

typedef struct ImgParam3 {	// size is 0x80?
	void *jpg_buffer; 
	uint32_t buffer_size;	// ex:0x4000
	uint32_t padding08;
	uint32_t padding0C;

	uint32_t unk_0x10;	// ex:0x17E836
	uint32_t padding14;
	uint32_t padding18;
	uint32_t type;

	char *img_ext;		// off:0x20
	uint32_t img_ext_len;
	uint32_t padding28;
	char *filename;

	uint32_t fn_len;	// off:0x30
	uint32_t padding34;
	char *titlename;
	uint32_t title_len;

	uint32_t padding50[4];	// off:0x40

	char *temp_location;	// off:0x50
	uint32_t tmp_loc_len;
	uint32_t padding58;
	uint32_t padding5C;

	void *ptr_0x60;		// ex:"2019-11-15-113750"
	void *ptr_0x64;		// path???
	uint32_t padding68;
	uint32_t unk_0x6C;	// ex:2

	void *ptr_0x70;		// path???
	void *ptr_0x74;
	char *temp_location2;	// ex:"ur0:temp/screenshot/capture.bmp"
	void *ptr_0x7C;
} ImgParam3;


void sanitize(char *in, int len) {
	char il_chars[] = "<>:/\"\\|?*\n";
	
	char *ptr = in;
	while(ptr < in + len) {
		int i = 0;
		while(il_chars[i]!=0) {
			if(*ptr==il_chars[i]) {
				*ptr = ' ';
				break;
			}
			i++;
		}
		ptr++;
	}
}

int hook_func1(int r1, ImgParam2 *param2, ImgParam3 *param3) {
	
	int ret;
	if(param2->type == 2) {
		SceDateTime time;
		sceRtcGetCurrentClockLocalTime(&time);
		
		char fn[30];
		sce_paf_private_snprintf(fn, 30, "%04d-%02d-%02d-%02d%02d%02d-%06d",  time.year, time.month, time.day, time.hour, time.minute, time.second, time.microsecond);
		
		/*if(!param2->img_ext_len) {
			param2->img_ext_len = param3->img_ext_len;
			param2->img_ext = sce_paf_private_malloc(param2->img_ext_len + 1);
			sce_paf_private_memset(param2->img_ext, 0, param2->img_ext_len  + 1);
			sce_paf_private_memcpy(param2->img_ext, param3->img_ext, param2->img_ext_len);
		}*/

		param2->outpath = sce_paf_private_malloc(PATH_MAX);		
	
		char *titlename = (param2->title_len != 0) ? param2->titlename : "Other";
		
		sanitize(titlename, param2->title_len);
		param2->path_len = sce_paf_private_snprintf(param2->outpath, PATH_MAX - sizeof(fn) - param2->img_ext_len, "photo0:/SCREENSHOT/%s", titlename);
		sceIoMkdir(param2->outpath, 6);
		
		
		sce_paf_private_snprintf(param2->outpath + param2->path_len, PATH_MAX - param2->path_len - 1, "/%s%s", fn, param3->img_ext);
		param2->path_len = sce_paf_private_strlen(param2->outpath);
		
		sce_paf_private_memset(param2->titlename, 0, param2->title_len);
		sce_paf_private_memcpy(param2->titlename, param3->titlename, param2->title_len);
		
		ret = 0;
	} else
		ret = TAI_CONTINUE(int, hook_ref[0], r1, param2, param3);
	return ret;
}


void _start() __attribute__ ((weak, alias ("module_start")));

int module_start(SceSize argc, const void *args) {
	tai_module_info_t tai_info;
	tai_info.size = sizeof(tai_module_info_t);
	taiGetModuleInfo("SceAvMediaService", &tai_info);

	switch(tai_info.module_nid){
		case 0x1656745F: // 3.60 Devkit
			hook_uid[0] = taiHookFunctionOffset(&hook_ref[0], tai_info.modid, 0, 0x3e64, 1, hook_func1);
			break;

		default: //Retail 3.73 - 3.60
			hook_uid[0] = taiHookFunctionOffset(&hook_ref[0], tai_info.modid, 0, 0x3e30, 1, hook_func1);
	}
 return SCE_KERNEL_START_SUCCESS;
}
int module_stop(SceSize argc, const void *args) {
	if (hook_uid[0] >= 0) taiHookRelease(hook_uid[0],hook_ref[0]);
	return SCE_KERNEL_STOP_SUCCESS;
}