#include "TCPServer.h"
#include <sstream>
#include <fstream>
#include <algorithm>

namespace KHAS {
    TCPServer::TCPServer(std::string&& ip, std::string&& port) noexcept
        : ip_{ "127.0.0.1" }
        , port_{ "8000" }
        , socket_{ -1 }
        , listen_socket_{ INVALID_SOCKET }
        , addr_info_{}
    {
        error_logging_.reserve(50);

        if (!startupSocket()) {
            std::cout << "startupSocket status:\tNOT OK\n";
            return;
        }
        std::cout << "startupSocket status:\tOK\n";
        
        if (!createListenSocket()) {
            std::cout << "createListenSocket\n\tstatus:\tNOT OK\n";
            return;
        }
        std::cout << "getaddrinfo status: \tOK\n";
        std::cout << "\nip:\t" << ip_ << "\n";
        std::cout << "port:\t" << port_ << "\n\n";
        std::cout << "createListenSocket\n\tstatus:\tOK\n";
        
        if (!bindSocket()) {
            std::cout << "bindSocket status:\tNOT OK\n";
            return;
        }
        std::cout << "bindSocket status:\tOK\n";


        if (!listenSocket()) {
            std::cout << "listenSocket status:\tNOT OK\n";
            return;
        }
        std::cout << "listenSocket status:\tOK\n";
        
    }

    void TCPServer::run() noexcept
    {
        for (;;) {
            auto client_socket{ acceptSocket() };
            if (!client_socket.has_value()) {
                std::cout << "accept status:\tNOT OK\n";
                return;
            }
            std::cout << "accept status: [" << client_socket.value() << "]\tOK\n";

            std::cout << "recvClientSocket status: [" << client_socket.value() << "]\t" 
                << (recvClientSocket(client_socket) ? "" : "NOT") << " OK\n";
            closesocket(client_socket.value());
            showErrors();
        }
    }

    void TCPServer::redirectUrl(std::string&& url_path, std::string&& file_path) noexcept
    {
        binding_url_.insert(std::pair(std::move(url_path), std::move(file_path)));
    }

    void TCPServer::showErrors() noexcept
    {
        for (auto&& elem : error_logging_) {
            std::cerr << elem << "\n";
        }
        std::cerr << std::endl;
    }

    bool TCPServer::startupSocket() noexcept
    {
        WSADATA wsa;
        socket_ = WSAStartup(MAKEWORD(2, 2), &wsa);
        if (socket_ != 0) {
            error_logging_.emplace_back("Can not Create Socket, return code: " + std::to_string(socket_));
            return false;
        }
        return true;
    }

    bool TCPServer::createListenSocket() noexcept
    {
        addrinfo hints{};
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;
        hints.ai_flags = AI_PASSIVE;

        socket_ = getaddrinfo(ip_.c_str(), port_.c_str(), &hints, &addr_info_);
        if (socket_ != 0) {
            error_logging_.emplace_back("getaddrinfo failed, return code: " + std::to_string(socket_));
            return false;
        }

        if (!addr_info_) {
            std::stringstream ss;
            ss << "addr_info_ == nullptr\n";
            error_logging_.emplace_back(ss.str());
            freeaddrinfo(addr_info_);
            closesocket(listen_socket_);
            WSACleanup();
            return false;
        }

        listen_socket_ = socket(addr_info_->ai_family, addr_info_->ai_socktype, addr_info_->ai_protocol);
        if (listen_socket_ == INVALID_SOCKET) {
            std::stringstream ss;
            ss << "Error at socket: " << WSAGetLastError() << "\n";
            error_logging_.emplace_back(ss.str());
            freeaddrinfo(addr_info_);
            WSACleanup();
            return false;
        }
        
        return true;
    }

