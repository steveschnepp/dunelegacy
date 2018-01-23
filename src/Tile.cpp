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

#include <Tile.h>

#include <globals.h>

#include <FileClasses/GFXManager.h>

#include <sand.h>
#include <Game.h>
#include <Map.h>
#include <House.h>
#include <SoundPlayer.h>
#include <ScreenBorder.h>
#include <ConcatIterator.h>
#include <Explosion.h>

#include <structures/StructureBase.h>
#include <units/InfantryBase.h>
#include <units/AirUnit.h>

Tile::Tile() {
    type = Terrain_Sand;

    for (auto i = 0; i < NUM_TEAMS; i++) {
        explored[i] = currentGame->getGameInitSettings().getGameOptions().startWithExploredMap;
        lastAccess[i] = 0;
    }

    fogColor = COLOR_BLACK;

    owner = INVALID;
    sandRegion = NONE_ID;

    spice = 0;

    sprite = pGFXManager->getObjPic(ObjPic_Terrain);

    for (auto& time : tracksCreationTime) {
        time = 0;
    }

    location.x = 0;
    location.y = 0;

    destroyedStructureTile = DestroyedStructure_None;
}


Tile::~Tile() = default;

void Tile::load(InputStream& stream) {
    type = stream.readUint32();

    stream.readBools(&explored[0], &explored[1], &explored[2], &explored[3], &explored[4], &explored[5], &explored[6]);

    bool bLastAccess[NUM_TEAMS];
    stream.readBools(&bLastAccess[0], &bLastAccess[1], &bLastAccess[2], &bLastAccess[3], &bLastAccess[4], &bLastAccess[5], &bLastAccess[6]);

    for (int i = 0; i < NUM_TEAMS; i++) {
        if (bLastAccess[i] == true) {
            lastAccess[i] = stream.readUint32();
        }
    }

    fogColor = stream.readUint32();

    owner = stream.readSint32();
    sandRegion = stream.readUint32();

    spice = stream.readFixPoint();

    bool bHasDamage, bHasDeadUnits, bHasAirUnits, bHasInfantry, bHasUndergroundUnits, bHasNonInfantryGroundObjects;
    stream.readBools(&bHasDamage, &bHasDeadUnits, &bHasAirUnits, &bHasInfantry, &bHasUndergroundUnits, &bHasNonInfantryGroundObjects);

    if (bHasDamage) {
        damage.clear();
        const auto numDamage = stream.readUint32();
        damage.reserve(numDamage);
        for (auto i = decltype(numDamage){0}; i < numDamage; i++) {
            DAMAGETYPE newDamage;
            newDamage.damageType = stream.readUint32();
            newDamage.tile = stream.readSint32();
            newDamage.realPos.x = stream.readSint32();
            newDamage.realPos.y = stream.readSint32();

            damage.push_back(newDamage);
        }
    }

    if (bHasDeadUnits) {
        deadUnits.clear();
        const auto numDeadUnits = stream.readUint32();
        deadUnits.reserve(numDeadUnits);
        for (auto i = decltype(numDeadUnits){0}; i < numDeadUnits; i++) {
            DEADUNITTYPE newDeadUnit;
            newDeadUnit.type = stream.readUint8();
            newDeadUnit.house = stream.readUint8();
            newDeadUnit.onSand = stream.readBool();
            newDeadUnit.realPos.x = stream.readSint32();
            newDeadUnit.realPos.y = stream.readSint32();
            newDeadUnit.timer = stream.readSint16();

            deadUnits.push_back(newDeadUnit);
        }
    }

    destroyedStructureTile = stream.readSint32();

    bool bTrackCounter[NUM_ANGLES];
    stream.readBools(&bTrackCounter[0], &bTrackCounter[1], &bTrackCounter[2], &bTrackCounter[3], &bTrackCounter[4], &bTrackCounter[5], &bTrackCounter[6], &bTrackCounter[7]);

    for (int i = 0; i < NUM_ANGLES; i++) {
        if (bTrackCounter[i] == true) {
            tracksCreationTime[i] = stream.readUint32();
        }
    }

    if (bHasAirUnits) {
        stream.readUint32Vector(assignedAirUnitList);
    }

    if (bHasInfantry) {
        stream.readUint32Vector(assignedInfantryList);
    }

    if (bHasUndergroundUnits) {
        stream.readUint32Vector(assignedUndergroundUnitList);
    }

    if (bHasNonInfantryGroundObjects) {
        stream.readUint32Vector(assignedNonInfantryGroundObjectList);
    }
}

