// client.cpp
#include <boost/asio.hpp>
#include <iostream>
#include <string>

using boost::asio::ip::tcp;

int main() {
    try {
        boost::asio::io_context io;
        tcp::socket socket(io);
        socket.connect(tcp::endpoint(boost::asio::ip::make_address("127.0.0.1"), 12345));

        std::cout << "가상 SSD 쉘에 접속했습니다. (종료하려면 'exit' 입력)\n";
        std::cout << "사용법: write [LBA] [데이터] / read [LBA]\n";

        while (true) {
            std::cout << "SSD_Shell> ";
            std::string msg;
            std::getline(std::cin, msg);

            if (msg == "exit") break;
            if (msg.empty()) continue;

            // 서버로 명령어 전송
            boost::asio::write(socket, boost::asio::buffer(msg));

            // 서버 응답 대기 및 출력
            char reply[1024] = { 0 };
            boost::system::error_code ec;
            size_t length = socket.read_some(boost::asio::buffer(reply), ec);

            if (ec == boost::asio::error::eof) {
                std::cout << "서버와의 연결이 끊어졌습니다.\n";
                break;
            }

            std::cout << std::string(reply, length) << std::endl;
        }
    }
    catch (std::exception& e) {
        std::cerr << "클라이언트 에러: " << e.what() << std::endl;
    }
}