    bool TCPServer::bindSocket() noexcept
    {
        if (!addr_info_) {
            std::stringstream ss;
            ss << "addr_info_ == nullptr\n";
            error_logging_.emplace_back(ss.str());
            freeaddrinfo(addr_info_);
            closesocket(listen_socket_);
            WSACleanup();
            return false;
        }

        socket_ = bind(listen_socket_, addr_info_->ai_addr, (int)addr_info_->ai_addrlen);
        if (socket_ == SOCKET_ERROR) {
            std::stringstream ss;
            ss << "bind failed with error: " << WSAGetLastError() << "\n";
            error_logging_.emplace_back(ss.str());
            freeaddrinfo(addr_info_);
            closesocket(listen_socket_);
            WSACleanup();
            return false;
        }
        return true;
    }

    bool TCPServer::listenSocket() noexcept
    {
        if (listen(listen_socket_, SOMAXCONN) == SOCKET_ERROR) {
            std::stringstream ss;
            ss << "listen failed with error: " << WSAGetLastError() << "\n";
            error_logging_.emplace_back(ss.str());
            closesocket(listen_socket_);
            WSACleanup();
            return false;
        }
        return true;
    }

    std::optional<SOCKET> TCPServer::acceptSocket() noexcept
    {
        SOCKET client_socket{ accept(listen_socket_, NULL, NULL) };
        if (client_socket == INVALID_SOCKET) {
            std::stringstream ss;
            ss << "accept failed: " << WSAGetLastError() << "\n";
            error_logging_.emplace_back(ss.str());
            closesocket(listen_socket_);
            WSACleanup();
            return std::nullopt;
        }
        return client_socket;
    }

    bool TCPServer::recvClientSocket(std::optional<SOCKET> client_socket) noexcept
    {
        sockaddr_in* sockaddr_ipv4 = (struct sockaddr_in*)addr_info_->ai_addr;
        char ipAddress[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(sockaddr_ipv4->sin_addr), ipAddress, INET_ADDRSTRLEN);
        std::cout << "ipv4: " << ipAddress;

        if (!client_socket.has_value()) {
            std::stringstream ss;
            ss << "recvClientSocket failed: " << WSAGetLastError() << "\n";
            error_logging_.emplace_back(ss.str());
            return false;
        }
        constexpr const int max_client_buffer_size = 8192;
        char buf[max_client_buffer_size]{};
        socket_ = recv(client_socket.value(), buf, max_client_buffer_size, 0);
        if (socket_ == SOCKET_ERROR) {
            std::stringstream ss;
            ss << "recv failed: " << WSAGetLastError() << "\n";
            error_logging_.emplace_back(ss.str());
            closesocket(client_socket.value());
            return false;
        }
        else if (socket_ == 0) {
            std::stringstream ss;
            ss << "connection closed...\n";
            error_logging_.emplace_back(ss.str());
            return false;
        }
        else if (socket_ > 0) 
        {
            std::stringstream sbuf;
            sbuf << buf;
            auto map_header_opt{ parseHeader(std::move(sbuf)) };

            if (!map_header_opt.has_value()) {
                std::stringstream response;
                std::stringstream ss;
                ss << "error: 500, Internal Server Error!" << "\n";
                error_logging_.emplace_back(ss.str());

                response << httpErrors("500", "Internal Server Error");
                sendToClient(client_socket, response.str());
                closesocket(client_socket.value());
                return false;
            }

            if (socket_ >= max_client_buffer_size) {
                std::stringstream ss;
                ss << "url: " << map_header_opt.value()["Path"] << "\n";
                ss << "error: 413, Payload Too Large!" << "\n";
                error_logging_.emplace_back(ss.str());
                std::stringstream response; 
                response << httpErrors("413", "Payload Too Large");

                sendToClient(client_socket, response.str());
                closesocket(client_socket.value());
                return false;
            }

            std::stringstream response;
            auto fn{ binding_url_.find(map_header_opt.value()["Path"]) };
            if (fn != binding_url_.end()) {
                std::ifstream file((*fn).second);
                if (file.is_open()) {
                    std::stringstream response_body;
                    response_body << file.rdbuf();
                    response << "HTTP/1.1 200 Ok\r\n"
                        << "Version: HTTP/1.1\r\n"
                        << "Content-Type: text/html; charset=utf-8\r\n"
                        << "Content-Length: " << response_body.str().length()
                        << "\r\n\r\n"
                        << response_body.str();

                    sendToClient(client_socket, response.str());
                    closesocket(client_socket.value());
                    return true;
                }
                std::stringstream ss;
                ss << "url: " << map_header_opt.value()["Path"] << "\n";
                ss << "error: 500, Internal Server Error!" << "\n";
                error_logging_.emplace_back(ss.str());
                response << httpErrors("500", "Internal Server Error");
                sendToClient(client_socket, response.str());
                closesocket(client_socket.value());
                return false;
            }
            std::stringstream ss;
            ss << "url: " << map_header_opt.value()["Path"] << "\n";
            ss << "error: 404, Not Found!" << "\n";
            error_logging_.emplace_back(ss.str());
            response << httpErrors("404", "Not Found");
            sendToClient(client_socket, response.str());
            closesocket(client_socket.value());
            return false;
        }
        return false;
    }

