#include "udp.h"
#include "mytools.h"
#include <stdio.h>

static SOCKET sockfd = INVALID_SOCKET;
static sockaddr_in serveraddr = {0};
static int serverlen;

int MMO_UdpInit(const char *host, u_short port, DWORD timeout) {

    WSADATA WsaData;
    if (WSAStartup(MAKEWORD(2, 2), &WsaData) != 0) {
        WSACleanup();
        return -1;
    }

    if (sockfd != INVALID_SOCKET) {
        closesocket(sockfd);
    }
    
    sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd == INVALID_SOCKET) {
        WSACleanup();
        return -1;
    }
    struct hostent *server = gethostbyname(host);

    serveraddr.sin_family = AF_INET;
    memcpy((char *)&serveraddr.sin_addr.s_addr, (char *)server->h_addr, server->h_length);
    serveraddr.sin_port = htons(port);
    serverlen = sizeof(serveraddr);

    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout)) == SOCKET_ERROR) {
        closesocket(sockfd);
        WSACleanup();
        return -1;
    }

    return 0;
}

void MMO_UdpSendString(char *buffer) {
    sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)&serveraddr, serverlen);
}

void MMO_UdpSendLocation(uint32_t *buffer, uint32_t len) {
    for (uint32_t i = 0; i < len; i++) {
        buffer[i] = htonl(buffer[i]);
    }
    MMO_Log("MMO_UdpSendLocation (%d)\n", len);
    for (uint32_t i = 0; i < len; i++) {
        MMO_Log("0x%08X, ", buffer[i]);
    }
    MMO_Log("\n");
    sendto(sockfd, (const char *)buffer, len, 0, (struct sockaddr *)&serveraddr, serverlen);
}

int MMO_ReadServerUpdate(char *buffer, size_t len) {
    MMO_Log("MMO_ReadServerUpdate\n");
    int n = recvfrom(sockfd, buffer, len, 0, (struct sockaddr *)&serveraddr, &serverlen);
    if (n < 0) {
        if (WSAGetLastError() == WSAETIMEDOUT) {
            return 0x80000000;
        }
        MMO_Log("socket recv error - %d\n", WSAGetLastError());
        return n;
    }
    buffer[n] = '\0';
    for (int i = 0; i < n; i++) {
        MMO_Log("0x%02X, ", (unsigned char)buffer[i]);
    }
    MMO_Log("\n");
    return n;
}

int MMO_IsRecvError(int s) {
    if (s == 0) { // socket was closed
        MMO_Log("socket was closed\n");
        return 1;
    }
    if (s < 0) {
        if (s == 0x80000000) { // Timeout
            MMO_Log("socket recv timeout\n");
            return 1;
        }
        MMO_Log("socket recv error\n");
        return 1; // Other socket error
    }
    return 0;
}
