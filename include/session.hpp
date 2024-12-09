#pragma once
#include "player.hpp"
#include <string>
#include <vector>

enum SessionState {
    WaitPlayers = 1,
    Running = 2,
    Ended = 3,
};

class Session {
  public:
    std::string sessionName;
    int _sz;
    unsigned int cntOfPlayers;
    int curPlayerIndex = 0;
    std::vector<Player> playerList;
    std::string hiddenNum;
    SessionState state = SessionState::WaitPlayers;
    void print() {
        std::cout << "=========================================\n";
        if (state == SessionState::WaitPlayers) {
            std::cout << "   SESSION INFORMATION (WAITING PLAYERS) \n";
        } else if (state == SessionState::Running) {
            std::cout << "        SESSION INFORMATION (RUNNING)    \n";
        } else if (state == SessionState::Ended) {
            std::cout << "        SESSION INFORMATION (ENDED)      \n";
        }
        std::cout << "=========================================\n";
        std::cout << "| Name of session : " << sessionName << "\n";
        std::cout << "| Players count   : " << cntOfPlayers << "\n";
        std::cout << "| Current turn    : Player " << curPlayerIndex << "\n";
        std::cout << "| Hidden number   : " << hiddenNum << "\n";
        std::cout << "-----------------------------------------\n";
        std::cout << "| Players List:                                 \n";

        for (size_t i = 0; i < playerList.size(); ++i) {
            std::cout << "-----------------------------------------\n";
            std::cout << "| Player " << (i + 1) << ":\n";
            std::cout << playerList[i];
        }
        std::cout << "=========================================\n\n\n";
    }

    Session();
    ~Session();
};

Session::Session() {}

Session::~Session() {}
