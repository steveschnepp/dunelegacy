/*
 *  This file is part of Dune Legacy.
 *
 *  Dune Legacy is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Dune Legacy is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Dune Legacy.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <mmath.h>

#include <Definitions.h>

#include <cstdarg>
#include <algorithm>
#include <limits>

#include "misc/RngSupport.h"

extern int currentZoomlevel;


int getRandomInt(int min, int max)
{
    return RandomUniformInt(min, max);
}


int getRandomOf(int numParam, ...) {
    const int nthParam = getRandomInt(0,numParam-1);

    va_list arg_ptr;
    va_start(arg_ptr, numParam);

    auto ret = va_arg(arg_ptr, int);

    for(auto i = 1; i <= nthParam; i++) {
        ret = va_arg(arg_ptr, int);
    }
    va_end(arg_ptr);

    return ret;
}

FixPoint destinationAngleRad(const Coord& p1, const Coord& p2)
{
    return destinationAngleRad(p1.x, p1.y, p2.x, p2.y);
}

FixPoint destinationAngleRad(FixPoint x1, FixPoint y1, FixPoint x2, FixPoint y2)
{

    const FixPoint diffX = x2 - x1;
    const FixPoint diffY = -(y2 - y1);    // flip y

    if(diffX == 0 && diffY == 0) {
        return FixPt_PI/2;
    }

    auto destAngle = FixPoint::atan2(diffY, diffX);

    if(destAngle < 0) {
        destAngle += (FixPt_PI << 1);   // add 360°
    }

    return destAngle;
}


FixPoint distanceFrom(const Coord& p1, const Coord& p2)
{
    const FixPoint first = (p1.x - p2.x);
    const FixPoint second = (p1.y - p2.y);

    const FixPoint z = FixPoint::sqrt(first*first + second*second);

    return z;
}

FixPoint distanceFrom(FixPoint x, FixPoint y, FixPoint to_x, FixPoint to_y)
{
    const FixPoint first = (x - to_x);
    const FixPoint second = (y - to_y);

   const  FixPoint z = FixPoint::sqrt(first*first + second*second);

    return z;
}

int mirrorAngleHorizontal(int angle) {
    switch(angle % NUM_ANGLES) {
        case RIGHT:     return LEFT;
        case RIGHTUP:   return LEFTUP;
        case UP:        return UP;
        case LEFTUP:    return RIGHTUP;
        case LEFT:      return RIGHT;
        case LEFTDOWN:  return RIGHTDOWN;
        case DOWN:      return DOWN;
        case RIGHTDOWN: return LEFTDOWN;
        default:        return angle;
    }
}



int mirrorAngleVertical(int angle) {
    switch(angle % NUM_ANGLES) {
        case RIGHT:     return RIGHT;
        case RIGHTUP:   return RIGHTDOWN;
        case UP:        return DOWN;
        case LEFTUP:    return LEFTDOWN;
        case LEFT:      return LEFT;
        case LEFTDOWN:  return LEFTUP;
        case DOWN:      return UP;
        case RIGHTDOWN: return RIGHTUP;
        default:        return angle;
    }
}


int world2zoomedWorld(int x) {
    if(x<0) {
        switch(currentZoomlevel) {
            case 0:     return (x-3)/4;
            case 1:     return (x-1)/2;
            case 2:     return ((x-1)*3)/4;
            case 3:
            default:    return x;
        }
    } else {
        switch(currentZoomlevel) {
            case 0:     return x/4;
            case 1:     return x/2;
            case 2:     return (x*3)/4;
            case 3:
            default:    return x;
        }
    }
}


int world2zoomedWorld(float x) {
    switch(currentZoomlevel) {
        case 0:     return lround(x*0.25f);
        case 1:     return lround(x*0.5f);
        case 2:     return lround(x*0.75f);
        case 3:
        default:    return lround(x);
    }
}


Coord world2zoomedWorld(const Coord& coord) {
    return Coord(world2zoomedWorld(coord.x), world2zoomedWorld(coord.y));
}


int zoomedWorld2world(int x) {
    switch(currentZoomlevel) {
        case 0:     return x*4;
        case 1:     return x*2;
        case 2:     return (x*4)/3;
        case 3:
        default:    return x;
    }
}


Coord zoomedWorld2world(const Coord& coord) {
    return Coord(zoomedWorld2world(coord.x), zoomedWorld2world(coord.y));
}
