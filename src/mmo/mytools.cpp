#include "mytools.h"
#include <wtypes.h>
#include <atlstr.h>

#define MMO_INI_SECTION "preferences"
#define MMO_INI_FNAME "vba_mmo.ini"

static char buffer[2048];
static CString iniPath;
static int isPathInit = 0;
static FILE* fp = NULL;

static void MMO_InitIni() {
    char winBuffer[2048];

    GetModuleFileName(NULL, winBuffer, 2048);
    char* p = strrchr(winBuffer, '\\');
    if (p)
        *p = 0;

    iniPath.Format("%s\\%s", winBuffer, MMO_INI_FNAME);
    isPathInit = 1;
}


void MMO_Log(const char* aFormat, ...) {

    if (fp == NULL) {
        fp = fopen("vba_mmo_log.txt", "w");
    }

    va_list ap;
    va_start(ap, aFormat);
    (void)vfprintf(fp, aFormat, ap);
    va_end(ap);
    fflush(fp);
}

//void MMO_Log(char* str) {
//    if (fp == NULL) {
//        fp = fopen("vba_log.txt", "w");
//    }
//    fputs(str, fp);
//    fflush(fp);
//}

void MMO_SaveParamInt(const char* key, int value) {
    if (!isPathInit) {
        MMO_InitIni();
    }

    wsprintf(buffer, "%u", value);
    WritePrivateProfileString(MMO_INI_SECTION, key, buffer, iniPath);
}

void MMO_SaveParamStr(const char* key, const char *value) {
    if (!isPathInit) {
        MMO_InitIni();
    }

    WritePrivateProfileString(MMO_INI_SECTION, key, value, iniPath);
}

char *MMO_LoadParamStr(const char* key, char *default) {
    if (!isPathInit) {
        MMO_InitIni();
    }

    int res = GetPrivateProfileString(MMO_INI_SECTION, key, default, (LPTSTR)buffer, 2048, iniPath);
    if (res)
        return buffer;

    return default;
}

int MMO_LoadParamInt(const char* key, int default) {
    if (!isPathInit) {
        MMO_InitIni();
    }

    return GetPrivateProfileInt(MMO_INI_SECTION, key, default, iniPath);
}