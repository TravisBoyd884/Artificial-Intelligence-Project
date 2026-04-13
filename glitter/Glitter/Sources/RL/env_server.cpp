#include "RL/env_server.hpp"

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <stdexcept>

EnvServer::EnvServer(const std::string& socketPath) : m_socketPath(socketPath) {
    unlink(socketPath.c_str());

    m_serverFd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (m_serverFd < 0)
        throw std::runtime_error("EnvServer: socket() failed");

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socketPath.c_str(), sizeof(addr.sun_path) - 1);

    if (bind(m_serverFd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0)
        throw std::runtime_error("EnvServer: bind() failed");

    if (listen(m_serverFd, 1) < 0)
        throw std::runtime_error("EnvServer: listen() failed");

    std::cout << "EnvServer: listening on " << socketPath << '\n';
}

EnvServer::~EnvServer() {
    if (m_clientFd >= 0) close(m_clientFd);
    if (m_serverFd >= 0) close(m_serverFd);
    unlink(m_socketPath.c_str());
}

void EnvServer::waitForClient() {
    std::cout << "EnvServer: waiting for client...\n";
    m_clientFd = accept(m_serverFd, nullptr, nullptr);
    if (m_clientFd < 0)
        throw std::runtime_error("EnvServer: accept() failed");
    std::cout << "EnvServer: client connected\n";
}

EnvServer::Action EnvServer::recvAction() {
    Action a{};
    readAll(&a.cmd, 1);
    if (a.cmd == 1) {
        readAll(&a.yaw,   sizeof(float));
        readAll(&a.pitch, sizeof(float));
        readAll(&a.power, sizeof(float));
    }
    return a;
}

void EnvServer::sendObs(const std::vector<uint8_t>& pixels, float reward, bool done) {
    writeAll(pixels.data(), pixels.size());
    writeAll(&reward, sizeof(float));
    uint8_t d = done ? 1 : 0;
    writeAll(&d, 1);
}

void EnvServer::writeAll(const void* buf, size_t n) {
    const char* p = static_cast<const char*>(buf);
    size_t sent = 0;
    while (sent < n) {
        ssize_t r = write(m_clientFd, p + sent, n - sent);
        if (r <= 0) throw std::runtime_error("EnvServer: write() failed");
        sent += static_cast<size_t>(r);
    }
}

void EnvServer::readAll(void* buf, size_t n) {
    char* p = static_cast<char*>(buf);
    size_t recvd = 0;
    while (recvd < n) {
        ssize_t r = read(m_clientFd, p + recvd, n - recvd);
        if (r <= 0) throw std::runtime_error("EnvServer: read() failed");
        recvd += static_cast<size_t>(r);
    }
}
