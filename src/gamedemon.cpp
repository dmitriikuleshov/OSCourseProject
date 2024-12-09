#include "tools.hpp"

std::map<std::string, Session> sessions;

Player createPlayer(nlohmann::json &createReply) {
    Player player;
    player.ans = createReply["ans"];
    player.bulls = createReply["bulls"];
    player.cows = createReply["cows"];
    player.name = createReply["name"];
    return player;
}

void addNewSession(nlohmann::json &createReply) {
    Player player = createPlayer(createReply);
    Session session;
    session.sessionName = createReply["sessionName"];
    session.cntOfPlayers = createReply["cntOfPlayers"];
    session.hiddenNum = createReply["hiddenNum"];
    session.playerList.push_back(player);
    sessions.insert({session.sessionName, session});
}

void setRequestOK(nlohmann::json &request, nlohmann::json &createReply) {
    request["check"] = "ok";
    request["state"] =
        sessions[createReply["sessionName"]].playerList.size() - 1;
    request["cnt"] = sessions[createReply["sessionName"]].cntOfPlayers;
}

void findCommand(nlohmann::json &request, nlohmann::json &createReply) {
    for (auto i : sessions) {
        if (i.second.playerList.size() <= i.second.cntOfPlayers) {
            request["sessionName"] = i.second.sessionName;
            createReply["sessionName"] = i.second.sessionName;
        }
    }
    if (!(request.contains("sessionName"))) {
        request["check"] = "error";
    }
    if (!request.contains("check") || request["check"] != "error") {
        setRequestOK(request, createReply);
    }
}

void joinCommand(nlohmann::json &request, nlohmann::json &createReply) {

    std::string sessionName = createReply["sessionName"];
    if (sessions.find(sessionName) == sessions.end()) {
        request["check"] = "error";
        return;
    }
    if (sessions[sessionName].playerList.size() <=
        sessions[sessionName].cntOfPlayers) {
        request["sessionName"] = sessionName;
        createReply["sessionName"] = sessionName;
    } else {
        request["check"] = "error";
        return;
    }
    if (!(request.contains("sessionName"))) {
        request["check"] = "error";
        return;
    }
    if (!request.contains("check") || request["check"] != "error") {
        std::string joinSemName = createReply["sessionName"];
        joinSemName += ".semaphore";
        sem_t *joinSem = sem_open(joinSemName.c_str(), O_CREAT, ACCESS_PERM, 0);
        std::string joinFdName = createReply["sessionName"];
        Player player = createPlayer(createReply);
        sessions[createReply["sessionName"]].playerList.push_back(player);
        write_to(joinFdName, createReply);
        sem_post(joinSem);
        setRequestOK(request, createReply);
    }
}

void createCommand(nlohmann::json &request, nlohmann::json &createReply,
                   sem_t *mainSem) {
    if (sessions.find(createReply["sessionName"]) == sessions.cend()) {
        std::string joinSemName = createReply["sessionName"];
        joinSemName += ".semaphore";
        sem_unlink(joinSemName.c_str());
        sem_t *joinSem = sem_open(joinSemName.c_str(), O_CREAT, ACCESS_PERM, 0);
        sem_setvalue(joinSem, 0);

        addNewSession(createReply);
        for (auto elem : sessions) {
            elem.second.print();
        }
        request["check"] = "ok";
        request["state"] = 0;
        pid_t serverPid = fork();
        if (serverPid == 0) {
            sem_close(mainSem);
            std::string strToJson = createReply.dump();
            execl("./server", "./server", strToJson.c_str(), NULL);
        }
    } else {
        request["check"] = "error";
    }
}

void endCommand(nlohmann::json &request, nlohmann::json &createReply) {
    if (!createReply.contains("sessionName")) {
        request["check"] = "error";
        return;
    }

    std::string sessionName = createReply["sessionName"];
    if (sessions.find(sessionName) != sessions.end()) {
        sessions.erase(sessionName);
    }
    request["check"] = "ok";
}

int main(int argc, char const *argv[]) {
    sem_unlink(MAIN_SEM_NAME.c_str());
    sem_t *mainSem = sem_open(MAIN_SEM_NAME.c_str(), O_CREAT, ACCESS_PERM, 0);
    int state = 0;
    sem_setvalue(mainSem, 1);
    sem_getvalue(mainSem, &state);
    while (true) {

        sem_getvalue(mainSem, &state);
        if (state != 0) {
            continue;
        }

        nlohmann::json createReply = read_from(MAIN_FILE_NAME);
        nlohmann::json request;

        if (!createReply.contains("type")) {
            sem_post(mainSem);
            continue;
        }

        if (createReply["type"] == "create") {
            createCommand(request, createReply, mainSem);
        } else if (createReply["type"] == "join") {
            joinCommand(request, createReply);
        } else if (createReply["type"] == "find") {
            findCommand(request, createReply);
        } else if (createReply["type"] == "end") {
            endCommand(request, createReply);
        }

        write_to(MAIN_FILE_NAME, request);
        sem_post(mainSem);
    }
    return 0;
}
