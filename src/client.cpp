#include "tools.hpp"

std::string randomNumber() {
    static std::vector<int> v = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    static std::random_device rd;
    static std::mt19937 generator(rd());
    std::shuffle(v.begin(), v.end(), generator);
    std::ostringstream oss;
    std::copy(v.begin(), v.begin() + 4, std::ostream_iterator<int>(oss, ""));
    return oss.str();
}

std::string randomId() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 9);

    std::string id;
    for (int i = 0; i < 10; ++i) {
        id += std::to_string(dis(gen));
    }

    return id;
}

void endSession(const std::string &sessionName) {
    int state2 = 0;
    nlohmann::json createRequest;
    createRequest["type"] = "end";
    createRequest["sessionName"] = sessionName;
    sem_t *mainSem = sem_open(MAIN_SEM_NAME.c_str(), O_CREAT, ACCESS_PERM, 0);
    int state = 0;
    while (state != 1) {
        sem_getvalue(mainSem, &state);
    }
    write_to(MAIN_FILE_NAME, createRequest);
    sem_wait(mainSem);
    sem_getvalue(mainSem, &state);
    while (state != 1) {
        sem_getvalue(mainSem, &state);
    }
    nlohmann::json reply = read_from(MAIN_FILE_NAME);
    if (reply["check"] == "ok") {
        std::cout << "\n[Success] Successfully exited session \"" << sessionName
                  << "\"!" << std::endl;
        sem_wait(mainSem);
        return;
    } else {
        std::cout << "\n[Error] Failed to exit session \"" << sessionName
                  << "\"." << std::endl;
        sem_wait(mainSem);
        return;
    }
}

void launchSession(const std::string &playerName,
                   const std::string &sessionName, int state, int cnt) {
    std::string apiSemName = sessionName + "api.semaphore";
    sem_t *apiSem = sem_open(apiSemName.c_str(), O_CREAT, ACCESS_PERM, 0);
    sem_setvalue(apiSem, cnt);
    int apiState = 0;
    sem_getvalue(apiSem, &apiState);

    if (state > 1) {
        while (apiState != state) {
            sem_getvalue(apiSem, &apiState);
        }
    }
    sem_getvalue(apiSem, &apiState);
    sem_close(apiSem);

    std::string gameSemName = sessionName + "game.semaphore";
    sem_t *gameSem;
    if (state == 1) {
        sem_unlink(gameSemName.c_str());
        gameSem = sem_open(gameSemName.c_str(), O_CREAT, ACCESS_PERM, 0);
    } else {
        gameSem = sem_open(gameSemName.c_str(), O_CREAT, ACCESS_PERM, 0);
    }

    sem_getvalue(gameSem, &apiState);
    int firstIt = 1, cntOfBulls = 0, cntOfCows = 0, flag = 1;

    std::cout << "\n[Info] Welcome to the session \"" << sessionName << "\"!\n"
              << "[Info] Game started. You are player \"" << playerName
              << "\".\n";

    while (flag) {
        sem_getvalue(gameSem, &apiState);
        if (apiState % (cnt + 1) == state) {
            nlohmann::json request = read_from(sessionName + "game.back");

            std::string ansField = playerName + "ans";
            std::string bullsField = playerName + "bulls";
            std::string cowsField = playerName + "cows";

            std::cout << "\n[Info] Player Statistics for \"" << playerName
                      << "\":\n";
            std::cout << "    - Last Answer: " << request[ansField] << "\n";
            std::cout << "    - Bulls Count: " << request[bullsField] << "\n";
            std::cout << "    - Cows Count:  " << request[cowsField] << "\n";

            if (request.contains("winner")) {
                std::cout << "\n[Game Over] Winner: \"" << request["winner"]
                          << "\"!\n"
                          << "[Info] Exiting session \"" << sessionName
                          << "\".\n";
                sem_post(gameSem);
                flag = 0;
                endSession(sessionName);
                break;
            }

            std::string answer;
            bool validAnswer = false;

            while (!validAnswer) {
                std::cout << "\n[Input Required] Enter a 4-digit number with "
                             "unique digits: ";
                std::cin >> answer;

                if (answer.length() == 4) {
                    std::set<char> uniqueDigits(answer.begin(), answer.end());
                    if (uniqueDigits.size() == 4) {
                        validAnswer = true;
                    } else {
                        std::cout << "[Error] Invalid input: Digits must be "
                                     "unique. Try again.\n";
                    }
                } else {
                    std::cout << "[Error] Invalid input: Number must have "
                                 "exactly 4 digits. Try again.\n";
                }
            }

            request[ansField] = answer;
            write_to(sessionName + "game.back", request);
            sem_post(gameSem);
        }
    }
}

bool createSession(const std::string &playerName,
                   const std::string &sessionName, const int cntOfPlayers) {
    int state2 = 0;
    nlohmann::json createRequest;
    createRequest["type"] = "create";
    createRequest["name"] = playerName;
    createRequest["bulls"] = 0;
    createRequest["cows"] = 0;
    createRequest["ans"] = "0000";
    createRequest["sessionName"] = sessionName;
    createRequest["cntOfPlayers"] = cntOfPlayers;
    createRequest["hiddenNum"] = randomNumber();

    sem_t *mainSem = sem_open(MAIN_SEM_NAME.c_str(), O_CREAT, ACCESS_PERM, 0);
    int state = 0;

    while (state != 1) {
        sem_getvalue(mainSem, &state);
    }
    write_to(MAIN_FILE_NAME, createRequest);
    sem_wait(mainSem);

    sem_getvalue(mainSem, &state);
    while (state != 1) {
        sem_getvalue(mainSem, &state);
    }

    nlohmann::json reply = read_from(MAIN_FILE_NAME);

    if (reply["check"] == "ok") {
        std::cout << "\n[Success] Session \"" << sessionName
                  << "\" created successfully!" << std::endl;
        state2 = reply["state"];
        sem_wait(mainSem);
        return true;
    } else {
        std::cout << "\n[Error] Failed to create session \"" << sessionName
                  << "\"." << " Session already exists." << std::endl;
        sem_wait(mainSem);
        return false;
    }
}