void Tile::save(OutputStream& stream) const {
    stream.writeUint32(type);

    stream.writeBools(explored[0], explored[1], explored[2], explored[3], explored[4], explored[5], explored[6]);

    stream.writeBools((lastAccess[0] != 0), (lastAccess[1] != 0), (lastAccess[2] != 0), (lastAccess[3] != 0), (lastAccess[4] != 0), (lastAccess[5] != 0), (lastAccess[6] != 0));
    for (auto lastAcces : lastAccess) {
        if (lastAcces != 0) {
            stream.writeUint32(lastAcces);
        }
    }

    stream.writeUint32(fogColor);

    stream.writeUint32(owner);
    stream.writeUint32(sandRegion);

    stream.writeFixPoint(spice);

    stream.writeBools(!damage.empty(), !deadUnits.empty(), !assignedAirUnitList.empty(),
        !assignedInfantryList.empty(), !assignedUndergroundUnitList.empty(), !assignedNonInfantryGroundObjectList.empty());

    if (!damage.empty()) {
        stream.writeUint32(damage.size());
        for (const auto& damageItem : damage) {
            stream.writeUint32(damageItem.damageType);
            stream.writeSint32(damageItem.tile);
            stream.writeSint32(damageItem.realPos.x);
            stream.writeSint32(damageItem.realPos.y);
        }
    }

    if (!deadUnits.empty()) {
        stream.writeUint32(deadUnits.size());
        for (const auto& deadUnit : deadUnits) {
            stream.writeUint8(deadUnit.type);
            stream.writeUint8(deadUnit.house);
            stream.writeBool(deadUnit.onSand);
            stream.writeSint32(deadUnit.realPos.x);
            stream.writeSint32(deadUnit.realPos.y);
            stream.writeSint16(deadUnit.timer);
        }
    }

    stream.writeSint32(destroyedStructureTile);

    // clean-up tracksCreationTime to save space in the save game
    Uint32 tracksCreationTimeToSave[NUM_ANGLES];
    for (int i = 0; i < NUM_ANGLES; i++) {
        tracksCreationTimeToSave[i] = (tracksCreationTime[i] + TRACKSTIME < currentGame->getGameCycleCount()) ? 0 : tracksCreationTime[i];
    }

    stream.writeBools((tracksCreationTimeToSave[0] != 0), (tracksCreationTimeToSave[1] != 0), (tracksCreationTimeToSave[2] != 0), (tracksCreationTimeToSave[3] != 0),
        (tracksCreationTimeToSave[4] != 0), (tracksCreationTimeToSave[5] != 0), (tracksCreationTimeToSave[6] != 0), (tracksCreationTimeToSave[7] != 0));
    for (auto i : tracksCreationTimeToSave) {
        if (i != 0) {
            stream.writeUint32(i);
        }
    }

    if (!assignedAirUnitList.empty()) {
        stream.writeUint32Vector(assignedAirUnitList);
    }

    if (!assignedInfantryList.empty()) {
        stream.writeUint32Vector(assignedInfantryList);
    }

    if (!assignedUndergroundUnitList.empty()) {
        stream.writeUint32Vector(assignedUndergroundUnitList);
    }

    if (!assignedNonInfantryGroundObjectList.empty()) {
        stream.writeUint32Vector(assignedNonInfantryGroundObjectList);
    }
}

void Tile::assignAirUnit(Uint32 newObjectID) {
    assignedAirUnitList.push_back(newObjectID);
}

void Tile::assignNonInfantryGroundObject(Uint32 newObjectID) {
    assignedNonInfantryGroundObjectList.push_back(newObjectID);
}

int Tile::assignInfantry(Uint32 newObjectID, Sint8 currentPosition) {
    Sint8 newPosition = currentPosition;

    if (currentPosition < 0) {
        bool used[NUM_INFANTRY_PER_TILE]{ false };

        for (auto objectID : assignedInfantryList) {
            const auto pInfantry = dynamic_cast<InfantryBase*>(currentGame->getObjectManager().getObject(objectID));
            if (pInfantry == nullptr) {
                continue;
            }

            const auto pos = pInfantry->getTilePosition();
            if ((pos >= 0) && (pos < NUM_INFANTRY_PER_TILE)) {
                used[pos] = true;
            }
        }

        for (newPosition = 0; newPosition < NUM_INFANTRY_PER_TILE; newPosition++) {
            if (used[newPosition] == false) {
                break;
            }
        }

        newPosition = std::max(static_cast<Sint8>(0), std::min(newPosition, static_cast<Sint8>(NUM_INFANTRY_PER_TILE)));
    }

    assignedInfantryList.push_back(newObjectID);
    return newPosition;
}


void Tile::assignUndergroundUnit(Uint32 newObjectID) {
    assignedUndergroundUnitList.push_back(newObjectID);
}

void Tile::blitGround(int xPos, int yPos) {
    if (hasANonInfantryGroundObject() && getNonInfantryGroundObject()->isAStructure())
        return;

    const auto tileIndex = getTerrainTile();
    const auto indexX = tileIndex % NUM_TERRAIN_TILES_X;
    const auto indexY = tileIndex / NUM_TERRAIN_TILES_X;
    const auto zoomed_tilesize = world2zoomedWorld(TILESIZE);
    SDL_Rect source = { indexX*zoomed_tilesize, indexY*zoomed_tilesize, zoomed_tilesize, zoomed_tilesize };
    SDL_Rect drawLocation = { xPos, yPos, zoomed_tilesize, zoomed_tilesize };

    //draw terrain
    if (destroyedStructureTile == DestroyedStructure_None || destroyedStructureTile == DestroyedStructure_Wall) {
        SDL_RenderCopy(renderer, sprite[currentZoomlevel], &source, &drawLocation);
    }

    if (destroyedStructureTile != DestroyedStructure_None) {
        const auto pDestroyedStructureSurface = pGFXManager->getObjPic(ObjPic_DestroyedStructure);
        SDL_Rect source2 = { destroyedStructureTile*zoomed_tilesize, 0, zoomed_tilesize, zoomed_tilesize };
        SDL_RenderCopy(renderer, pDestroyedStructureSurface[currentZoomlevel], &source2, &drawLocation);
    }

    if (isFogged(pLocalHouse->getHouseID()))
        return;

    // tracks
    const auto pTracks = pGFXManager->getObjPic(ObjPic_Terrain_Tracks)[currentZoomlevel];
    for (auto i = 0; i < NUM_ANGLES; i++) {
        const auto tracktime = static_cast<int>(currentGame->getGameCycleCount() - tracksCreationTime[i]);
        if ((tracksCreationTime[i] != 0) && (tracktime < TRACKSTIME)) {
            source.x = ((10 - i) % 8)*zoomed_tilesize;
            SDL_SetTextureAlphaMod(pTracks, std::min(255, 256 * (TRACKSTIME - tracktime) / TRACKSTIME));
            SDL_RenderCopy(renderer, pTracks, &source, &drawLocation);
        }
    }

    // damage
    for (const auto& damageItem : damage) {
        source.x = damageItem.tile*zoomed_tilesize;
        SDL_Rect dest = { screenborder->world2screenX(damageItem.realPos.x) - zoomed_tilesize / 2,
            screenborder->world2screenY(damageItem.realPos.y) - zoomed_tilesize / 2,
            zoomed_tilesize,
            zoomed_tilesize };

        if (damageItem.damageType == Terrain_RockDamage) {
            SDL_RenderCopy(renderer, pGFXManager->getObjPic(ObjPic_RockDamage)[currentZoomlevel], &source, &dest);
        }
        else {
            SDL_RenderCopy(renderer, pGFXManager->getObjPic(ObjPic_SandDamage)[currentZoomlevel], &source, &drawLocation);
        }
    }
}

