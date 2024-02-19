#include "Player.h"
#include <iostream>

const std::string CAP_POST = "5t0p";

Player::Player() : playerShape(sf::Vector2f(50, 50)), playerVelocity(5.0f), playerId(0), playerHealth(100), playerAlive(true){
    playerShape.setFillColor(sf::Color::Green);
    playerShape.setPosition(400, 300);
}

void Player::setPlayerHealth(int health) {
    playerHealth = health;
}

int Player::getPlayerHealth() {
    return playerHealth;
}

void Player::decreasePlayerHealth(int damage) {
    playerHealth -= damage;
}

void Player::setPlayerId(int id) {
    playerId = id;
}

int Player::getPlayerId() {
    return playerId;
}

void Player::getPlayerIdFromServer(sf::TcpSocket& socket) {
    if (socket.getRemoteAddress() == sf::IpAddress::None) {
        std::cout << "Not connected to server in get player id" << std::endl;
        return;
    }
    else{
        size_t sent = 0;
        char buffer[1024];
        std::string playerIdMsg;
        playerIdMsg = "Gimmie dat tasty tasty id" + CAP_POST;
        if (socket.send(playerIdMsg.c_str(), playerIdMsg.size(), sent) != sf::Socket::Done) {
            std::cout << "Error sending json player data to server" << std::endl;
            return;
        }
        size_t received = 0;
        if (socket.receive(buffer, sizeof(buffer), received) != sf::Socket::Done) {
            std::cout << "Error receiving json player data from server" << std::endl;
            return;
        }
        std::vector<std::string> dataV;
        std::string dataString = buffer;
        size_t start = 0;
        size_t end = 0;

        //std::cout << "Received data from server pre decap: " << dataString << std::endl;

        while ((end = dataString.find(CAP_POST, start)) != std::string::npos) {
            std::string data = dataString.substr(start, end - start);
            start = end + CAP_POST.size();
            dataV.push_back(data);
        }

        for (std::string d : dataV) {
            //std::cout << "Received data from server cap: " << d << std::endl;
            nlohmann::json j = nlohmann::json::parse(d);
            // Player Id
            if (j.contains("type") && j["type"] == "playerId") {
                std::cout << "Received player id from server" << std::endl;
                setPlayerId(j["id"]);
                setPlayerIdText();
                setPlayerHealthText();
            }
        }
        

    }
}

void Player::move(float offsetX, float offsetY) {
    playerShape.move(offsetX * playerVelocity, offsetY * playerVelocity);
}

const sf::RectangleShape& Player::getShape() const {
    return playerShape;
}

void Player::setPlayerHealthText() {
    font.loadFromFile("../resources/Arial.ttf");
    healthText.setFont(font);
    healthText.setCharacterSize(32);
    healthText.setFillColor(sf::Color::Black);
    healthText.setString("HP: " + std::to_string(playerHealth));
    healthText.setPosition(10,10);
}

sf::Text& Player::getHealthText() {
    healthText.setString("HP: " + std::to_string(playerHealth));
    return healthText;
}

void Player::setPlayerIdText() {
    font.loadFromFile("../resources/Arial.ttf");
    idText.setFont(font);
    idText.setCharacterSize(24);
    idText.setFillColor(sf::Color::Black);
    idText.setString(std::to_string(playerId));
    idText.setPosition(playerShape.getPosition().x + playerShape.getSize().x / 2 - idText.getGlobalBounds().width / 2,
                      playerShape.getPosition().y + playerShape.getSize().y / 2 - idText.getGlobalBounds().height / 2);
}

sf::Text& Player::getIdText() {
    idText.setPosition(playerShape.getPosition().x + playerShape.getSize().x / 2 - idText.getGlobalBounds().width / 2,
                      playerShape.getPosition().y + playerShape.getSize().y / 2 - idText.getGlobalBounds().height / 2);
    return idText;
}

void Player::shoot(float velocityX, float velocityY, sf::TcpSocket& socket) {
    if (shootClock.getElapsedTime().asSeconds() >= shootCooldown){
        Bullet b(playerShape.getPosition().x + playerShape.getSize().x / 2,
                      playerShape.getPosition().y + playerShape.getSize().y / 2,
                      velocityX, velocityY, getPlayerId());
        b.sendBulletToServer(socket);
        shootClock.restart();
    }
}

bool Player::checkCollision(const Wall& wall) const {
    return playerShape.getGlobalBounds().intersects(wall.getShape().getGlobalBounds());
}

sf::Vector2f Player::getPosition() {
    return playerShape.getPosition();
}

void Player::setPosition(sf::Vector2f position) {
    playerShape.setPosition(position);
}

void Player::setFillColor(sf::Color color) {
    playerShape.setFillColor(color);
}

void Player::restartShootClock() {
    shootClock.restart();
}

nlohmann::json Player::toJson() {
    return {
        {"type", "player"},
        {"position", {playerShape.getPosition().x, playerShape.getPosition().y}},
        {"id", playerId},
        {"health", playerHealth},
        {"alive", playerAlive}
    };
}

void Player::sendPlayerToServer(sf::TcpSocket& socket) {
    if (socket.getRemoteAddress() == sf::IpAddress::None) {
        std::cout << "Not connected to server in send player" << std::endl;
        return;
    }
    else{
        size_t sent = 0;
        nlohmann::json playerJson = toJson();
        std::string playerJsonMsg = playerJson.dump() + CAP_POST;
        if (socket.send(playerJsonMsg.c_str(), playerJsonMsg.size(), sent) != sf::Socket::Done) {
            std::cout << "Error sending json player data to server" << std::endl;
            return;
        }
    }
}

void Player::setPlayerAlive(bool alive) {
    playerAlive = alive;
}

bool Player::getPlayerAlive() {
    return playerAlive;
}

bool Player::checkIfHitByBullet(const Bullet& bullet) {
    if (playerShape.getGlobalBounds().intersects(bullet.getShape().getGlobalBounds()) && bullet.getOwner() != playerId){
        std::cout << "Got hit by bullet from player: " << bullet.getOwner() << std::endl;
        return true;
    }
    return false;
}

void Player::checkIfDead() {
    if (playerHealth <= 0) {
        setPlayerAlive(false);
    }
}