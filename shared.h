#ifndef __SHARED_H
#define __SHARED_H

#include <cstdint>

#ifdef __cplusplus
extern "C" {
#endif

extern void userInit(int argc, char **argv);
extern int userLoop();

extern int userAddLPMRule(uint32_t dst_ip, uint8_t cidr, uint8_t dst_port);
extern int userDelLPMRule(uint32_t dst_ip, uint8_t cidr);
extern int userGetNextHop(uint32_t dst_ip, uint8_t cidr);

#ifdef __cplusplus
}
#endif

#endif
