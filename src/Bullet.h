#ifndef BULLET_H
#define BULLET_H

#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <nlohmann/json.hpp>
#include "Wall.h"

class Bullet {
public:
    Bullet(float x, float y, float velocityX, float velocityY);

    void move();
    const sf::CircleShape& getShape() const;
    bool checkCollision(const Wall& wall) const;

    nlohmann::json toJson();
    void sendBulletToServer(sf::TcpSocket& socket);

private:
    sf::CircleShape bulletShape;
    sf::Vector2f bulletVelocity;
};

#endif // BULLET_H