bool joinSession(const std::string &playerName,
                 const std::string &sessionName) {
    nlohmann::json joinRequest;
    joinRequest["type"] = "join";
    joinRequest["name"] = playerName;
    joinRequest["bulls"] = 0;
    joinRequest["cows"] = 0;
    joinRequest["ans"] = "0000";
    joinRequest["sessionName"] = sessionName;
    sem_t *mainSem = sem_open(MAIN_SEM_NAME.c_str(), O_CREAT, ACCESS_PERM, 0);
    int state = 0;
    while (state != 1) {
        sem_getvalue(mainSem, &state);
    }
    write_to(MAIN_FILE_NAME, joinRequest);
    sem_wait(mainSem);
    sem_getvalue(mainSem, &state);
    while (state != 1) {
        sem_getvalue(mainSem, &state);
    }
    nlohmann::json reply = read_from(MAIN_FILE_NAME);
    int state2 = 0, cnt = 0;
    if (reply["check"] == "ok") {
        std::cout << "\n[Success] Successfully joined session \"" << sessionName
                  << "\"!" << std::endl;
        state2 = reply["state"];
        cnt = reply["cnt"];
    } else {
        std::cout << "\n[Error] Failed to join session \"" << sessionName
                  << "\"." << std::endl;
        return false;
    }
    sem_wait(mainSem);
    if (reply["check"] == "ok") {
        launchSession(playerName, sessionName, state2, cnt);
    }
    return true;
}

std::string findSession(const std::string &playerName) {
    nlohmann::json findRequest;
    findRequest["type"] = "find";
    findRequest["name"] = playerName;
    findRequest["bulls"] = 0;
    findRequest["cows"] = 0;
    findRequest["ans"] = "0000";
    sem_t *mainSem = sem_open(MAIN_SEM_NAME.c_str(), O_CREAT, ACCESS_PERM, 0);
    int state = 0;
    while (state != 1) {
        sem_getvalue(mainSem, &state);
    }
    write_to(MAIN_FILE_NAME, findRequest);
    sem_wait(mainSem);
    sem_getvalue(mainSem, &state);
    while (state != 1) {
        sem_getvalue(mainSem, &state);
    }
    nlohmann::json reply = read_from(MAIN_FILE_NAME);
    int state2 = 0, cnt = 0;
    std::string sessionName;
    if (reply["check"] == "ok") {
        std::string sessionName = reply["sessionName"];
        state2 = reply["state"];
        cnt = reply["cnt"];
    } else {
        std::cout << "\n[Error] Unable to find a suitable session to join."
                  << std::endl;
    }
    sem_wait(mainSem);
    if (reply["check"] == "ok") {
        return reply["sessionName"];
    } else {
        return "";
    }
}

void printWelcome() {
    std::cout << "------------------------------------\n";
    std::cout << "          Welcome to the Game       \n";
    std::cout << "------------------------------------\n";
}

void printHints(const std::string &playerName) {
    std::cout << "Your player info: \n";
    std::cout << "                 name: " + playerName + "\n\n";
    std::cout << "Available Commands:\n";
    std::cout << "------------------------------------\n";
    std::cout << "  * **create** [name] [maxPlayers]\n";
    std::cout << "    - Create a new game session with the given name and "
                 "maximum number of players.\n";
    std::cout << "    - Example: create MySession 4\n";
    std::cout << "\n";
    std::cout << "  * **join** [name]\n";
    std::cout << "    - Join an existing game session by its name.\n";
    std::cout << "    - Example: join MySession\n";
    std::cout << "\n";
    std::cout << "  * **find**\n";
    std::cout
        << "    - Automatically find and join an available game session.\n";

    std::cout << "\n";
    std::cout << "  * **exit**\n";
    std::cout << "    - Exit game client.\n";
    std::cout << "------------------------------------\n";
}

bool createHandler(const std::string &playerName) {
    std::string name;
    int cntOfPlayers;
    std::cin >> name >> cntOfPlayers;

    if (cntOfPlayers < 2) {
        std::cout << "[Error] Number of players must be greater than 1.\n";
        return false;
    }

    if (createSession(playerName, name, cntOfPlayers)) {
        return joinSession(playerName, name);
    }

    return false;
}

bool joinHandler(const std::string &playerName) {
    std::string name;
    std::cin >> name;
    return joinSession(playerName, name);
}

bool findHandler(const std::string &playerName) {
    std::string sessionName = findSession(playerName);
    if (sessionName != "") {
        return joinSession(playerName, sessionName);
    }
    return false;
}

int main(int argc, char const *argv[]) {
    printWelcome();
    std::cout << "Input your name: ";

    std::string playerName;
    std::cin >> playerName;
    std::cout << std::endl;
    playerName += ", id: " + randomId();

    printHints(playerName);

    std::string command;

    while (true) {

        std::cout << "\nEnter command: ";
        std::cin >> command;

        if (command == "create") {

            createHandler(playerName);

        } else if (command == "join") {

            joinHandler(playerName);

        } else if (command == "find") {

            findHandler(playerName);

        } else if (command == "exit") {

            break;

        } else {
            std::cout << "[Error] Unknown command. Please try again.\n";
        }
    }
    return 0;
}
