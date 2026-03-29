// ssd_shell.cpp - Virtual SSD Shell (Client)
// 역할: 사용자 명령어를 받아 SSD 서버에 전송, 결과 출력
// 빌드: g++ -o ssd_shell ssd_shell.cpp -lboost_system -lpthread

#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include <sstream>

using boost::asio::ip::tcp;

// ─── 서버에서 전체 응답 수신 (\n 종료까지) ──────────────────
std::string receiveResponse(tcp::socket& socket) {
    std::string response;
    char ch;
    boost::system::error_code ec;

    // 서버 응답은 항상 마지막 줄이 \n으로 끝남
    while (true) {
        size_t n = boost::asio::read(socket,
                       boost::asio::buffer(&ch, 1), ec);
        if (ec || n == 0) break;
        response += ch;
        // 단일 줄 응답은 \n 하나로 끝남
        // FULLREAD 같은 멀티라인은 마지막이 항상 ")\n" 패턴
        if (ch == '\n') {
            // 멀티라인 확인: 버퍼에 더 있는지 (논블로킹 확인)
            socket.non_blocking(true);
            char peek;
            size_t peeked = socket.read_some(
                boost::asio::buffer(&peek, 1), ec);
            socket.non_blocking(false);
            if (peeked == 1) response += peek;
            else break;  // 더 없으면 종료
        }
    }
    return response;
}

// ─── 프롬프트 배너 출력 ──────────────────────────────────────
void printBanner() {
    std::cout << "\n";
    std::cout << "  ╔═══════════════════════════════════╗\n";
    std::cout << "  ║     Virtual SSD Shell v1.0        ║\n";
    std::cout << "  ║  'help' 로 명령어 목록 확인        ║\n";
    std::cout << "  ║  'exit' 로 종료                    ║\n";
    std::cout << "  ╚═══════════════════════════════════╝\n";
    std::cout << "\n";
}

// ─── 메인 ────────────────────────────────────────────────────
int main() {
    try {
        boost::asio::io_context io;
        tcp::socket socket(io);

        // 서버 연결
        socket.connect(tcp::endpoint(
            boost::asio::ip::make_address("127.0.0.1"), 12345));

        printBanner();
        std::cout << "[연결됨] SSD Server @ 127.0.0.1:12345\n\n";

        while (true) {
            std::cout << "SSD> ";
            std::string input;
            if (!std::getline(std::cin, input)) break;  // EOF 처리

            // 빈 입력 무시
            if (input.empty()) continue;

            // 서버로 명령어 전송
            boost::asio::write(socket,
                boost::asio::buffer(input + "\n"));

            // 응답 수신 및 출력
            boost::system::error_code ec;
            boost::asio::streambuf buf;
            boost::asio::read_until(socket, buf, '\n', ec);

            if (ec && ec != boost::asio::error::eof) {
                std::cerr << "[오류] 수신 실패: " << ec.message() << "\n";
                break;
            }

            std::istream is(&buf);
            std::string line;
            // 첫 줄 출력
            if (std::getline(is, line)) {
                std::cout << line << "\n";
            }

            // 멀티라인 응답 추가 수신 (FULLREAD, HELP 등)
            // 서버가 여러 줄을 보낼 때 버퍼에 남은 것 처리
            while (buf.size() > 0 && std::getline(is, line)) {
                std::cout << line << "\n";
            }

            // exit 응답이면 종료
            if (line == "EXIT" || input == "exit") {
                std::cout << "[SSD Shell] 종료합니다.\n";
                break;
            }

            std::cout << "\n";
        }

        socket.close();
    }
    catch (std::exception& e) {
        std::cerr << "[오류] " << e.what() << "\n";
        std::cerr << "SSD 서버가 실행 중인지 확인하세요.\n";
        return 1;
    }
    return 0;
}
