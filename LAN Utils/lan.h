#pragma once
#include <string>
#include <thread>
#include <asio.hpp> 
#include <iostream>

class LANClient {
public:
    LANClient(const std::string& serverIP, unsigned short serverPort)
        : ioContext_(), socket_(ioContext_), serverIP_(serverIP), serverPort_(serverPort) {}

    bool connect() {
        try {
            asio::ip::tcp::resolver resolver(ioContext_);
            auto endpoints = resolver.resolve(serverIP_, std::to_string(serverPort_));
            asio::connect(socket_, endpoints);
            std::cout << "Connected to master server at " << serverIP_ << ":" << serverPort_ << "\n";
            readThread_ = std::thread([this]() { ioContext_.run(); });
            return true;
        } catch (std::exception& e) {
            std::cerr << "Connection failed: " << e.what() << "\n";
            return false;
        }
    }

    void sendMessage(const std::string& message) {
        asio::write(socket_, asio::buffer(message + "\n"));
    }

    void readMessages() {
        auto buffer = std::make_shared<std::vector<char>>(1024);
        socket_.async_read_some(asio::buffer(*buffer), [this, buffer](std::error_code ec, std::size_t length) {
            if (!ec) {
                std::string msg(buffer->data(), length);
                std::cout << "[Server]: " << msg;
                readMessages();
            } else {
                std::cerr << "Disconnected from server.\n";
            }
        });
    }

    void stop() {
        ioContext_.stop();
        if (readThread_.joinable()) readThread_.join();
    }

private:
    asio::io_context ioContext_;
    asio::ip::tcp::socket socket_;
    std::string serverIP_;
    unsigned short serverPort_;
    std::thread readThread_;
};
