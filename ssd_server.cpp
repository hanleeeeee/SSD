// ssd_server.cpp - Virtual SSD Application (Server)
// 역할: 가상 SSD를 관리하는 서버. nand.txt에 100개 LBA를 저장/조회함
// 빌드: g++ -o ssd_server ssd_server.cpp -lboost_system -lpthread

#include <boost/asio.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
#include <iomanip>

using boost::asio::ip::tcp;

// ─── SSD 상수 ────────────────────────────────────────────────
const int LBA_COUNT   = 100;           // LBA 개수 (0~99)
const std::string NAND_FILE = "nand.txt"; // 가상 낸드 저장 파일
const uint32_t ERASE_VALUE  = 0x00000000; // ERASE 기본값

// ─── NAND 초기화 (파일 없으면 0x00으로 생성) ────────────────
void initNand() {
    std::ifstream fin(NAND_FILE);
    if (!fin.good()) {
        std::ofstream fout(NAND_FILE);
        for (int i = 0; i < LBA_COUNT; i++)
            fout << "0x00000000\n";
        std::cout << "[SSD] nand.txt 초기화 완료 (" << LBA_COUNT << " LBA)\n";
    }
}

// ─── LBA 전체 읽기 ───────────────────────────────────────────
std::vector<std::string> readNand() {
    std::vector<std::string> data(LBA_COUNT, "0x00000000");
    std::ifstream fin(NAND_FILE);
    for (int i = 0; i < LBA_COUNT && std::getline(fin, data[i]); i++);
    return data;
}

// ─── NAND 전체 쓰기 ─────────────────────────────────────────
void writeNand(const std::vector<std::string>& data) {
    std::ofstream fout(NAND_FILE);
    for (const auto& v : data) fout << v << "\n";
}

// ─── 16진수 문자열 검증 (0xXXXXXXXX 형식) ──────────────────
bool isValidHex(const std::string& s) {
    if (s.size() != 10) return false;
    if (s.substr(0, 2) != "0x" && s.substr(0, 2) != "0X") return false;
    return std::all_of(s.begin() + 2, s.end(), ::isxdigit);
}

// ─── 명령어 처리 핵심 함수 ───────────────────────────────────
std::string processCommand(const std::string& cmdLine) {
    std::istringstream ss(cmdLine);
    std::string cmd;
    ss >> cmd;

    // 소문자 변환
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);

    // ── READ lba ──────────────────────────────────────────────
    if (cmd == "read") {
        int lba;
        if (!(ss >> lba) || lba < 0 || lba >= LBA_COUNT)
            return "ERROR: READ 사용법: read <lba(0~99)>";
        auto data = readNand();
        return "[READ] LBA " + std::to_string(lba) + " = " + data[lba];
    }

    // ── WRITE lba value ───────────────────────────────────────
    if (cmd == "write") {
        int lba;
        std::string val;
        if (!(ss >> lba >> val) || lba < 0 || lba >= LBA_COUNT)
            return "ERROR: WRITE 사용법: write <lba> <0xXXXXXXXX>";
        if (!isValidHex(val))
            return "ERROR: 값 형식 오류 (예: 0xABCDEFFF)";

        // 대문자 통일
        std::transform(val.begin() + 2, val.end(), val.begin() + 2, ::toupper);

        auto data = readNand();
        data[lba] = val;
        writeNand(data);
        return "[WRITE] LBA " + std::to_string(lba) + " <- " + val + " 완료";
    }

    // ── ERASE lba size ────────────────────────────────────────
    if (cmd == "erase") {
        int lba, size;
        if (!(ss >> lba >> size) || lba < 0 || lba >= LBA_COUNT || size <= 0)
            return "ERROR: ERASE 사용법: erase <lba> <size(1~10)>";
        if (size > 10)
            return "ERROR: ERASE 최대 크기는 10입니다";
        if (lba + size > LBA_COUNT)
            size = LBA_COUNT - lba; // 범위 초과 시 끝까지만

        auto data = readNand();
        for (int i = lba; i < lba + size; i++)
            data[i] = "0x00000000";
        writeNand(data);
        return "[ERASE] LBA " + std::to_string(lba) + "~"
             + std::to_string(lba + size - 1) + " 초기화 완료";
    }

    // ── FULLWRITE value ───────────────────────────────────────
    if (cmd == "fullwrite") {
        std::string val;
        if (!(ss >> val) || !isValidHex(val))
            return "ERROR: FULLWRITE 사용법: fullwrite <0xXXXXXXXX>";
        std::transform(val.begin() + 2, val.end(), val.begin() + 2, ::toupper);

        std::vector<std::string> data(LBA_COUNT, val);
        writeNand(data);
        return "[FULLWRITE] 전체 " + std::to_string(LBA_COUNT)
             + " LBA <- " + val + " 완료";
    }

    // ── FULLREAD ──────────────────────────────────────────────
    if (cmd == "fullread") {
        auto data = readNand();
        std::ostringstream out;
        out << "[FULLREAD] 전체 LBA 출력:\n";
        for (int i = 0; i < LBA_COUNT; i++) {
            out << "  LBA[" << std::setw(2) << i << "] = " << data[i];
            if (i % 4 == 3) out << "\n";
            else            out << "  ";
        }
        if (LBA_COUNT % 4 != 0) out << "\n";
        return out.str();
    }

    // ── HELP ─────────────────────────────────────────────────
    if (cmd == "help") {
        return
            "=== Virtual SSD Commands ===\n"
            "  read <lba>           : LBA 값 읽기 (0~99)\n"
            "  write <lba> <0x...>  : LBA에 값 쓰기\n"
            "  erase <lba> <size>   : 연속 LBA 초기화 (최대 10)\n"
            "  fullwrite <0x...>    : 전체 LBA에 동일 값 쓰기\n"
            "  fullread             : 전체 LBA 출력\n"
            "  help                 : 도움말\n"
            "  exit                 : 종료";
    }

    if (cmd == "exit")
        return "EXIT";

    return "ERROR: 알 수 없는 명령어 '" + cmd
         + "'. 'help' 를 입력하세요.";
}

// ─── 메인: TCP 서버 루프 ──────────────────────────────────────
int main() {
    try {
        initNand();

        boost::asio::io_context io;
        tcp::acceptor acceptor(io, tcp::endpoint(tcp::v4(), 12345));
        std::cout << "[SSD Server] 포트 12345에서 대기 중...\n";

        while (true) {
            tcp::socket socket(io);
            acceptor.accept(socket);
            std::cout << "[SSD Server] Shell 연결됨\n";

            while (true) {
                char buf[2048] = {};
                boost::system::error_code ec;
                size_t len = socket.read_some(boost::asio::buffer(buf), ec);

                if (ec == boost::asio::error::eof) break;
                else if (ec) throw boost::system::system_error(ec);

                std::string cmdLine(buf, len);
                // 개행 제거
                while (!cmdLine.empty() &&
                       (cmdLine.back() == '\n' || cmdLine.back() == '\r'))
                    cmdLine.pop_back();

                std::cout << "[CMD] " << cmdLine << "\n";
                std::string result = processCommand(cmdLine);

                boost::asio::write(socket,
                    boost::asio::buffer(result + "\n"));

                if (result == "EXIT") break;
            }

            socket.close();
            std::cout << "[SSD Server] Shell 연결 해제\n";
        }
    }
    catch (std::exception& e) {
        std::cerr << "[ERROR] " << e.what() << std::endl;
    }
    return 0;
}
