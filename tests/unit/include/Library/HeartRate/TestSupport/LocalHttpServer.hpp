#ifndef LIBRARY_HEARTRATE_TESTSUPPORT_LOCALHTTPSERVER_HPP
#define LIBRARY_HEARTRATE_TESTSUPPORT_LOCALHTTPSERVER_HPP

#include <arpa/inet.h>
#include <array>
#include <asm-generic/socket.h>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <netinet/in.h>
#include <poll.h>
#include <string>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>
#include <utility>

class LocalHttpServer
{
public:

    LocalHttpServer(LocalHttpServer&&) = delete;
    auto operator=(LocalHttpServer&&) -> LocalHttpServer& = delete;
    LocalHttpServer(
        int statusCode, std::string body
    )
        : statusCode_(statusCode)
        , body_(std::move(body))
    {
        socket_ = ::socket(AF_INET, SOCK_STREAM, 0);
        if (socket_ < 0)
        {
            return;
        }

        int reuseAddress = 1;
        static_cast<void>(
            ::setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR, &reuseAddress, sizeof(reuseAddress))
        );

        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        address.sin_port = 0;

        if (::bind(socket_, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0 ||
            ::listen(socket_, 8) < 0)
        {
            closeSocket();
            return;
        }

        socklen_t addressLength = sizeof(address);
        if (::getsockname(socket_, reinterpret_cast<sockaddr*>(&address), &addressLength) < 0)
        {
            closeSocket();
            return;
        }

        port_ = ntohs(address.sin_port);
        worker_ = std::thread([this] -> void { serve(); });
    }

    ~LocalHttpServer()
    {
        running_ = false;
        if (socket_ >= 0)
        {
            static_cast<void>(::shutdown(socket_, SHUT_RDWR));
        }
        if (worker_.joinable())
        {
            worker_.join();
        }
        closeSocket();
    }

    LocalHttpServer(const LocalHttpServer&) = delete;
    auto operator=(const LocalHttpServer&) -> LocalHttpServer& = delete;

    [[nodiscard]] auto isReady() const -> bool
    {
        return socket_ >= 0 && port_ != 0;
    }

    [[nodiscard]] auto url() const -> std::string
    {
        return "http://127.0.0.1:" + std::to_string(port_);
    }

private:

    void serve()
    {
        while (running_)
        {
            pollfd descriptor{
                .fd = socket_,
                .events = POLLIN,
                .revents = 0,
            };

            if (::poll(&descriptor, 1, 50) <= 0 || !running_)
            {
                continue;
            }

            const int client = ::accept(socket_, nullptr, nullptr);
            if (client < 0)
            {
                continue;
            }

            std::array<char, 4096> request{};
            static_cast<void>(::recv(client, request.data(), request.size(), 0));

            const std::string response = "HTTP/1.1 " + std::to_string(statusCode_) +
                                         " Test\r\n"
                                         "Content-Type: application/json\r\n"
                                         "Content-Length: " +
                                         std::to_string(body_.size()) +
                                         "\r\n"
                                         "Connection: close\r\n\r\n" +
                                         body_;

            std::size_t sent = 0;
            while (sent < response.size())
            {
                const ssize_t count =
                    ::send(client, response.data() + sent, response.size() - sent, MSG_NOSIGNAL);
                if (count <= 0)
                {
                    break;
                }
                sent += static_cast<std::size_t>(count);
            }

            static_cast<void>(::close(client));
        }
    }

    void closeSocket()
    {
        if (socket_ >= 0)
        {
            static_cast<void>(::close(socket_));
            socket_ = -1;
        }
    }

    int statusCode_{};
    std::string body_;
    int socket_{-1};
    std::uint16_t port_{};
    std::atomic<bool> running_{true};
    std::thread worker_;
};

#endif // LIBRARY_HEARTRATE_TESTSUPPORT_LOCALHTTPSERVER_HPP
