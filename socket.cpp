// server.cpp
#include <boost/asio.hpp>
#include <iostream>
#include <vector>
#include <sstream>
#include <string>

using boost::asio::ip::tcp;

int main() {
    try {
        boost::asio::io_context io;
        tcp::acceptor acceptor(io, tcp::endpoint(tcp::v4(), 12345));

        std::cout << "[SSD Server] 가상 SSD가 포트 12345에서 동작 중입니다.\n";

        // 가상의 SSD 저장 공간 (LBA 0 ~ 99), 초기값은 0x00000000
        std::vector<std::string> ssd_storage(100, "0x00000000");

        while (true) {
            tcp::socket socket(io);
            acceptor.accept(socket);
            std::cout << "[SSD Server] 쉘(Client)이 연결되었습니다.\n";

            while (true) {
                char data[1024] = { 0 };
                boost::system::error_code ec;

                size_t length = socket.read_some(boost::asio::buffer(data), ec);

                if (ec == boost::asio::error::eof) {
                    break; // 클라이언트가 연결을 끊음
                }
                else if (ec) {
                    throw boost::system::system_error(ec);
                }

                // 수신한 데이터를 문자열로 변환하여 파싱
                std::string request(data, length);
                std::stringstream ss(request);
                std::string cmd;
                ss >> cmd;

                std::string response;

                // 1. WRITE 명령어 처리: write [LBA] [DATA]
                if (cmd == "write") {
                    int lba;
                    std::string value;
                    ss >> lba >> value;

                    if (lba >= 0 && lba < ssd_storage.size()) {
                        ssd_storage[lba] = value;
                        response = "SUCCESS: LBA " + std::to_string(lba) + "에 데이터 기록 완료";
                    }
                    else {
                        response = "ERROR: LBA 범위 초과 (0~99)";
                    }
                }
                // 2. READ 명령어 처리: read [LBA]
                else if (cmd == "read") {
                    int lba;
                    ss >> lba;

                    if (lba >= 0 && lba < ssd_storage.size()) {
                        response = ssd_storage[lba]; // 해당 주소의 데이터 반환
                    }
                    else {
                        response = "ERROR: LBA 범위 초과 (0~99)";
                    }
                }
                // 3. 알 수 없는 명령어
                else {
                    response = "ERROR: 알 수 없는 명령어입니다. (사용법: read [LBA] 또는 write [LBA] [DATA])";
                }

                // 클라이언트(Shell)로 응답 전송
                boost::asio::write(socket, boost::asio::buffer(response));
            }
            std::cout << "[SSD Server] 쉘(Client) 연결이 해제되었습니다.\n";
        }
    }
    catch (std::exception& e) {
        std::cerr << "서버 에러: " << e.what() << std::endl;
    }
}