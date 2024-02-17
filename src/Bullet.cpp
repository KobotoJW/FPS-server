#include "Bullet.h"
#include <iostream>

const std::string CAP_POST = "5t0p";

Bullet::Bullet(float x, float y, float velocityX, float velocityY, int owner) : bulletShape(5), bulletVelocity(velocityX, velocityY) {
    bulletShape.setFillColor(sf::Color::Blue);
    bulletShape.setPosition(x, y);
    this->owner = owner;
}

void Bullet::move() {
    bulletShape.move(bulletVelocity);
}

int Bullet::getOwner() const {
    return owner;
}

const sf::CircleShape& Bullet::getShape() const {
    return bulletShape;
}

bool Bullet::checkCollision(const Wall& wall) const {
    return bulletShape.getGlobalBounds().intersects(wall.getShape().getGlobalBounds());
}

nlohmann::json Bullet::toJson() {
    return {
        {"type", "bullet"},
        {"position", {bulletShape.getPosition().x, bulletShape.getPosition().y}},
        {"velocity", {bulletVelocity.x, bulletVelocity.y}},
        {"owner", owner}
    };
}

void Bullet::sendBulletToServer(sf::TcpSocket& socket) {
    if (socket.getRemoteAddress() == sf::IpAddress::None) {
        std::cout << "Not connected to server in send bullet" << std::endl;
        return;
    }
    else{
        size_t sent = 0;
        nlohmann::json bulletJson = toJson();
        std::string bulletJsonMsg = bulletJson.dump() + CAP_POST;
        //std::cout << "Sending bullet with owner: " << owner << std::endl;
        if (socket.send(bulletJsonMsg.c_str(), bulletJsonMsg.size(), sent) != sf::Socket::Done) {
            std::cout << "Error sending json bullet data to server" << std::endl;
            return;
        }
    }
}