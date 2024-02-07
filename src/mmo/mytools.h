#ifndef __MMO_MYTOOLS_H__
#define __MMO_MYTOOLS_H__

#define BIT_ENABLE(REG, BIT) (REG) |= 1 << (BIT)
#define BIT_DISABLE(REG, BIT) (REG) &= ~(1 << (BIT))
#define BIT_IS_ENABLE(REG, BIT) ((REG) & (1 << (BIT)))
#define BIT_TEST BIT_IS_ENABLE
#define BIT_BIT(BIT) (1 << (BIT))

void MMO_Log(const char* , ...);
void MMO_SaveParamInt(const char* , int );
void MMO_SaveParamStr(const char* , const char* );
int MMO_LoadParamInt(const char* , int );
char *MMO_LoadParamStr(const char* , char* );

#endif // !__MMO_MYTOOLS_H__