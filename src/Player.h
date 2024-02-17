#ifndef PLAYER_H
#define PLAYER_H

#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <nlohmann/json.hpp>
#include "Wall.h"
#include "Bullet.h"

class Player {
public:
    Player();

    mutable sf::Text idText;
    mutable sf::Text healthText;
    sf::Font font;

    void setPlayerIdText();
    void move(float offsetX, float offsetY);
    const sf::RectangleShape& getShape() const;
    sf::Text& getIdText();
    void shoot(float velocityX, float velocityY, sf::TcpSocket& socket);
    bool checkCollision(const Wall& wall) const;
    sf::Vector2f getPosition();
    void setPosition(sf::Vector2f position);
    void setFillColor(sf::Color color);
    void restartShootClock();
    void setPlayerId(int id);
    int getPlayerId();
    void setPlayerHealth(int health);
    void decreasePlayerHealth(int damage);
    int getPlayerHealth();
    void setPlayerHealthText();
    sf::Text& getHealthText();
    bool checkIfHitByBullet(const Bullet& bullet);
    void checkIfDead();
    void setPlayerAlive(bool alive);
    bool getPlayerAlive();


    void getPlayerIdFromServer(sf::TcpSocket& socket);
    nlohmann::json toJson();
    void sendPlayerToServer(sf::TcpSocket& socket);


private:
    sf::RectangleShape playerShape;
    float playerVelocity;
    int playerId;
    int playerHealth;
    bool playerAlive;

    sf::Clock shootClock;
    float shootCooldown = 0.5f;
};

#endif // PLAYER_H