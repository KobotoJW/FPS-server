#include "Player.h"
#include <iostream>

const std::string CAP_POST = "5t0p";

Player::Player() : playerShape(sf::Vector2f(50, 50)), playerVelocity(5.0f) {
    playerShape.setFillColor(sf::Color::Green);
    playerShape.setPosition(400, 300);
}

void Player::move(float offsetX, float offsetY) {
    playerShape.move(offsetX * playerVelocity, offsetY * playerVelocity);
}

const sf::RectangleShape& Player::getShape() const {
    return playerShape;
}

void Player::shoot(float velocityX, float velocityY, sf::TcpSocket& socket) {
    if (shootClock.getElapsedTime().asSeconds() >= shootCooldown){
        Bullet bullet(playerShape.getPosition().x + playerShape.getSize().x / 2,
                      playerShape.getPosition().y + playerShape.getSize().y / 2,
                      velocityX, velocityY);
        bullet.sendBulletToServer(socket);
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
        {"position", {playerShape.getPosition().x, playerShape.getPosition().y}}
    };
}

void Player::sendPlayerToServer(sf::TcpSocket& socket) {
    if (socket.getRemoteAddress() == sf::IpAddress::None) {
        std::cout << "Not connected to server in send player" << std::endl;
        return;
    }
    else{
        nlohmann::json playerJson = toJson();
        std::string playerJsonMsg = playerJson.dump() + CAP_POST;
        if (socket.send(playerJsonMsg.c_str(), playerJsonMsg.size()) != sf::Socket::Done) {
            std::cout << "Error sending json player data to server" << std::endl;
            return;
        }
    }
}