void Tile::blitStructures(int xPos, int yPos) const {
    if (!hasANonInfantryGroundObject())
        return;

    const auto object = getNonInfantryGroundObject();
    if (!object->isAStructure())
        return;

    //if got a structure, draw the structure, and dont draw any terrain because wont be seen
    auto structure = dynamic_cast<StructureBase*>(object);

    assert(structure);  // Why not use static_cast<> if we "know" dynamic_cast will work?

    for (auto i = structure->getX(); i < structure->getX() + structure->getStructureSizeX(); i++) {
        for (auto j = structure->getY(); j < structure->getY() + structure->getStructureSizeY(); j++) {
            if (screenborder->isTileInsideScreen(Coord(i, j))
                && currentGameMap->tileExists(i, j) && (currentGameMap->getTile(i, j)->isExplored(pLocalHouse->getHouseID()) || debug))
            {
                structure->setFogged(isFogged(pLocalHouse->getHouseID()));

                if ((i == location.x) && (j == location.y)) {
                    //only this tile will draw it, so will be drawn only once
                    structure->blitToScreen();
                }

                return;
            }
        }
    }
}

void Tile::blitUndergroundUnits(int xPos, int yPos) const {
    if (!hasAnUndergroundUnit() || isFogged(pLocalHouse->getHouseID()))
        return;

    auto current = getUndergroundUnit();

    if (current->isVisible(pLocalHouse->getTeam())) {
        if (location == current->getLocation()) {
            current->blitToScreen();
        }
    }
}

void Tile::blitDeadUnits(int xPos, int yPos) {
    if (isFogged(pLocalHouse->getHouseID()))
        return;

    const auto zoomed_tile = world2zoomedWorld(TILESIZE);

    for (const auto& deadUnit : deadUnits) {
        SDL_Rect source = { 0, 0, zoomed_tile, zoomed_tile };
        SDL_Texture** pTexture = nullptr;
        switch (deadUnit.type) {
        case DeadUnit_Infantry: {
            pTexture = pGFXManager->getObjPic(ObjPic_DeadInfantry, deadUnit.house);
            source.x = (deadUnit.timer < 1000 && deadUnit.onSand) ? zoomed_tile : 0;
        } break;

        case DeadUnit_Infantry_Squashed1: {
            pTexture = pGFXManager->getObjPic(ObjPic_DeadInfantry, deadUnit.house);
            source.x = 4 * zoomed_tile;
        } break;

        case DeadUnit_Infantry_Squashed2: {
            pTexture = pGFXManager->getObjPic(ObjPic_DeadInfantry, deadUnit.house);
            source.x = 5 * zoomed_tile;
        } break;

        case DeadUnit_Carrall: {
            pTexture = pGFXManager->getObjPic(ObjPic_DeadAirUnit, deadUnit.house);
            if (deadUnit.onSand) {
                source.x = (deadUnit.timer < 1000) ? 5 * zoomed_tile : 4 * zoomed_tile;
            }
            else {
                source.x = 3 * zoomed_tile;
            }
        } break;

        case DeadUnit_Ornithopter: {
            pTexture = pGFXManager->getObjPic(ObjPic_DeadAirUnit, deadUnit.house);
            if (deadUnit.onSand) {
                source.x = (deadUnit.timer < 1000) ? 2 * zoomed_tile : zoomed_tile;
            }
            else {
                source.x = 0;
            }
        } break;

        default:
            break;
        }

        if (pTexture != nullptr) {
            SDL_Rect dest = { screenborder->world2screenX(deadUnit.realPos.x) - zoomed_tile / 2,
                screenborder->world2screenY(deadUnit.realPos.y) - zoomed_tile / 2,
                zoomed_tile,
                zoomed_tile };
            SDL_RenderCopy(renderer, pTexture[currentZoomlevel], &source, &dest);
        }
    }
}

void Tile::blitInfantry(int xPos, int yPos) {
    if (isFogged(pLocalHouse->getHouseID()))
        return;

    for (auto objectID : assignedInfantryList) {
        auto pInfantry = static_cast<InfantryBase*>(currentGame->getObjectManager().getObject(objectID));
        if (pInfantry == nullptr) {
            continue;
        }

        if (pInfantry->isVisible(pLocalHouse->getTeam())) {
            if (location == pInfantry->getLocation()) {
                pInfantry->blitToScreen();
            }
        }
    }
}

void Tile::blitNonInfantryGroundUnits(int xPos, int yPos) {
    if (isFogged(pLocalHouse->getHouseID()))
        return;

    for (auto objectID : assignedNonInfantryGroundObjectList) {
        auto pObject = currentGame->getObjectManager().getObject(objectID);

        if (pObject->isAUnit() && pObject->isVisible(pLocalHouse->getTeam())) {
            if (location == pObject->getLocation()) {
                pObject->blitToScreen();
            }
        }
    }
}


void Tile::blitAirUnits(int xPos, int yPos) {
    if (isFogged(pLocalHouse->getHouseID()))
        return;

    for (auto objectID : assignedAirUnitList) {
        auto pAirUnit = static_cast<AirUnit*>(currentGame->getObjectManager().getObject(objectID));
        if (pAirUnit == nullptr) {
            continue;
        }

        if (pAirUnit->getOwner() == pLocalHouse) {
            if (pAirUnit->isVisible(pLocalHouse->getTeam())) {
                if (location == pAirUnit->getLocation()) {
                    pAirUnit->blitToScreen();
                }
            }
        }
    }
}

