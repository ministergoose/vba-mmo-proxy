#include "account.h"
#include "udp.h"
#include "mytools.h"
#include <stdio.h>

int MMO_Login(const char *cookie) {
    if (cookie == NULL || strlen(cookie) == 0) {
        return false;
    } else {
        char login_message[100];
        sprintf_s(login_message, 100, "login~~%s", cookie);
        MMO_UdpSendString(login_message);
    
        char response_buffer[512];
        int rec_size = MMO_ReadServerUpdate(response_buffer, 512);
        if (MMO_IsRecvError(rec_size)) {
            return 0;
        }

        return strcmp(response_buffer, "success") == 0;
    }
}

char *MMO_CreateAccount(const char* name, const char* email) {
    if (name == NULL || strlen(name) < 1 || email == NULL || strlen(email) < 1) {
        return NULL;
    }

    char create_message[512];
    sprintf_s(create_message, 512, "create~~%s~~%s", name, email);

    MMO_UdpSendString(create_message);

    char response_buffer[512];
    int rec_size = MMO_ReadServerUpdate(response_buffer, 512);
    if (MMO_IsRecvError(rec_size)) {
        return NULL;
    }

    if (strncmp("success~~", response_buffer, strlen("success~~")) == 0) {
        char* cookie = response_buffer;
        cookie += strlen("success~~");
        return cookie;
    }

    return NULL;
}
