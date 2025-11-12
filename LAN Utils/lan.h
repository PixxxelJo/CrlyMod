// LAN.h
#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <functional>
#include <atomic>
#include <iostream>
#include <asio.hpp>

class LANServer {
public:
    LANServer(unsigned short port)
        : ioContext_(), acceptor_(ioContext_), port_(port), running_(false) {}

    void start() {
        asio::ip::tcp::endpoint endpoint(asio::ip::tcp::v4(), port_);
        acceptor_.open(endpoint.protocol());
        acceptor_.set_option(asio::ip::tcp::acceptor::reuse_address(true));
        acceptor_.bind(endpoint);
        acceptor_.listen();
        running_ = true;
        acceptConnections();
        ioThread_ = std::thread([this]() { ioContext_.run(); });
        std::cout << "Master server started on port " << port_ << "\n";
    }

    void stop() {
        running_ = false;
        ioContext_.stop();
        if (ioThread_.joinable()) ioThread_.join();
        std::cout << "Master server stopped.\n";
    }

    void broadcast(const std::string& message) {
        std::lock_guard<std::mutex> lock(clientMutex_);
        for (auto& client : clients_) {
            asio::async_write(*client.second,
                asio::buffer(message + "\n"),
                [](std::error_code, std::size_t) {});
        }
    }

private:
    void acceptConnections() {
        auto socket = std::make_shared<asio::ip::tcp::socket>(ioContext_);
        acceptor_.async_accept(*socket, [this, socket](std::error_code ec) {
            if (!ec && running_) {
                std::lock_guard<std::mutex> lock(clientMutex_);
                std::string id = socket->remote_endpoint().address().to_string();
                clients_[id] = socket;
                std::cout << "Client connected: " << id << "\n";
                readClient(id, socket);
            }
            if (running_) acceptConnections();
        });
    }

    void readClient(const std::string& id, std::shared_ptr<asio::ip::tcp::socket> socket) {
        auto buffer = std::make_shared<std::vector<char>>(1024);
        socket->async_read_some(asio::buffer(*buffer), 
            [this, id, socket, buffer](std::error_code ec, std::size_t length) {
                if (!ec) {
                    std::string message(buffer->data(), length);
                    std::cout << "[" << id << "]: " << message;
                    broadcast("[" + id + "]: " + message);
                    readClient(id, socket);
                } else {
                    std::lock_guard<std::mutex> lock(clientMutex_);
                    clients_.erase(id);
                    std::cout << "Client disconnected: " << id << "\n";
                }
            });
    }

    asio::io_context ioContext_;
    asio::ip::tcp::acceptor acceptor_;
    unsigned short port_;
    std::unordered_map<std::string, std::shared_ptr<asio::ip::tcp::socket>> clients_;
    std::mutex clientMutex_;
    std::thread ioThread_;
    std::atomic<bool> running_;
};