void Tile::blitSelectionRects(int xPos, int yPos) const {
    if (isFogged(pLocalHouse->getHouseID()))
        return;

#if __cpp_coroutines
    // draw underground selection rectangles

    auto all = all_assigned();

    std::for_each(std::begin(all), std::end(all),
        [](Uint32 objectID) {
            auto pObject = currentGame->getObjectManager().getObject(objectID);
            if (pObject == nullptr) {
                return;
            }

            // possibly draw selection rectangle multiple times, e.g. for structures
            if (pObject->isVisible(pLocalHouse->getTeam())) {
                if (pObject->isSelected()) {
                    pObject->drawSelectionBox();
                }

                if (pObject->isSelectedByOtherPlayer()) {
                    pObject->drawOtherPlayerSelectionBox();
                }
            }
        });
#else
    const auto blitObjectSelectionRect =
        [&](Uint32 objectID) {
        auto pObject = currentGame->getObjectManager().getObject(objectID);
        if (pObject == nullptr) {
            return;
        }

        // possibly draw selection rectangle multiple times, e.g. for structures
        if (pObject->isVisible(pLocalHouse->getTeam())) {
            if (pObject->isSelected()) {
                pObject->drawSelectionBox();
            }

            if (pObject->isSelectedByOtherPlayer()) {
                pObject->drawOtherPlayerSelectionBox();
            }
        }
    };

    // draw underground selection rectangles

    std::for_each(assignedUndergroundUnitList.begin(),
        assignedUndergroundUnitList.end(),
        blitObjectSelectionRect);

    std::for_each(assignedInfantryList.begin(),
        assignedInfantryList.end(),
        blitObjectSelectionRect);

    std::for_each(assignedNonInfantryGroundObjectList.begin(),
        assignedNonInfantryGroundObjectList.end(),
        blitObjectSelectionRect);

    std::for_each(assignedAirUnitList.begin(),
        assignedAirUnitList.end(),
        blitObjectSelectionRect);
#endif
}

void Tile::update_impl()
{
    deadUnits.erase(
        std::remove_if(std::begin(deadUnits), std::end(deadUnits),
            [](DEADUNITTYPE& dut)
            {
                if (0 == dut.timer)
                    return true;
                --dut.timer;
                return false;
            }),
        std::end(deadUnits));
}


void Tile::clearTerrain() {
    damage.clear();
    deadUnits.clear();
}

void Tile::setTrack(Uint8 direction) {
    if (type == Terrain_Sand || type == Terrain_Dunes || type == Terrain_Spice || type == Terrain_ThickSpice) {
        tracksCreationTime[direction] = currentGame->getGameCycleCount();
    }
}

void Tile::selectAllPlayersUnits(int houseID, ObjectBase** lastCheckedObject, ObjectBase** lastSelectedObject) {
    selectFilter(houseID, lastCheckedObject, lastSelectedObject,
        [](ObjectBase* obj) { return  obj->isAUnit() && obj->isRespondable(); });
}


void Tile::selectAllPlayersUnitsOfType(int houseID, int itemID, ObjectBase** lastCheckedObject, ObjectBase** lastSelectedObject) {
    selectFilter(houseID, lastCheckedObject, lastSelectedObject,
        [=](ObjectBase* obj) { return  obj->getItemID() == itemID; });
}

template<typename Container, typename Val>
static void erase_remove(Container& c, Val val) {
    c.erase(std::remove(std::begin(c), std::end(c), val), std::end(c));
}

void Tile::unassignAirUnit(Uint32 objectID) {
    erase_remove(assignedAirUnitList, objectID);
}

void Tile::unassignNonInfantryGroundObject(Uint32 objectID) {
    erase_remove(assignedNonInfantryGroundObjectList, objectID);
}

void Tile::unassignUndergroundUnit(Uint32 objectID) {
    erase_remove(assignedUndergroundUnitList, objectID);
}

void Tile::unassignInfantry(Uint32 objectID, int currentPosition) {
    erase_remove(assignedInfantryList, objectID);
}

void Tile::unassignObject(Uint32 objectID) {
    if (hasInfantry()) unassignInfantry(objectID, -1);
    if (hasAnUndergroundUnit()) unassignUndergroundUnit(objectID);
    if (hasANonInfantryGroundObject()) unassignNonInfantryGroundObject(objectID);
    if (hasAnAirUnit()) unassignAirUnit(objectID);
}


void Tile::setType(int newType) {
    type = newType;
    destroyedStructureTile = DestroyedStructure_None;

    if (type == Terrain_Spice) {
        spice = currentGame->randomGen.rand(RANDOMSPICEMIN, RANDOMSPICEMAX);
    }
    else if (type == Terrain_ThickSpice) {
        spice = currentGame->randomGen.rand(RANDOMTHICKSPICEMIN, RANDOMTHICKSPICEMAX);
    }
    else if (type == Terrain_Dunes) {
    }
    else {
        spice = 0;

        if (isRock()) {
            std::vector<ObjectBase*> pending_destroy;

            sandRegion = NONE_ID;
            if (hasAnUndergroundUnit()) {
                pending_destroy.reserve(assignedUndergroundUnitList.size());

                auto units = std::move(assignedUndergroundUnitList);
                assignedUndergroundUnitList.clear();

                for (const auto object_id : units)
                {
                    const auto object = currentGame->getObjectManager().getObject(object_id);

                    if (object)
                        pending_destroy.push_back(object);
                    else
                        assignedUndergroundUnitList.push_back(object_id);
                }

                // Try to keep the largest buffer.
                if (assignedUndergroundUnitList.empty() && units.capacity() > assignedUndergroundUnitList.capacity())
                {
                    units.clear();
                    assignedUndergroundUnitList = std::move(units);
                }
            }

            if (type == Terrain_Mountain) {
                if (hasANonInfantryGroundObject()) {
                    pending_destroy.reserve(pending_destroy.size() + assignedNonInfantryGroundObjectList.size());

                    auto units = std::move(assignedNonInfantryGroundObjectList);
                    assignedNonInfantryGroundObjectList.clear();

                    for (const auto object_id : units)
                    {
                        const auto object = currentGame->getObjectManager().getObject(object_id);

                        if (object)
                            pending_destroy.push_back(object);
                        else
                            assignedNonInfantryGroundObjectList.push_back(object_id);
                    }

                    // Try to keep the largest buffer.
                    if (assignedNonInfantryGroundObjectList.empty() && units.capacity() > assignedNonInfantryGroundObjectList.capacity())
                    {
                        units.clear();
                        assignedNonInfantryGroundObjectList = std::move(units);
                    }
                }
            }

            std::for_each(std::begin(pending_destroy), std::end(pending_destroy), [](ObjectBase* obj) { obj->destroy(); });
        }
    }

    for (auto i = location.x; i <= location.x + 3; i++) {
        for (auto j = location.y; j <= location.y + 3; j++) {
            if (currentGameMap->tileExists(i, j)) {
                currentGameMap->getTile(i, j)->clearTerrain();
            }
        }
    }
}


