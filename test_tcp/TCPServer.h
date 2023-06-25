#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <optional>
#include <unordered_map>
#include <functional>
#include <WS2tcpip.h>
#include <WinSock2.h>

namespace KHAS {
    class TCPServer
    {

    public:
        explicit TCPServer(std::string&& ip, std::string&& port) noexcept;
        void run() noexcept;
        void redirectUrl(std::string&& url_path, std::string&& file_path) noexcept;
    private:
        void showErrors() noexcept;
        void sendToClient(std::optional<SOCKET> client_socket, std::string&& response) noexcept;
        [[nodiscard]] bool startupSocket() noexcept;
        [[nodiscard]] bool createListenSocket() noexcept;
        [[nodiscard]] bool bindSocket() noexcept;
        [[nodiscard]] bool listenSocket() noexcept;
        [[nodiscard]] bool recvClientSocket(std::optional<SOCKET> client_socket) noexcept;
        [[nodiscard]] std::optional<SOCKET> acceptSocket() noexcept;
        [[nodiscard]] std::string httpErrors(std::string&& code_error, std::string&& text_error) const noexcept;

        std::optional<std::unordered_map<std::string, std::string>> parseHeader(std::stringstream&& ss) noexcept;
    private:
        std::string ip_{};
        std::string port_{};
        int socket_{};
        SOCKET listen_socket_{};
        addrinfo* addr_info_{};
        std::vector<std::string> error_logging_;
        std::unordered_map<std::string, std::string> binding_url_;
    };


}