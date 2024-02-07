#ifndef __MMO_UDP_H__
#define __MMO_UDP_H__

#include <winsock2.h>
#include <stdint.h>

int MMO_UdpInit(const char* host, u_short port, DWORD timeout);
void MMO_UdpSendString(char* buffer);
void MMO_UdpSendLocation(uint32_t* buffer, uint32_t len);
int MMO_ReadServerUpdate(char* buffer, size_t len);
int MMO_IsRecvError(int );


#endif // !__MMO_UDP_H__