void Tile::squash() const {
    if (!hasInfantry()) return;

    for (const auto object_id : assignedInfantryList)
    {
        auto current = dynamic_cast<InfantryBase*>(currentGame->getObjectManager().getObject(object_id));

        if (current == nullptr)
            continue;

        current->squash();
    }
}


int Tile::getInfantryTeam() const {
    int team = INVALID;
    if (hasInfantry())
        team = getInfantry()->getOwner()->getTeam();
    return team;
}


FixPoint Tile::harvestSpice() {
    const auto oldSpice = spice;

    if ((spice - HARVESTSPEED) >= 0) {
        spice -= HARVESTSPEED;
    }
    else {
        spice = 0;
    }

    if (oldSpice >= RANDOMTHICKSPICEMIN && spice < RANDOMTHICKSPICEMIN) {
        setType(Terrain_Spice);
    }

    if (oldSpice > 0 && spice == 0) {
        setType(Terrain_Sand);
    }

    return (oldSpice - spice);
}


void Tile::setSpice(FixPoint newSpice) {
    if (newSpice <= 0) {
        type = Terrain_Sand;
    }
    else if (newSpice >= RANDOMTHICKSPICEMIN) {
        type = Terrain_ThickSpice;
    }
    else {
        type = Terrain_Spice;
    }
    spice = newSpice;
}


AirUnit* Tile::getAirUnit() const {
    return dynamic_cast<AirUnit*>(currentGame->getObjectManager().getObject(assignedAirUnitList.front()));
}

ObjectBase* Tile::getGroundObject() const {
    if (hasANonInfantryGroundObject())
        return getNonInfantryGroundObject();
    else if (hasInfantry())
        return getInfantry();
    else
        return nullptr;
}

InfantryBase* Tile::getInfantry() const {
    return dynamic_cast<InfantryBase*>(currentGame->getObjectManager().getObject(assignedInfantryList.front()));
}

ObjectBase* Tile::getNonInfantryGroundObject() const {
    return currentGame->getObjectManager().getObject(assignedNonInfantryGroundObjectList.front());
}

UnitBase* Tile::getUndergroundUnit() const {
    return dynamic_cast<UnitBase*>(currentGame->getObjectManager().getObject(assignedUndergroundUnitList.front()));
}


/*ObjectBase* Tile::getInfantry(int i)
{
    int count;
    InfantryBase* infantry;
    assignedInfantry.reset();
    while (assignedInfantry.currentNotNull())
    {
        ((InfantryBase*)assignedInfantry.getCurrent())->squash();
        assignedInfantry.nextLink();
    }
    return assignedInfantry.removeElement();
}*/


ObjectBase* Tile::getObject() const {
    ObjectBase* temp = nullptr;
    if (hasAnAirUnit())
        temp = getAirUnit();
    else if (hasANonInfantryGroundObject())
        temp = getNonInfantryGroundObject();
    else if (hasInfantry())
        temp = getInfantry();
    else if (hasAnUndergroundUnit())
        temp = getUndergroundUnit();
    return temp;
}


ObjectBase* Tile::getObjectAt(int x, int y) const {
    ObjectBase* pObject = nullptr;
    if (hasAnAirUnit()) {
        pObject = getAirUnit();
    }
    else if (hasANonInfantryGroundObject()) {
        pObject = getNonInfantryGroundObject();
    }
    else if (hasInfantry()) {
        auto closestDistance = FixPt_MAX;
        const Coord atPos(x, y);

        for (const auto objectID : assignedInfantryList) {
            const auto pInfantry = dynamic_cast<InfantryBase*>(currentGame->getObjectManager().getObject(objectID));
            if (pInfantry == nullptr) {
                continue;
            }

            const auto centerPoint = pInfantry->getCenterPoint();
            const auto distance = distanceFrom(atPos, centerPoint);
            if (distance < closestDistance) {
                closestDistance = distance;
                pObject = pInfantry;
            }
        }
    }
    else if (hasAnUndergroundUnit()) {
        pObject = getUndergroundUnit();
    }

    return pObject;
}


ObjectBase* Tile::getObjectWithID(Uint32 objectID) const {
#ifdef __cpp_coroutines
    auto all = all_assigned();

    if (std::find(std::begin(all), std::end(all), objectID) == std::end(all))
        return nullptr;

    return currentGame->getObjectManager().getObject(objectID);
#else
    for (Uint32 curObjectID : assignedInfantryList) {
        if (curObjectID == objectID) {
            return currentGame->getObjectManager().getObject(curObjectID);
        }
    }

    for (Uint32 curObjectID : assignedNonInfantryGroundObjectList) {
        if (curObjectID == objectID) {
            return currentGame->getObjectManager().getObject(curObjectID);
        }
    }

    for (Uint32 curObjectID : assignedUndergroundUnitList) {
        if (curObjectID == objectID) {
            return currentGame->getObjectManager().getObject(curObjectID);
        }
    }
    for (Uint32 curObjectID : assignedAirUnitList) {
        if (curObjectID == objectID) {
            return currentGame->getObjectManager().getObject(curObjectID);
        }
    }

    return nullptr;
#endif
}

