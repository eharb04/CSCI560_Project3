#include "traceroute.h"

#include <netinet/ip.h>
#include <netinet/ip_icmp.h>

// ****************************************************************************
// * Compute the Internet Checksum over an arbitrary buffer.
// * (written with the help of ChatGPT 3.5)
// ****************************************************************************
uint16_t checksum(unsigned short *buffer, int size) {
    unsigned long sum = 0;
    while (size > 1) {
        sum += *buffer++;
        size -= 2;
    }
    if (size == 1) {
        sum += *(unsigned char *) buffer;
    }
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    return (unsigned short) (~sum);
}

void fill_in_IP_header(char *packet, const std::string &destIP) {
  DEBUG << "Filling in IP header..." << std::endl;

  struct iphdr *ip = (struct iphdr *) packet;

  ip->version = 4; // IPv4
  ip->ihl = 5; // Internet Header Length (5 * 4 = 20 bytes)
  ip->tos = 0; // Type of Service
  ip->tot_len = htons(BUFFER_SIZE); // Total Length
  ip->id = 0; // Identification
  ip->frag_off = 0; // Fragment Offset
  ip->ttl = 2; // Time to Live
  ip->protocol = IPPROTO_ICMP; // Protocol (ICMP)
  ip->check = 0; // Checksum (initially 0)
  ip->saddr = 0; // OS can fil in
  ip->daddr = inet_addr(destIP.c_str()); // Dest IP address
}

void fill_in_ICMP_header(char *packet, const std::string &destIP) {
  DEBUG << "Filling in ICMP header..." << std::endl;

  struct icmphdr *icmp = (struct icmphdr *) packet;

  icmp->type = ICMP_ECHO; // Type (Echo Request)
  icmp->code = 0; // Code
  icmp->checksum = 0; // Checksum (initially 0)
  icmp->un.echo.id = htons(67); // Id for matching echo replies
  icmp->un.echo.sequence = htons(1); // Sequence Number

}

void buildDatagram (char *packet, const std::string &destIP) { 
  fill_in_IP_header(packet, destIP); 
  fill_in_ICMP_header(packet + 20, destIP); // Skip IP header (20 bytes)
  memset(packet + 20 + 8, 'Z', BUFFER_SIZE - 20 - 8); // Fill payload with 'Z'
} 

// Traceroute program
void traceroute(const std::string &destIP, int maxHops = 30) {
  char sendBuffer[BUFFER_SIZE];
  char recvBuffer[BUFFER_SIZE];

  // Build datagram to send
  buildDatagram(sendBuffer, destIP);

  // Create send and recieve sockets
  int sendFD, recvFD;
  if ((sendFD = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) == -1) { 
    perror("socket"); 
  } 
  if ((recvFD = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) == -1) { 
    perror("socket"); 
  }

  int currTTL = 2;
  bool reply_not_received = true;
  while ((currTTL <= maxHops) && (reply_not_received)) {
    sendBuffer->ttl = currTTL; // Set TTL in IP header

    // Compute ICMP checksum
    sendBuffer[20]->checksum = checksum(sendBuffer + 20 + 8, BUFFER_SIZE - 20 - 8);

    
  }
}


int main (int argc, char *argv[]) {
  std::string destIP;

  // ********************************************************************
  // * Process the command line arguments
  // ********************************************************************
  int opt = 0;
  while ((opt = getopt(argc,argv,"t:d:")) != -1) {

    switch (opt) {
    case 't':
      destIP = optarg;
      break;
    case 'd':
      LOG_LEVEL = atoi(optarg);;
      break;
    case ':':
    case '?':
    default:
      std::cout << "useage: " << argv[0] << " -t [target ip] -d [Debug Level]" << std::endl;
      exit(-1);
    }
  }

  traceroute(destIP);

}
