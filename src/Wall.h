#ifndef WALL_H
#define WALL_H

#include <SFML/Graphics.hpp>

class Wall {
public:
    Wall(float x, float y, float width, float height, const sf::Color& color);

    const sf::RectangleShape& getShape() const;

private:
    sf::RectangleShape wallShape;
};

#endif // WALL_H