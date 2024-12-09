#include "tools.hpp"

Session session;
std::map<char, int> hidV;

std::pair<int, int> game(std::string ans) {
    int cntOfBulls = 0, cntOfCows = 0, ind = 0;
    for (int i = 0; i < ans.size(); i++) {
        if (hidV.find(ans[i]) != hidV.cend()) {
            if (hidV[ans[i]] == i) {
                cntOfBulls++;
            } else {
                cntOfCows++;
            }
        }
    }
    return std::make_pair(cntOfBulls, cntOfCows);
}

void launchServer(std::string &sessionName) {
    std::string tmp = session.hiddenNum;
    for (int i = 0; i < tmp.size(); i++) {
        hidV.insert({tmp[i], i});
    }
    std::string gameSemName = sessionName + "game.semaphore";
    sem_t *gameSem = sem_open(gameSemName.c_str(), O_CREAT, ACCESS_PERM, 0);
    sem_setvalue(gameSem, 0);
    int state = 0, firstIt = 1;
    while (1) {
        sem_getvalue(gameSem, &state);
        if (state % (session.cntOfPlayers + 1) == 0 && firstIt == 0) {
            nlohmann::json request = read_from(sessionName + "game.back");
            for (auto &player : session.playerList) {
                std::string ansField = player.name + "ans";
                std::string bullsField = player.name + "bulls";
                std::string cowsField = player.name + "cows";

                player.ans = request[ansField];
                std::pair<int, int> result = game(player.ans);
                player.bulls = result.first;
                player.cows = result.second;

                request[ansField] = player.ans;
                request[bullsField] = player.bulls;
                request[cowsField] = player.cows;

                if (player.bulls == 4) {
                    request["winner"] = player.name;
                    session.state = SessionState::Ended;
                }
            }

            write_to(sessionName + "game.back", request);
            session.print();
            sem_post(gameSem);

        } else if (state % (session.cntOfPlayers + 1) == 0 && firstIt == 1) {
            nlohmann::json request;
            for (const auto &player : session.playerList) {
                std::string ansField = player.name + "ans";
                std::string bullsField = player.name + "bulls";
                std::string cowsField = player.name + "cows";
                request[ansField] = player.ans;
                request[bullsField] = player.bulls;
                request[cowsField] = player.cows;
            }
            write_to(sessionName + "game.back", request);
            firstIt = 0;
            session.print();
            sem_post(gameSem);
        }
    }
}

void waitPlayers(std::string &sessionName) {
    session.curPlayerIndex = 0;
    std::string joinSemName = (sessionName + ".semaphore");

    sem_t *joinSem = sem_open(joinSemName.c_str(), O_CREAT, ACCESS_PERM, 0);
    int state = 0;

    sem_getvalue(joinSem, &state);
    std::string joinFdName;
    while (state < session.cntOfPlayers) {
        sem_getvalue(joinSem, &state);
        if (state > session.curPlayerIndex) {
            joinFdName = sessionName;
            nlohmann::json joinReply = read_from(joinFdName);
            Player playerN;
            playerN.name = joinReply["name"];
            playerN.ans = joinReply["ans"];
            playerN.bulls = joinReply["bulls"];
            playerN.cows = joinReply["cows"];
            session.playerList.push_back(playerN);
            session.curPlayerIndex = state;
        }
    }
}

int main(int argc, char const *argv[]) {
    std::string strToJson = argv[1];
    nlohmann::json reply;
    reply = nlohmann::json::parse(strToJson);
    session.sessionName = reply["sessionName"];
    session.cntOfPlayers = reply["cntOfPlayers"];
    session.hiddenNum = reply["hiddenNum"];
    session.curPlayerIndex = 0;

    std::string apiSemName = session.sessionName + "api.semaphore";
    sem_unlink(apiSemName.c_str());
    sem_t *apiSem = sem_open(apiSemName.c_str(), O_CREAT, ACCESS_PERM, 0);
    sem_setvalue(apiSem, session.cntOfPlayers);
    int f = 0;
    sem_getvalue(apiSem, &f);

    waitPlayers(session.sessionName);
    session.state = SessionState::Running;

    while (f > 0) {
        sem_wait(apiSem);
        sem_getvalue(apiSem, &f);
        std::cout << "Loading session players, left: " << f + 1 << "."
                  << std::endl;
        sleep(1);
    }
    sem_getvalue(apiSem, &f);
    launchServer(session.sessionName);
    return 0;
}
