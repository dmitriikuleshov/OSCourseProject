#pragma once
#include <iostream>
#include <string>

class Player {
  public:
    std::string name;
    int bulls;
    int cows;
    std::string ans;
    bool operator<(const Player &x);
    friend std::ostream &operator<<(std::ostream &cout, const Player &p) {
        cout << "+----------------------------+\n";
        cout << "|          PLAYER            |\n";
        cout << "+----------------------------+\n";
        cout << "| Name   : " << p.name << "\n";
        cout << "| Bulls  : " << p.bulls << "\n";
        cout << "| Cows   : " << p.cows << "\n";
        cout << "| Answer : " << p.ans << "\n";
        cout << "+----------------------------+\n";
        return cout;
    }
    Player();
    ~Player();
};

bool Player::operator<(const Player &x) {
    if (this->bulls > x.bulls) {
        return true;
    }
    return this->cows > x.cows;
}

Player::Player() {}

Player::~Player() {}
