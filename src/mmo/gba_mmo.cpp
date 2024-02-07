#include "../GBA.h"
#include "../Port.h"
#include "gba_mmo.h"
#include "account.h"
#include "udp.h"
#include "mytools.h"


static u32 incoming_buffer[30];
static u32 incoming_buffer_position = 0;
static u32 expected_length = 0;

static CRITICAL_SECTION mmo_cs;
static HANDLE mmo_sync = NULL;
static HANDLE mmo_write_sync = NULL;
static HANDLE mmo_thread = NULL;

DWORD WINAPI MMOThread(void*);

static int mmo_isSIODATA32Update = 0;	// recv new SIODATA32
static u32 mmo_SIODATA32 = 0;			// SIODATA32 buffer

static int _checkUpdate();
static void _transfer(u32);
static int _writeUpdate(u32 u);

#define UPDATE_REG(address, value) WRITE16LE(((u16 *)&ioMem[address]),value)

static void _transfer(u32 out) {

	WaitForSingleObject(mmo_write_sync, INFINITE); // wait when can write
	ResetEvent(mmo_write_sync);
	_writeUpdate(out);

	WaitForSingleObject(mmo_sync, INFINITE); // wait SIODATA32
	ResetEvent(mmo_sync);
	u32 data = mmo_SIODATA32;
	if (expected_length == 0) {
		expected_length = data;
		incoming_buffer_position = 0;
		return;
	}
	incoming_buffer[incoming_buffer_position++] = data;

	if (incoming_buffer_position >= expected_length) {
		MMO_UdpSendLocation(incoming_buffer, expected_length * 4);

		expected_length = 0;
		incoming_buffer_position = 0;
	}
}

DWORD WINAPI MMOThread(void* param) {
	SOCKET mysocket = *((SOCKET*)param);
	int client_live = 1;
	u32 d;
	int rec_size = 0;

	u32 buffer_len = 512;
	char buffer[512];

	WaitForSingleObject(mmo_sync, INFINITE); // wait for login
	ResetEvent(mmo_sync);
	while (client_live) {
		rec_size = MMO_ReadServerUpdate(buffer, buffer_len);
		if (rec_size > 0) {
			_transfer((rec_size + 4) | 0xBEEF0000);

			for (int i = 0; i < rec_size; i++) {
				d = ((u8)buffer[i++] << 24);
				d += ((u8)buffer[i++] << 16);
				d += ((u8)buffer[i++] << 8);
				d += ((u8)buffer[i]);
				_transfer(d);
			}
			_transfer(0xDEADBEEF);
		}
	}

	CloseHandle(mmo_thread);
	mmo_thread = NULL;
	ExitThread(0);
}

int MMO_Init() {

	if (mmo_thread != NULL) {
		CloseHandle(mmo_thread);
		mmo_thread = NULL;
	}
	if (mmo_sync == NULL) {
		if ((mmo_sync = CreateEvent(NULL, TRUE, FALSE, NULL)) == NULL) {
			return 0;
		}
	}
	if (mmo_write_sync == NULL) {
		if ((mmo_write_sync = CreateEvent(NULL, TRUE, FALSE, NULL)) == NULL) {
			return 0;
		}
	}

	InitializeCriticalSection(&mmo_cs);
	ResetEvent(mmo_sync);
	ResetEvent(mmo_write_sync);

	MMO_Log("MMO_Init\n");
	return 1;
}

int MMO_Start() {
	DWORD ehh;

	MMO_Log("MMO_Start\n");
	SetEvent(mmo_sync);
	SetEvent(mmo_write_sync);

	if (mmo_thread != NULL) {
		CloseHandle(mmo_thread);
		mmo_thread = NULL;
	}

	if ((mmo_thread = CreateThread(NULL, 0, MMOThread, NULL, 0, &ehh)) == NULL) {
		return 0;
	}

	return 1;
}

void MMO_Update() {

	u16 siocnt_current = READ16LE(&ioMem[0x128]);
	if ((siocnt_current & 0x81) != 0x80) {			// check if in slave mode and start(7) bit is on
		return;
	}

	if (_checkUpdate()) {
		if (siocnt_current & 0x4000) {				// check if Interrupt is on
			IF |= 0x80;
			UPDATE_REG(0x202, IF);
		}

		BIT_DISABLE(siocnt_current, 7);				// set Start(7) bit to off
		UPDATE_REG(0x128, siocnt_current);
	}
}

static int _checkUpdate() {
	int res = 0;

	EnterCriticalSection(&mmo_cs);
	if (mmo_isSIODATA32Update) {
		u32 siodata32_current = READ32LE(&ioMem[0x120]);
		u8 *t = (u8 *)&mmo_SIODATA32;
		ioMem[0x120] = t[0];
		ioMem[0x121] = t[1];
		ioMem[0x122] = t[2];
		ioMem[0x123] = t[3];

		mmo_SIODATA32 = siodata32_current;
		SetEvent(mmo_sync);
		SetEvent(mmo_write_sync);
		mmo_isSIODATA32Update = 0;
		res = 1;
	}
	LeaveCriticalSection(&mmo_cs);

	return res;
}

static int _writeUpdate(u32 u) {
	int res = 0;
	EnterCriticalSection(&mmo_cs);
	if (!mmo_isSIODATA32Update) {
		mmo_SIODATA32 = u;
		mmo_isSIODATA32Update = 1;
		res = 1;
	}
	LeaveCriticalSection(&mmo_cs);
	return res;
}

void MMO_Stop() {
	if (mmo_sync != NULL) {
		CloseHandle(mmo_sync);
		mmo_sync = NULL;
	}
	if (mmo_sync != NULL) {
		CloseHandle(mmo_write_sync);
		mmo_write_sync = NULL;
	}
	if (mmo_thread != NULL) {
		CloseHandle(mmo_thread);
		mmo_thread = NULL;
	}
	DeleteCriticalSection(&mmo_cs);
	WSACleanup();
}

