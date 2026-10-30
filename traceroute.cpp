#include "traceroute.h"

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
  DEBUG << "Filling in IP header..." << ENDL;

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
  DEBUG << "Filling in ICMP header..." << ENDL;

  struct icmphdr *icmp = (struct icmphdr *) packet;

  icmp->type = ICMP_ECHO; // Type (Echo Request)
  icmp->code = 0; // Code
  icmp->checksum = 0; // Checksum (initially 0)
  icmp->un.echo.id = htons(PACKET_ID); // Id for matching echo replies
  icmp->un.echo.sequence = htons(SEQ_NUM); // Sequence Number

}

void buildDatagram (char *packet, const std::string &destIP) { 
  fill_in_IP_header(packet, destIP); 
  fill_in_ICMP_header(packet + 20, destIP); // Skip IP header (20 bytes)
  memset(packet + 20 + 8, 'Z', BUFFER_SIZE - 20 - 8); // Fill payload with 'Z'
}

// Handle receiving of packets
void receivePacket(const int &recvFD, char *recvBuffer, bool &reply_not_received, bool &not_done_reading) {
  DEBUG << "Receiving packet..." << ENDL;

  // Prepare and receive the packet
  struct sockaddr_in recv_addr;
  socklen_t addr_len = sizeof(recv_addr);
  int bytes_received = recvfrom(recvFD, recvBuffer, 32 * BUFFER_SIZE, 0, (struct sockaddr *)&recv_addr, &addr_len);
  if (bytes_received < 0) { 
    ERROR << "recvfrom" << ENDL;
  }

  // Get headers
  struct iphdr *recvIP = (struct iphdr *)recvBuffer;
  int ipHeaderLength = recvIP->ihl * 4;
  struct icmphdr *recvICMP = (struct icmphdr *)(recvBuffer + ipHeaderLength);

  // Check length
  if (recvIP->version != 4 ||
    ipHeaderLength < (int)sizeof(struct iphdr) ||
    bytes_received < ipHeaderLength + (int)sizeof(struct icmphdr)) {
    return;
  }

  // Convert address to dotted decimal
  char respondentIP[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &recv_addr.sin_addr, respondentIP, sizeof(respondentIP));

  if (recvICMP->type == ICMP_ECHOREPLY) {

    // Match ids
    if (recvICMP->un.echo.id != htons(PACKET_ID)) {
      DEBUG << "Received ICMP packet with unexpected ID: " << ntohs(recvICMP->un.echo.id) << ENDL;
      return; // Ignore packets that don't match the expected ID
    }

    std::cout << "Received ICMP Echo Reply from " << respondentIP << std::endl;
    reply_not_received = false; // Stop traceroute
    not_done_reading = false; // Stop reading

  } else if (recvICMP->type == ICMP_TIME_EXCEEDED && recvICMP->code == ICMP_EXC_TTL) {
    std::cout << "Received ICMP Time Exceeded from " << respondentIP << std::endl;
    not_done_reading = false; // Stop reading
  } else {
    DEBUG << "Received unexpected ICMP type: " << (int)recvICMP->type << ENDL;
  }
}

// Traceroute program
void traceroute(const std::string &destIP) {
  char sendBuffer[BUFFER_SIZE];
  char recvBuffer[32 * BUFFER_SIZE];

  // Build datagram to send
  buildDatagram(sendBuffer, destIP);

  // Create send and recieve sockets
  int sendFD, recvFD;
  if ((sendFD = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) == -1) { 
    ERROR << "socket" << ENDL;
  } 
  if ((recvFD = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) == -1) { 
    ERROR << "socket" << ENDL;
  }

  // Set up destination address
  struct sockaddr_in dest_addr;
  memset(&dest_addr, 0, sizeof(dest_addr));
  dest_addr.sin_family = AF_INET;
  dest_addr.sin_addr.s_addr = inet_addr(destIP.c_str());
  // //Convert an address structure to a dotted decimal string. 
  // inet_ntop(AF_INET, &recv_addr.sin_addr, respondent_ip, INET_ADDRSTRLEN);

  int currTTL = 2;
  bool reply_not_received = true;
  while ((currTTL <= 31) && (reply_not_received)) { // Max of 30 hops
    // Set TTL in IP header
    struct iphdr *ip = (struct iphdr *)sendBuffer;
    ip->ttl = currTTL;
    DEBUG << "Sending packet with TTL: " << currTTL << ENDL;

    // Compute ICMP checksum, covers payload and ICMP header
    struct icmphdr *icmp = (struct icmphdr *)(sendBuffer + 20);
    icmp->checksum = 0; // Reset checksum
    icmp->checksum = checksum((unsigned short *)(sendBuffer + 20), BUFFER_SIZE - 20);

    // 6c, send buffer
    if (sendto(sendFD, sendBuffer, BUFFER_SIZE, 0, (struct sockaddr *) &dest_addr, sizeof(dest_addr)) == -1) { 
      ERROR << "sendto" << ENDL;
    }

    bool not_done_reading = true;
    auto start_time = std::chrono::high_resolution_clock::now();
    // auto end_time = start_time + std::chrono::seconds(15); // 15 second timeout
    auto end_time = start_time + std::chrono::seconds(3); // 15 second timeout
    fd_set readSet;
    // Wait for 15 seconds max
    while (std::chrono::high_resolution_clock::now() < end_time && not_done_reading) {
      FD_ZERO(&readSet);
      FD_SET(recvFD, &readSet);
      struct timeval timeout;
      // timeout.tv_sec = std::min(5L, std::chrono::duration_cast<std::chrono::seconds>(
      //                           end_time - std::chrono::high_resolution_clock::now()).count() + 1);
      timeout.tv_sec = 1;
      timeout.tv_usec = 0;

      int selectResult = select(recvFD + 1, &readSet, NULL, NULL, &timeout);
      if (selectResult < 0) {
        ERROR << "select" << ENDL;
      } else if (selectResult == 0) {
        DEBUG << "Timeout" << ENDL;
      } else if (FD_ISSET(recvFD, &readSet)) { // Socket is ready
        receivePacket(recvFD, recvBuffer, reply_not_received, not_done_reading); // Handle packet
      } else {
        DEBUG << "Unexpected select result" << ENDL;
      }
    }

    if (not_done_reading) {
      std::cout << "No response with TTL of " << currTTL << std::endl;
    }

    // First: currTTL = 2, seqNum = 1
    // Second: currTTL = 3, seqNum = 2
    // etc. until currTTL = 31, seqNum = 30
    icmp->un.echo.sequence = htons(currTTL);
    currTTL++;
  }

  // Close sockets
  close(sendFD);
  close(recvFD);
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

  // Ensure destination IP is provided
  if (destIP.empty()) {
    std::cout << "No destination specified" << std::endl;
    exit(-1);
  }

  traceroute(destIP);

}