void Tile::triggerSpiceBloom(House* pTrigger) {
    if (!isSpiceBloom()) return;

    //a spice bloom
    soundPlayer->playSoundAt(Sound_Bloom, getLocation());
    screenborder->shakeScreen(18);
    if (pTrigger == pLocalHouse) {
        soundPlayer->playVoice(BloomLocated, pLocalHouse->getHouseID());
    }

    setType(Terrain_Spice); // Set this tile to spice first
    currentGameMap->createSpiceField(location, 5);

    const auto realLocation = location * TILESIZE + Coord(TILESIZE / 2, TILESIZE / 2);

    if (damage.size() < DAMAGE_PER_TILE) {
        DAMAGETYPE newDamage;
        newDamage.tile = SandDamage1;
        newDamage.damageType = Terrain_SandDamage;
        newDamage.realPos = realLocation;

        damage.push_back(newDamage);
    }

    currentGame->getExplosionList().push_back(new Explosion(Explosion_SpiceBloom, realLocation, pTrigger->getHouseID()));
}

void Tile::triggerSpecialBloom(House* pTrigger) {
    if (!isSpecialBloom()) return;

    setType(Terrain_Sand);

    switch (currentGame->randomGen.rand(0, 3)) {
    case 0: {
        // the player gets an randomly choosen amount of credits between 150 and 400
        pTrigger->addCredits(currentGame->randomGen.rand(150, 400), false);
    } break;

    case 1: {
        // The house gets a Trike for free. It spawns beside the special bloom.
        auto pNewUnit = pTrigger->createUnit(Unit_Trike);
        if (pNewUnit != nullptr) {
            const auto spot = currentGameMap->findDeploySpot(pNewUnit, location, currentGame->randomGen);
            pNewUnit->deploy(spot);
        }
    } break;

    case 2: {
        // One of the AI players on the map (one that has at least one unit) gets a Trike for free. It spawns beside the special bloom.
        int numCandidates = 0;
        for (int i = 0; i < NUM_HOUSES; i++) {
            const auto pHouse = currentGame->getHouse(i);
            if (pHouse != nullptr && pHouse->getTeam() != pTrigger->getTeam() && pHouse->getNumUnits() > 0) {
                numCandidates++;
            }
        }

        if (numCandidates == 0) {
            break;
        }

        int candidate = currentGame->randomGen.rand(0, numCandidates - 1);

        House* pEnemyHouse = nullptr;
        for (int i = 0; i < NUM_HOUSES; i++) {
            const auto pHouse = currentGame->getHouse(i);
            if (pHouse != nullptr && pHouse->getTeam() != pTrigger->getTeam() && pHouse->getNumUnits() > 0) {
                if (candidate == 0) {
                    pEnemyHouse = pHouse;
                    break;
                }
                candidate--;
            }
        }

        const auto pNewUnit = pEnemyHouse->createUnit(Unit_Trike);
        if (pNewUnit != nullptr) {
            const auto spot = currentGameMap->findDeploySpot(pNewUnit, location, currentGame->randomGen);
            pNewUnit->deploy(spot);
        }

    } break;

    case 3:
    default: {
        // One of the AI players on the map (one that has at least one unit) gets an Infantry unit (3 Soldiers) for free. The spawn beside the special bloom.
        int numCandidates = 0;
        for (int i = 0; i < NUM_HOUSES; i++) {
            const auto pHouse = currentGame->getHouse(i);
            if (pHouse != nullptr && pHouse->getTeam() != pTrigger->getTeam() && pHouse->getNumUnits() > 0) {
                numCandidates++;
            }
        }

        if (numCandidates == 0) {
            break;
        }

        int candidate = currentGame->randomGen.rand(0, numCandidates - 1);

        House* pEnemyHouse = nullptr;
        for (int i = 0; i < NUM_HOUSES; i++) {
            const auto pHouse = currentGame->getHouse(i);
            if (pHouse != nullptr && pHouse->getTeam() != pTrigger->getTeam() && pHouse->getNumUnits() > 0) {
                if (candidate == 0) {
                    pEnemyHouse = pHouse;
                    break;
                }
                candidate--;
            }
        }

        for (int i = 0; i < 3; i++) {
            const auto pNewUnit = pEnemyHouse->createUnit(Unit_Soldier);
            if (pNewUnit != nullptr) {
                const auto spot = currentGameMap->findDeploySpot(pNewUnit, location, currentGame->randomGen);
                pNewUnit->deploy(spot);
            }
        }
    } break;
    }
}

bool Tile::hasAStructure() const {
    if (!hasANonInfantryGroundObject()) {
        return false;
    }

    const auto pObject = currentGame->getObjectManager().getObject(assignedNonInfantryGroundObjectList.front());
    return ((pObject != nullptr) && pObject->isAStructure());
}

bool Tile::isFogged(int houseID) const noexcept {
    if (debug)
        return false;

    if (currentGame->getGameInitSettings().getGameOptions().fogOfWar == false) {
        return false;
    }

    return (currentGame->getGameCycleCount() - lastAccess[houseID]) >= MILLI2CYCLES(10 * 1000);
}

Uint32 Tile::getRadarColor(House* pHouse, bool radar) {
    if (!isExplored(pHouse->getHouseID()) && !debug) {
        return COLOR_BLACK;
    }

    if (isFogged(pHouse->getHouseID()) && radar) {
        return fogColor;
    }

    auto pObject = getObject();
    if (pObject != nullptr) {
        Uint32 color;

        if (pObject->getItemID() == Unit_Sandworm) {
            color = COLOR_WHITE;
        }
        else {
            switch (pObject->getOwner()->getHouseID()) {
            case HOUSE_HARKONNEN:   color = SDL2RGB(palette[PALCOLOR_HARKONNEN]);  break;
            case HOUSE_ATREIDES:    color = SDL2RGB(palette[PALCOLOR_ATREIDES]);   break;
            case HOUSE_ORDOS:       color = SDL2RGB(palette[PALCOLOR_ORDOS]);      break;
            case HOUSE_FREMEN:      color = SDL2RGB(palette[PALCOLOR_FREMEN]);     break;
            case HOUSE_SARDAUKAR:   color = SDL2RGB(palette[PALCOLOR_SARDAUKAR]);  break;
            case HOUSE_MERCENARY:   color = SDL2RGB(palette[PALCOLOR_MERCENARY]);  break;
            default:                color = COLOR_BLACK;                           break;
            }
        }

        if (pObject->isAUnit()) {
            fogColor = getColorByTerrainType(getType());
        }
        else {
            fogColor = color;
        }

        // units and structures of the enemy are not visible if no radar
        if (!radar && !debug && (pObject->getOwner()->getTeam() != pHouse->getTeam())) {
            return COLOR_BLACK;
        }

        return color;
    }

    fogColor = getColorByTerrainType(getType());

    if (!radar && !debug) {
        return COLOR_BLACK;
    }

    return fogColor;
}