    void TCPServer::sendToClient(std::optional<SOCKET> client_socket, std::string&& response) noexcept
    {
        if (!client_socket.has_value()) {
            std::stringstream ss;
            ss << "sendToClient failed: " << WSAGetLastError() << "\n";
            error_logging_.emplace_back(ss.str());
            return;
        }

        socket_ = send(client_socket.value(), response.c_str(), static_cast<int>(response.length()), 0);
        if (socket_ != SOCKET_ERROR) return;

        std::stringstream ss;
        ss << "send failed: " << WSAGetLastError() << "\n";
        error_logging_.emplace_back(ss.str());
        closesocket(client_socket.value());
    }

    std::string TCPServer::httpErrors(std::string&& code_error, std::string&& text_error) const noexcept
    {
        std::stringstream response;
        std::stringstream response_body;
        response_body << "<title>C++ HTTP Server</title>\n"
            << "<h1>" << code_error << "</h1>\n"
            << "<p>" << text_error << "</p>\n"
            << "<em><small>C++ Http Server</small></em>\n";
        response << "HTTP/1.1 200 Ok\r\n"
            << "Version: HTTP/1.1\r\n"
            << "Content-Type: text/html; charset=utf-8\r\n"
            << "Content-Length: " << response_body.str().length()
            << "\r\n\r\n"
            << response_body.str();
        return response.str();
    }

    std::optional<std::unordered_map<std::string, std::string>> TCPServer::parseHeader(std::stringstream&& ss) noexcept
    {
        std::unordered_map<std::string, std::string> base;
        base.reserve(50);

        {
            /* parse first line */
            std::string line;
            std::getline(ss, line);
            auto first_iter{ line.begin() };
            auto second_iter{ line.end() };

            second_iter = std::find(first_iter, line.end(), ' ');
            if (second_iter == line.end()) second_iter = std::prev(second_iter);
            base.insert(std::make_pair("Type", std::string(first_iter, second_iter)));
            first_iter = ++second_iter;

            second_iter = std::find(first_iter, line.end(), ' ');
            if (second_iter == line.end()) second_iter = std::prev(second_iter);
            base.insert(std::make_pair("Path", std::string(first_iter, second_iter)));
            first_iter = ++second_iter;

            second_iter = std::find(first_iter, line.end(), ' ');
            if (second_iter == line.end()) second_iter = std::prev(second_iter);
            base.insert(std::make_pair("Version", std::string(first_iter, second_iter)));
            first_iter = ++second_iter;
        }
        
        /* parse rest line */
        for (std::string line; std::getline(ss, line);) {
            auto fn{ std::find(line.begin(), line.end(), ':') };
            if (fn == line.end()) break;
            std::string key(line.begin(), fn);
            std::string value(std::next(fn, 2), std::prev(line.end()));
            base.insert(std::make_pair(std::move(key), std::move(value)));
        }

        return base;
    }
}