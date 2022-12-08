#pragma once

#include <cstdint>

extern int userLoop();

extern int userAddLPMRule(uint32_t dst_ip, uint8_t cidr, uint8_t dst_port);
extern int userDelLPMRule(uint32_t dst_ip, uint8_t cidr);
extern int userGetNextHop(uint32_t dst_ip, uint8_t cidr);