int Tile::getTerrainTile() const {
    auto terrainType = type;
    if (terrainType == Terrain_ThickSpice) {
        // check if we are surrounded by spice/thick spice
        bool up = (currentGameMap->tileExists(location.x, location.y - 1) == false) || (currentGameMap->getTile(location.x, location.y - 1)->isSpice() == true);
        bool right = (currentGameMap->tileExists(location.x + 1, location.y) == false) || (currentGameMap->getTile(location.x + 1, location.y)->isSpice() == true);
        bool down = (currentGameMap->tileExists(location.x, location.y + 1) == false) || (currentGameMap->getTile(location.x, location.y + 1)->isSpice() == true);
        bool left = (currentGameMap->tileExists(location.x - 1, location.y) == false) || (currentGameMap->getTile(location.x - 1, location.y)->isSpice() == true);

        if (!up || !right || !down || !left) {
            // to avoid graphical glitches (there is no tile for thick spice next to a non-spice tile) we draw this tile as normal spice
            terrainType = Terrain_Spice;
        }
    }

    switch (terrainType) {
    case Terrain_Slab: {
        return TerrainTile_Slab;
    } break;

    case Terrain_Sand: {
        return TerrainTile_Sand;
    } break;

    case Terrain_Rock: {
        // determine which surrounding tiles are rock
        bool up = (currentGameMap->tileExists(location.x, location.y - 1) == false) || (currentGameMap->getTile(location.x, location.y - 1)->isRock() == true);
        bool right = (currentGameMap->tileExists(location.x + 1, location.y) == false) || (currentGameMap->getTile(location.x + 1, location.y)->isRock() == true);
        bool down = (currentGameMap->tileExists(location.x, location.y + 1) == false) || (currentGameMap->getTile(location.x, location.y + 1)->isRock() == true);
        bool left = (currentGameMap->tileExists(location.x - 1, location.y) == false) || (currentGameMap->getTile(location.x - 1, location.y)->isRock() == true);

        return TerrainTile_Rock + (((int)up) | (right << 1) | (down << 2) | (left << 3));
    } break;

    case Terrain_Dunes: {
        // determine which surrounding tiles are dunes
        bool up = (currentGameMap->tileExists(location.x, location.y - 1) == false) || (currentGameMap->getTile(location.x, location.y - 1)->getType() == Terrain_Dunes);
        bool right = (currentGameMap->tileExists(location.x + 1, location.y) == false) || (currentGameMap->getTile(location.x + 1, location.y)->getType() == Terrain_Dunes);
        bool down = (currentGameMap->tileExists(location.x, location.y + 1) == false) || (currentGameMap->getTile(location.x, location.y + 1)->getType() == Terrain_Dunes);
        bool left = (currentGameMap->tileExists(location.x - 1, location.y) == false) || (currentGameMap->getTile(location.x - 1, location.y)->getType() == Terrain_Dunes);

        return TerrainTile_Dunes + (((int)up) | (right << 1) | (down << 2) | (left << 3));
    } break;

    case Terrain_Mountain: {
        // determine which surrounding tiles are mountains
        bool up = (currentGameMap->tileExists(location.x, location.y - 1) == false) || (currentGameMap->getTile(location.x, location.y - 1)->isMountain() == true);
        bool right = (currentGameMap->tileExists(location.x + 1, location.y) == false) || (currentGameMap->getTile(location.x + 1, location.y)->isMountain() == true);
        bool down = (currentGameMap->tileExists(location.x, location.y + 1) == false) || (currentGameMap->getTile(location.x, location.y + 1)->isMountain() == true);
        bool left = (currentGameMap->tileExists(location.x - 1, location.y) == false) || (currentGameMap->getTile(location.x - 1, location.y)->isMountain() == true);

        return TerrainTile_Mountain + (((int)up) | (right << 1) | (down << 2) | (left << 3));
    } break;

    case Terrain_Spice: {
        // determine which surrounding tiles are spice
        bool up = (currentGameMap->tileExists(location.x, location.y - 1) == false) || (currentGameMap->getTile(location.x, location.y - 1)->isSpice() == true);
        bool right = (currentGameMap->tileExists(location.x + 1, location.y) == false) || (currentGameMap->getTile(location.x + 1, location.y)->isSpice() == true);
        bool down = (currentGameMap->tileExists(location.x, location.y + 1) == false) || (currentGameMap->getTile(location.x, location.y + 1)->isSpice() == true);
        bool left = (currentGameMap->tileExists(location.x - 1, location.y) == false) || (currentGameMap->getTile(location.x - 1, location.y)->isSpice() == true);

        return TerrainTile_Spice + (((int)up) | (right << 1) | (down << 2) | (left << 3));
    } break;

    case Terrain_ThickSpice: {
        // determine which surrounding tiles are thick spice
        bool up = (currentGameMap->tileExists(location.x, location.y - 1) == false) || (currentGameMap->getTile(location.x, location.y - 1)->getType() == Terrain_ThickSpice);
        bool right = (currentGameMap->tileExists(location.x + 1, location.y) == false) || (currentGameMap->getTile(location.x + 1, location.y)->getType() == Terrain_ThickSpice);
        bool down = (currentGameMap->tileExists(location.x, location.y + 1) == false) || (currentGameMap->getTile(location.x, location.y + 1)->getType() == Terrain_ThickSpice);
        bool left = (currentGameMap->tileExists(location.x - 1, location.y) == false) || (currentGameMap->getTile(location.x - 1, location.y)->getType() == Terrain_ThickSpice);

        return TerrainTile_ThickSpice + (((int)up) | (right << 1) | (down << 2) | (left << 3));
    } break;

    case Terrain_SpiceBloom: {
        return TerrainTile_SpiceBloom;
    } break;

    case Terrain_SpecialBloom: {
        return TerrainTile_SpecialBloom;
    } break;

    default: {
        THROW(std::runtime_error, "Tile::getTerrainTile(): Invalid terrain type");
    } break;
    }
}

