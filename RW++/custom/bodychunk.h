#pragma once

#include "geometry.h"
#include <glm/glm.hpp>
#include <cmath>
#include "custom.h"

class BodyChunk {
public:
    BodyChunk(glm::vec2 position, float Mass, float radius, float Friction, float Bounce, RoomGeometry& room)
        : geo(&room) {
        pos = lastPos = lastLastPos = position;
        mass = Mass;
        //doUpdate = true;
        //collideWithTerrain = true;
        rad = radius;
        rad_2 = rad*rad;
        friction = Friction;
        bounce = Bounce;
        vel = glm::vec2{0, 0};
        IsOnSolid = false;
        printf("Created a bodychunk with radius = %f\n", rad);
    }

    void draw_ui(const glm::vec2 screenOffset, const float image_height) {
        // IMGUI CONTROLS
        ImGui::Begin("Collider Controls");

        // Draw line to the collider on-screen
        const auto dl = ImGui::GetBackgroundDrawList();
        const auto pos2 = screenOffset + pos;
        dl->AddLine(ImGui::GetWindowPos(), ImVec2(pos2.x, image_height - pos2.y), ImGui::GetColorU32(ImVec4(1, 1, 1, 1)), 2);

        ImGui::DragFloat("Mass", &mass, 1, -2, 2);
        ImGui::DragFloat("Radius", &rad, 1, -2, 2);
        rad_2 = rad*rad;
        ImGui::DragFloat("Friction", &friction, 1, -2, 2);
        ImGui::DragFloat("Bounce", &bounce, 1, -2, 2);

        ImGui::Text("VEL: %f, %f", vel.x, vel.y);

        ImGui::End();
    }

    //it might be more accurate to name velocity "dPos", but this works fine too
    void Update(glm::vec2 g) {
        if(std::isinf(vel.x))
        {
            vel.x = 0;
        }
        if(std::isinf(vel.y))
        {
            vel.y = 0;
        }

        vel += g;
        vel *= (1 - friction);

        lastLastPos = lastPos;
        lastPos = pos;
        pos += vel;

        CheckVerticalCollision();
        CheckHorizontalCollision();
    }

    glm::vec2 getPosition() const {
        return pos;
    }

    void setPosition(const glm::vec2 new_pos) {
        pos = new_pos;
    }

    void setVelocity(const glm::vec2 Vel) {
        vel = Vel;
    }

    void addVelocity(const glm::vec2 addVel) {
        vel += addVel;
    }

    inline bool isOnSolid(){
        return IsOnSolid;
    }

private:
    int const MaxRepeats = 1000;

    RoomGeometry* geo;

    glm::vec2 pos;
    glm::vec2 lastPos;
    glm::vec2 lastLastPos;
    glm::vec2 vel;

    float rad;
    float rad_2;
    //float slopeRad;

    float mass;
    float friction;
    float bounce;

    //bool doUpdate;
    //bool collideWithTerrain;
    //bool goThroughFloors;
    //bool collideWithSlopes;
    //bool grabsDisableFloors;

    bool IsOnSolid;

    void CheckHorizontalCollision() {

        glm::ivec2 tilePos = custom::getTilePos(lastPos);
        int n = 0;

        int x, x2;
        int y, y2;

        bool Cease = false;

        //we create a range of tiles to iterate over

        if (vel.x>0.f)
        {
            x   = custom::getTilePos(pos.x + rad + 10.01f);
            x2  = custom::getTilePos(lastPos.x + rad + 10.0f);
            y   = custom::getTilePos(pos.y + rad - 1.f);
            y2  = custom::getTilePos(pos.y - rad + 1.f);
            for(int i=x2; i<=x; i++)
            {
                if (Cease) break;

                for(int j=y2; j<=y; j++)
                {
                    if (Cease) break;
                    // we check if we are inside of a solid, and check if there is air left of us, this way, we know if we have collided!
                    //its rather shrimple
                    if((geo->getTileType(i, j)) && (!(geo->getTileType(i-1, j))) && ((tilePos.x < i) || (geo->getTileType(lastPos))))
                    {
                        pos.x = static_cast<float>(i) * 20.0f - rad - 10.0f;
                        vel.x = (0.f - std::abs(vel.x) * bounce);
                        if (std::abs(vel.x) < 1.f + 9.f * (1.f - bounce))
                        {
                            vel.x = 0;
                        }
                        vel.y *= glm::clamp(friction*2.f, 0.f, 1.f);
                        Cease = true;
                    }
                    n++;
                    if (n > MaxRepeats)
                    {
                        Cease = true;
                        printf("Bodychunk breakout detected HORIZONTAL Y is above 0 \n");
                    }
                }
            }
        } else if (vel.x<0.f)
        {
            x   = custom::getTilePos(pos.x - rad - 0.01f + 10.0f);
            x2  = custom::getTilePos(lastPos.x - rad);
            y   = custom::getTilePos(pos.y + rad - 1.f);
            y2  = custom::getTilePos(pos.y - rad + 1);
            for(int i = x2; i >= x; i--)
            {
                if(Cease) break;

                for(int j = y2; j <= y; y++)
                {
                    if(Cease) break;

                    if((geo->getTileType(i, j)) && (!(geo->getTileType(i+1, j))) && ((tilePos.x > i) || (geo->getTileType(lastPos))))
                    {
                        pos.x = static_cast<float>(i) * 20.0f + rad + 10.0f;
                        vel.x = (0.f - std::abs(vel.x) * bounce);
                        if (std::abs(vel.x) < 1.f + 9.f* (1.f - bounce))
                        {
                            vel.x = 0;
                        }
                        vel.y *= glm::clamp(friction*2.f, 0.f, 1.f);
                        Cease = true;
                    }
                    n++;
                    if (n > MaxRepeats)
                    {
                        Cease = true;
                        printf("Bodychunk breakout detected HORIZONTAL Y is below 0 \n");
                    }
                }
            }
        }
    }

