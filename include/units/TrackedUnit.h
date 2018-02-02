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

#ifndef TRACKEDUNIT_H
#define TRACKEDUNIT_H

#include <units/GroundUnit.h>

class TrackedUnit : public GroundUnit
{
public:

    explicit TrackedUnit(House* newOwner);
    explicit TrackedUnit(InputStream& stream);
    void init();
    virtual ~TrackedUnit();

    TrackedUnit(const TrackedUnit &) = delete;
    TrackedUnit(TrackedUnit &&) = delete;
    TrackedUnit& operator=(const TrackedUnit &) = delete;
    TrackedUnit& operator=(TrackedUnit &&) = delete;

    void save(OutputStream& stream) const override;

    void checkPos() override;
    bool canPassTile(const Tile* pTile) const override;

    /**
        Returns how fast a unit can move over the specified terrain type.
        \param  terrainType the type to consider
        \return Returns a speed factor. Higher values mean slower.
    */
    FixPoint getTerrainDifficulty(TERRAINTYPE terrainType) const override
    {
        static const FixPoint difficulty[] = {
            FixPt(1,0), //Terrain_Slab,
            FixPt(1,5625), //Terrain_Sand,
            FixPt(1,375), //Terrain_Rock,
            FixPt(1,375), //Terrain_Dunes,
            FixPt(1,0), //Terrain_Mountain,
            FixPt(1,375), //Terrain_Spice,
            FixPt(1,375), //Terrain_ThickSpice,
            FixPt(1,5625), //Terrain_SpiceBloom,
            FixPt(1,5625) //Terrain_SpecialBloom
        };

        if (terrainType < 0 || terrainType > Terrain_SpecialBloom)
            return FixPt(0, 0);

        return difficulty[terrainType];

#if 0
        switch(terrainType) {
            case Terrain_Slab:          return FixPt(1,0);
            case Terrain_Sand:          return FixPt(1,5625);
            case Terrain_Rock:          return FixPt(1,375);
            case Terrain_Dunes:         return FixPt(1,375);
            case Terrain_Mountain:      return FixPt(1,0);
            case Terrain_Spice:         return FixPt(1,375);
            case Terrain_ThickSpice:    return FixPt(1,375);
            case Terrain_SpiceBloom:    return FixPt(1,5625);
            case Terrain_SpecialBloom:  return FixPt(1,5625);
            default:                    return FixPt(1,0);
        }
#endif // 0
        }
};

#endif // TRACKEDUNIT_H