int Tile::getHideTile(int houseID) const {

    // are all surrounding tiles explored?
    if (((currentGameMap->tileExists(location.x, location.y - 1) == false) || (currentGameMap->getTile(location.x, location.y - 1)->isExplored(houseID) == true))
        && ((currentGameMap->tileExists(location.x + 1, location.y) == false) || (currentGameMap->getTile(location.x + 1, location.y)->isExplored(houseID) == true))
        && ((currentGameMap->tileExists(location.x, location.y + 1) == false) || (currentGameMap->getTile(location.x, location.y + 1)->isExplored(houseID) == true))
        && ((currentGameMap->tileExists(location.x - 1, location.y) == false) || (currentGameMap->getTile(location.x - 1, location.y)->isExplored(houseID) == true))) {
        return 0;
    }

    // determine what tiles are unexplored
    bool up = (currentGameMap->tileExists(location.x, location.y - 1) == false) || (currentGameMap->getTile(location.x, location.y - 1)->isExplored(houseID) == false);
    bool right = (currentGameMap->tileExists(location.x + 1, location.y) == false) || (currentGameMap->getTile(location.x + 1, location.y)->isExplored(houseID) == false);
    bool down = (currentGameMap->tileExists(location.x, location.y + 1) == false) || (currentGameMap->getTile(location.x, location.y + 1)->isExplored(houseID) == false);
    bool left = (currentGameMap->tileExists(location.x - 1, location.y) == false) || (currentGameMap->getTile(location.x - 1, location.y)->isExplored(houseID) == false);

    return (((int)up) | (right << 1) | (down << 2) | (left << 3));
}

int Tile::getFogTile(int houseID) const {

    // are all surrounding tiles fogged?
    if (((currentGameMap->tileExists(location.x, location.y - 1) == false) || (currentGameMap->getTile(location.x, location.y - 1)->isFogged(houseID) == false))
        && ((currentGameMap->tileExists(location.x + 1, location.y) == false) || (currentGameMap->getTile(location.x + 1, location.y)->isFogged(houseID) == false))
        && ((currentGameMap->tileExists(location.x, location.y + 1) == false) || (currentGameMap->getTile(location.x, location.y + 1)->isFogged(houseID) == false))
        && ((currentGameMap->tileExists(location.x - 1, location.y) == false) || (currentGameMap->getTile(location.x - 1, location.y)->isFogged(houseID) == false))) {
        return 0;
    }

    // determine what tiles are fogged
    bool up = (currentGameMap->tileExists(location.x, location.y - 1) == false) || (currentGameMap->getTile(location.x, location.y - 1)->isFogged(houseID) == true);
    bool right = (currentGameMap->tileExists(location.x + 1, location.y) == false) || (currentGameMap->getTile(location.x + 1, location.y)->isFogged(houseID) == true);
    bool down = (currentGameMap->tileExists(location.x, location.y + 1) == false) || (currentGameMap->getTile(location.x, location.y + 1)->isFogged(houseID) == true);
    bool left = (currentGameMap->tileExists(location.x - 1, location.y) == false) || (currentGameMap->getTile(location.x - 1, location.y)->isFogged(houseID) == true);

    return (((int)up) | (right << 1) | (down << 2) | (left << 3));
}

#ifdef __cpp_coroutines

#include "misc/generator.h"

generator<Uint32> Tile::all_assigned() const noexcept
{
    for (auto i : assignedInfantryList)
        co_yield i;

    for (auto i : assignedNonInfantryGroundObjectList)
        co_yield i;

    for (auto i : assignedUndergroundUnitList)
        co_yield i;

    for (auto i : assignedAirUnitList)
        co_yield i;
}

template<typename Pred>
void Tile::selectFilter(int houseID, ObjectBase** lastCheckedObject, ObjectBase** lastSelectedObject, Pred&& predicate)
{
    auto changed = false;
    ObjectBase* obj = nullptr;
    ObjectBase* last_selected = nullptr;

    for (auto object_id : all_assigned()) {
        obj = currentGame->getObjectManager().getObject(object_id);

        assert(object_id == obj->getObjectID());

        if (obj->isSelected() || houseID != obj->getOwner()->getHouseID())
            continue;

        if (!predicate(obj))
            continue;

        obj->setSelected(true);

        if (currentGame->getSelectedList().insert(object_id).second)
            changed = true;

        last_selected = obj;
    }

    if (changed)
        currentGame->selectionChanged();

    if (obj)
        *lastCheckedObject = obj;

    if (last_selected)
        *lastSelectedObject = last_selected;
}
#else
template<typename Pred>
void Tile::selectFilter(int houseID, ObjectBase** lastCheckedObject, ObjectBase** lastSelectedObject, Pred&& predicate)
{
    ConcatIterator<Uint32> iterator;
    iterator.addList(assignedInfantryList);
    iterator.addList(assignedNonInfantryGroundObjectList);
    iterator.addList(assignedUndergroundUnitList);
    iterator.addList(assignedAirUnitList);

    auto changed = false;
    ObjectBase* obj = nullptr;
    ObjectBase* last_selected = nullptr;

    for (; !iterator.isIterationFinished(); ++iterator) {
        obj = currentGame->getObjectManager().getObject(*iterator);

        if (obj->isSelected() || houseID != obj->getOwner()->getHouseID())
            continue;

        if (!predicate(obj))
            continue;

        obj->setSelected(true);

        if (currentGame->getSelectedList().insert(obj->getObjectID()).second)
            changed = true;

        last_selected = obj;
    }

    if (changed)
        currentGame->selectionChanged();

    if (obj)
        *lastCheckedObject = obj;

    if (last_selected)
        *lastSelectedObject = last_selected;
}
#endif