    void CheckVerticalCollision() {
        IsOnSolid = false;
        glm::ivec2 tilePos = custom::getTilePos(lastPos);
        int n = 0;

        int x, x2;
        int y, y2;

        bool Cease = false;

        if (vel.y > 0.f)
        {
            y   = custom::getTilePos(pos.y + rad + 0.01f);
            y2  = custom::getTilePos(lastPos.y + rad);
            x   = custom::getTilePos(pos.x - rad + 1.f);
            x2  = custom::getTilePos(pos.x + rad - 1.f);
            for (int i = y2; i <= y; i++)
            {
                if(Cease) break;

                for (int j = x; j <= x2; j++)
                {
                    if(Cease) break;

                    if((geo->getTileType(j, i)) && (!(geo->getTileType(j, i-1))) && ((tilePos.y < i) || (geo->getTileType(lastPos))))
                    {
                        pos.y = static_cast<float>(i) * 20.0f - rad;
                        vel.y = (0.f - std::abs(vel.y) * bounce);
                        if (std::abs(vel.y) < 1.f + 9.f * (1.f - bounce))
                        {
                            vel.y = 0.f;
                        }
                        vel.x *= glm::clamp(friction*2.f, 0.f, 1.f);
                        Cease = true;
                        IsOnSolid = true;
                    }
                    n++;
                    if (n > MaxRepeats)
                    {
                        Cease = true;
                        printf("Bodychunk breakout detected VERTICAL Y IS ABOVE 0 \n");
                    }
                }
            }
        }else if (vel.y < 0.f)
        {
            //printf("pos.x: %f, pos.y: %f, lastPos.y: %f rad: %f\n", pos.x, pos.y, lastPos.y, rad);

            y   = custom::getTilePos(pos.y - rad - 0.01f);
            y2  = custom::getTilePos(lastPos.y - rad);
            x   = custom::getTilePos(pos.x - rad + 1.f);
            x2  = custom::getTilePos(pos.x + rad - 1.f);

            //printf("y: %d, y2: %d, x: %d, x2: %d\n", y, y2, x, x2);

            for (int i = y2; i >= y; i--)
            {
                if(Cease) break;

                for (int j = x; j <= x2; j++)
                {
                    if(Cease) break;

                    if((geo->getTileType(j, i)) && (!(geo->getTileType(j, i + 1))) && ((tilePos.y > i) || (geo->getTileType(lastPos))))
                    {
                        //printf("Collision detected at x:%d y:%d\n", j, i);
                        //printf("%d\n", geo->getTileType(j, i + 1));
                        //printf("%d\n", geo->getTileType(j, i));
                        //printf("%d > %d, %d\n", tilePos.y, i, geo->getTileType(lastPos));
                        pos.y = (static_cast<float>(i) + 1.f) * 20.0f + rad;
                        vel.y = std::abs(vel.y) * bounce;
                        if (vel.y < 1.f + 9.f * (1.f - bounce))
                        {
                            vel.y = 0;
                        }
                        vel.x *= glm::clamp(friction*2.f, 0.f, 1.f);
                        Cease = true;
                        IsOnSolid = isOnSolid();
                    }
                    n++;
                    if (n > MaxRepeats)
                    {
                        Cease = true;
                        printf("Bodychunk breakout detected VERTICAL Y is below 0\n");
                    }
                }
            }
        }
    }

    /*
     * Sorry, no slopes in the demo
     *void CheckVerticalCollisionSlopes(RoomGeometry& room, glm::vec2& newPos){
     *}
     */
};
