#include "Wall.h"

Wall::Wall(float x, float y, float width, float height, const sf::Color& color) : wallShape(sf::Vector2f(width, height)) {
    wallShape.setPosition(x, y);
    wallShape.setFillColor(color);
}

const sf::RectangleShape& Wall::getShape() const {
    return wallShape;
}