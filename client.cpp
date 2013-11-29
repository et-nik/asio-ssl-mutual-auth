//
// client.cpp
// ~~~~~~~~~~
//
// Copyright (c) 2003-2010 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <cstdlib>
#include <iostream>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

enum {
    max_length = 1024
};

class client {
public:

    client(boost::asio::io_service& io_service, boost::asio::ssl::context& context,
            boost::asio::ip::tcp::resolver::iterator endpoint_iterator)
    : socket_(io_service, context) {



        boost::asio::ip::tcp::endpoint endpoint = *endpoint_iterator;
        socket_.lowest_layer().async_connect(endpoint,
                boost::bind(&client::handle_connect, this,
                boost::asio::placeholders::error, ++endpoint_iterator));
    }

    std::string get_password() const {
        return "test";
    }

    void handle_connect(const boost::system::error_code& error,
            boost::asio::ip::tcp::resolver::iterator endpoint_iterator) {
        if (!error) {
            socket_.async_handshake(boost::asio::ssl::stream_base::client,
                    boost::bind(&client::handle_handshake, this,
                    boost::asio::placeholders::error));
        } else if (endpoint_iterator != boost::asio::ip::tcp::resolver::iterator()) {
            socket_.lowest_layer().close();
            boost::asio::ip::tcp::endpoint endpoint = *endpoint_iterator;
            socket_.lowest_layer().async_connect(endpoint,
                    boost::bind(&client::handle_connect, this,
                    boost::asio::placeholders::error, ++endpoint_iterator));
        } else {
            std::cout << "Connect failed: " << error << "\n";
        }
    }

    void handle_handshake(const boost::system::error_code& error) {
        if (!error) {
            std::cout << "Enter message: ";
            std::cin.getline(request_, max_length);
            size_t request_length = strlen(request_);

            boost::asio::async_write(socket_,
                    boost::asio::buffer(request_, request_length),
                    boost::bind(&client::handle_write, this,
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
        } else {
            std::cout << "Handshake failed: " << error << "\n";
        }
    }

    void handle_write(const boost::system::error_code& error,
            size_t bytes_transferred) {
        if (!error) {
            boost::asio::async_read(socket_,
                    boost::asio::buffer(reply_, bytes_transferred),
                    boost::bind(&client::handle_read, this,
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
        } else {
            std::cout << "Write failed: " << error << "\n";
        }
    }

    void handle_read(const boost::system::error_code& error,
            size_t bytes_transferred) {
        if (!error) {
            std::cout << "Reply: ";
            std::cout.write(reply_, bytes_transferred);
            std::cout << "\n";
        } else {
            std::cout << "Read failed: " << error << "\n";
        }
    }

private:
    boost::asio::ssl::stream<boost::asio::ip::tcp::socket> socket_;
    char request_[max_length];
    char reply_[max_length];
};

int main(int argc, char* argv[]) {
    try {
        if (argc != 4) {
            std::cerr << "Usage: client <host> <port> <certfolder>\n";
            return 1;
        }
        
        const std::string cert_folder = argv[3];

        boost::asio::io_service io_service;

        boost::asio::ip::tcp::resolver resolver(io_service);
        boost::asio::ip::tcp::resolver::query query(argv[1], argv[2]);
        boost::asio::ip::tcp::resolver::iterator iterator = resolver.resolve(query);


        boost::asio::ssl::context ctx(io_service, boost::asio::ssl::context::sslv23);

        ctx.set_options(
                boost::asio::ssl::context::default_workarounds
                | boost::asio::ssl::context::no_sslv2
                | boost::asio::ssl::context::single_dh_use);


        ctx.set_verify_mode(boost::asio::ssl::context::verify_peer || boost::asio::ssl::context::verify_fail_if_no_peer_cert);
        ctx.load_verify_file("certs/server.crt");

        ctx.use_certificate_chain_file(cert_folder+"/server.crt");
        ctx.use_private_key_file(cert_folder+"/server.key", boost::asio::ssl::context::pem);
        ctx.use_tmp_dh_file(cert_folder+"/dh512.pem");

        client c(io_service, ctx, iterator);
        io_service.run();
    } catch (boost::system::error_code& e) {
        std::cerr << "Exception: " << e.message() << "\n";
    }

    return 0;
}
