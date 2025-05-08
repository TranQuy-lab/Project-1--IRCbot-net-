#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>

const std::string SERVER = "irc.df.lth.se";
const int PORT = 6667;
const std::string NICK = "CppBot";
const std::string USER = "USER CppBot 8 * :I'm a C++ IRC bot";
const std::string CHANNEL = "#my_channel";

int sockfd;

void sendLine(const std::string& line) {
    std::string data = line + "\r\n";
    send(sockfd, data.c_str(), data.length(), 0);
}

void pingSender() {
    while (true) {
        sendLine("PING :" + SERVER);
        std::this_thread::sleep_for(std::chrono::seconds(15));
    }
}

int main() {
    struct hostent* host = gethostbyname(SERVER.c_str());
    if (!host) {
        std::cerr << "Failed to resolve hostname.\n";
        return 1;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    std::memcpy(&serverAddr.sin_addr, host->h_addr, host->h_length);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (connect(sockfd, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Connection failed.\n";
        return 1;
    }

    sendLine(USER);
    sendLine("NICK " + NICK);
    sendLine("JOIN " + CHANNEL);

    std::thread pingThread(pingSender);
    pingThread.detach();

    char buffer[512];
    while (true) {
        std::memset(buffer, 0, sizeof(buffer));
        int bytesReceived = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived <= 0) break;

        std::string msg(buffer);
        std::cout << msg;

        // Respond to PING from server
        if (msg.find("PING :") != std::string::npos) {
            std::string response = "PONG :" + msg.substr(msg.find(":") + 1);
            sendLine(response);
        }

        // Welcome users who join the channel
        if (msg.find("JOIN :" + CHANNEL) != std::string::npos) {
            size_t start = msg.find(":") + 1;
            size_t end = msg.find("!");
            std::string nickname = msg.substr(start, end - start);
            sendLine("NOTICE " + nickname + " :Hi " + nickname + ", welcome to " + CHANNEL + "!");
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    }

    close(sockfd);
    return 0;
}