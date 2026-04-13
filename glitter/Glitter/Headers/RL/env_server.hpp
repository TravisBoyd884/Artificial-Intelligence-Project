#pragma once
#include <string>
#include <vector>
#include <cstdint>

class EnvServer {
public:
    explicit EnvServer(const std::string& socketPath);
    ~EnvServer();

    void waitForClient();

    struct Action {
        uint8_t cmd;   // 0 = reset, 1 = step
        float yaw, pitch, power;
    };

    Action recvAction();
    void   sendObs(const std::vector<uint8_t>& pixels, float reward, bool done);

    int clientFd() const { return m_clientFd; }

private:
    int         m_serverFd = -1;
    int         m_clientFd = -1;
    std::string m_socketPath;

    void writeAll(const void* buf, size_t n);
    void readAll (void* buf, size_t n);
};
