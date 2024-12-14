#include "network_interface_bsd.h"
#include <arpa/inet.h>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

namespace ArtNet {

bool NetworkInterfaceBSD::createSocket(const std::string &bindAddress,
                                       int port) {
  m_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  m_bindAddress = bindAddress;
  m_port = port;
  if (m_socket == -1) {
    std::cerr << "ArtNet: Error creating socket." << std::endl;
    return false;
  }

  // Allow socket to reuse address
  int enable = 1;
  if (setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) <
      0) {
    std::cerr << "ArtNet: Failed to set socket to reuse address" << std::endl;
    return false;
  }

  // Set socket to non-blocking
  int flags = fcntl(m_socket, F_GETFL, 0);
  if (flags == -1) {
    std::cerr << "ArtNet: Error getting socket flags" << std::endl;
    return false;
  }
  if (fcntl(m_socket, F_SETFL, flags | O_NONBLOCK) == -1) {
    std::cerr << "ArtNet: Error setting socket to non-blocking" << std::endl;
    return false;
  }
  return true;
}

bool NetworkInterfaceBSD::bindSocket() {
  sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(m_port);
  addr.sin_addr.s_addr = inet_addr(m_bindAddress.c_str());

  if (bind(m_socket, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) == -1) {
    std::cerr << "ArtNet: Error binding socket to address: " << m_bindAddress
              << ":" << m_port << std::endl;
    return false;
  }
  return true;
}

bool NetworkInterfaceBSD::sendPacket(const std::vector<uint8_t> &packet,
                                     const std::string &address, int port) {
  if (m_socket == -1) {
    std::cerr << "ArtNet: Socket not initialized" << std::endl;
    return false;
  }

  sockaddr_in broadcastAddr;
  broadcastAddr.sin_family = AF_INET;
  broadcastAddr.sin_port = htons(port);
  broadcastAddr.sin_addr.s_addr = inet_addr(address.c_str());

  ssize_t bytesSent = sendto(m_socket, packet.data(), packet.size(), 0,
                             reinterpret_cast<sockaddr *>(&broadcastAddr),
                             sizeof(broadcastAddr));

  if (bytesSent == -1) {
    std::cerr << "ArtNet: Error sending packet: " << strerror(errno)
              << std::endl;
    return false;
  }
  return true;
}

int NetworkInterfaceBSD::receivePacket(std::vector<uint8_t> &buffer) {
  sockaddr_in senderAddr;
  socklen_t addrLen = sizeof(senderAddr);

  ssize_t bytesReceived =
      recvfrom(m_socket, buffer.data(), buffer.size(), 0,
               reinterpret_cast<sockaddr *>(&senderAddr), &addrLen);
  if (bytesReceived == -1) {
    if (errno != EAGAIN && errno != EWOULDBLOCK) {
      std::cerr << "ArtNet: Error receiving data: " << strerror(errno)
                << std::endl;
    }

    return 0; // Non-blocking socket returns 0 if no data
  }
  return static_cast<int>(bytesReceived);
}

void NetworkInterfaceBSD::closeSocket() {
  if (m_socket != -1) {
    close(m_socket);
    m_socket = -1;
  }
}

} // namespace ArtNet
