#ifndef HEADER_H
#define HEADER_H 

#include <cstdio>
#include <cstring>
#include <string>
#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netinet/ip_icmp.h>

const int BUFFER_SIZE = 64;
inline int LOG_LEVEL = 0;
#define TRACE   if (LOG_LEVEL > 5) { std::cout << "TRACE: "
#define DEBUG   if (LOG_LEVEL > 4) { std::cout << "DEBUG: "
#define INFO    if (LOG_LEVEL > 3) { std::cout << "INFO: "
#define WARNING if (LOG_LEVEL > 2) { std::cout << "WARNING: "
#define ERROR   if (LOG_LEVEL > 1) { std::cout << "ERROR: "
#define FATAL   if (LOG_LEVEL > 0) { std::cout << "FATAL: "
#define ENDL  " (" << __FILE__ << ":" << __LINE__ << ")" << std::endl; }

uint16_t checksum(unsigned short *buffer, int size);

void traceroute(const std::string &destIP);
void buildDatagram (char *packet, const std::string &destIP);
void fill_in_IP_header(char *packet, const std::string &destIP);
void fill_in_ICMP_header(char *packet, const std::string &destIP);
void receivePacket(const int &recvFD, char *recvBuffer, bool &reply_not_received, bool &not_done_reading);

#endif
