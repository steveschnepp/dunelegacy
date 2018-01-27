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

#ifndef HOUSE_H
#define HOUSE_H

#include <misc/InputStream.h>
#include <misc/OutputStream.h>
#include <Definitions.h>
#include <DataTypes.h>
#include <data.h>
#include <Choam.h>

#include <players/Player.h>

#include <memory>

// forward declarations
class UnitBase;
class StructureBase;
class ObjectBase;
class HumanPlayer;

class House
{
public:
    House(int newHouse, int newCredits, int maxUnits, Uint8 team = 0, int quota = 0);
    explicit House(InputStream& stream);
    House(const House &) = delete;
    House(House &&) = delete;
    House& operator=(const House &) = delete;
    House& operator=(House &&) = delete;
    void init();
    virtual ~House();
    virtual void save(OutputStream& stream) const;

    void addPlayer(const std::shared_ptr<Player>& newPlayer);

    int getHouseID() const noexcept { return houseID; }
    int getTeam() const noexcept { return team; }

    bool isAI() const noexcept { return ai; }
    bool isAlive() const noexcept { return (team == 0) || !(((numStructures - numItem[Structure_Wall]) <= 0) && (((numUnits - numItem[Unit_Carryall] - numItem[Unit_Harvester] - numItem[Unit_Frigate] - numItem[Unit_Sandworm]) <= 0))); }

    bool hasCarryalls() const noexcept { return (numItem[Unit_Carryall] > 0); }
    bool hasBarracks() const noexcept { return (numItem[Structure_Barracks] > 0); }
    bool hasIX() const noexcept { return (numItem[Structure_IX] > 0); }
    bool hasLightFactory() const noexcept { return (numItem[Structure_LightFactory] > 0); }
    bool hasHeavyFactory() const noexcept { return (numItem[Structure_HeavyFactory] > 0); }
    bool hasRefinery() const noexcept { return (numItem[Structure_Refinery] > 0); }
    bool hasRepairYard() const noexcept { return (numItem[Structure_RepairYard] > 0); }
    bool hasStarPort() const noexcept { return (numItem[Structure_StarPort] > 0); }
    bool hasWindTrap() const noexcept { return (numItem[Structure_WindTrap] > 0); }
    bool hasSandworm() const noexcept { return (numItem[Unit_Sandworm] > 0); }
    bool hasRadar() const noexcept { return (numItem[Structure_Radar] > 0); }

    bool hasRadarOn() const noexcept { return (hasRadar() && hasPower()); }
    bool hasPower() const noexcept { return (producedPower >= powerRequirement); }

    int getNumStructures() const noexcept { return numStructures; };
    int getNumUnits() const noexcept{ return numUnits; };
    int getNumItems(int itemID) const { return (isStructure(itemID) || isUnit(itemID)) ? numItem[itemID] : 0; };

    int getCapacity() const noexcept{ return capacity; }

    int getProducedPower() const noexcept { return producedPower; }
    void setProducedPower(int newPower);
    int getPowerRequirement() const noexcept { return powerRequirement; }

    int getBuiltValue() const noexcept { return unitBuiltValue + structureBuiltValue; }
    int getUnitBuiltValue() const noexcept { return unitBuiltValue; }
    int getMilitaryValue() const noexcept { return militaryValue; }
    int getKillValue() const noexcept { return killValue; }
    int getLossValue() const noexcept { return lossValue; }
    int getStructureBuiltValue() const noexcept { return structureBuiltValue; }
    int getNumBuiltUnits() const noexcept { return numBuiltUnits; }
    int getNumBuiltStructures() const noexcept { return numBuiltStructures; }
    int getDestroyedValue() const noexcept { return destroyedValue; }
    int getNumDestroyedUnits() const noexcept { return numDestroyedUnits; }
    int getNumDestroyedStructures() const noexcept { return numDestroyedStructures; }
    int getNumBuiltItems(int itemID) const { return numItemBuilt[itemID]; }
    int getNumKilledItems(int itemID) const { return numItemKills[itemID]; }
    int getNumLostItems(int itemID) const { return numItemLosses[itemID]; }
    Sint32 getNumItemDamageInflicted(int itemID) const { return numItemDamageInflicted[itemID]; }
    FixPoint getHarvestedSpice() const noexcept { return harvestedSpice; }

    int getQuota() const noexcept { return quota; };
    int getMaxUnits() const noexcept { return maxUnits; };

    /**
        This function checks if the limit for ground units is already reached. Infantry units are only counted as 1/3.
        \return true, if the limit is already reached, false if building further ground units is allowed
    */
    bool isGroundUnitLimitReached() const {
        int numGroundUnit = numUnits - numItem[Unit_Soldier] - numItem[Unit_Trooper] - numItem[Unit_Carryall] - numItem[Unit_Ornithopter];
        return (numGroundUnit + (numItem[Unit_Soldier]+2)/3 + (numItem[Unit_Trooper]+2)/3  >= maxUnits);
    };

    /**
        This function checks if the limit for infantry units is already reached. Infantry units are only counted as 1/3.
        \return true, if the limit is already reached, false if building further infantry units is allowed
    */
    bool isInfantryUnitLimitReached() const {
        const auto numGroundUnit = numUnits - numItem[Unit_Soldier] - numItem[Unit_Trooper] - numItem[Unit_Carryall] - numItem[Unit_Ornithopter];
        return (numGroundUnit + numItem[Unit_Soldier]/3 + numItem[Unit_Trooper]/3  >= maxUnits);
    };

    /**
        This function checks if the limit for air units is already reached.
        \return true, if the limit is already reached, false if building further air units is allowed
    */
    bool isAirUnitLimitReached() const {
        return (numItem[Unit_Carryall] + numItem[Unit_Ornithopter] >= 11*std::max(maxUnits,25)/25);
    }

    Choam& getChoam() { return choam; };
    const Choam& getChoam() const { return choam; };


    FixPoint getStartingCredits() const { return startingCredits; }
    FixPoint getStoredCredits() const { return storedCredits; }
    int getCredits() const { return lround(storedCredits+startingCredits); }
    void addCredits(FixPoint newCredits, bool wasRefined = false);
    void returnCredits(FixPoint newCredits);
    FixPoint takeCredits(FixPoint amount);

    void printStat() const;

    void updateBuildLists();

    void update();

    void incrementUnits(int itemID);
    void decrementUnits(int itemID);
    void incrementStructures(int itemID);
    void decrementStructures(int itemID, const Coord& location);

    /**
        An object was hit by something or damaged somehow else.
        \param  pObject     the object that was damaged
        \param  damage      the damage taken
        \param  damagerID   the shooter of the bullet, rocket, etc. if known; NONE_ID otherwise
    */
    void noteDamageLocation(ObjectBase* pObject, int damage, Uint32 damagerID);

    void informWasBuilt(Uint32 itemID);
    void informHasKilled(Uint32 itemID);
    void informHasDamaged(Uint32 itemID, Uint32 damage);

    void lose(bool bSilent = false) const;
    void win();

    void freeHarvester(int xPos, int yPos);
    void freeHarvester(const Coord& coord) { freeHarvester(coord.x, coord.y); };
    StructureBase* placeStructure(Uint32 builderID, int itemID, int xPos, int yPos, bool bForcePlacing = false);
    UnitBase* createUnit(int itemID);
    UnitBase* placeUnit(int itemID, int xPos, int yPos);

    Coord getCenterOfMainBase() const;

    Coord getStrongestUnitPosition() const;

    const std::vector<std::shared_ptr<Player> >& getPlayerList() const { return players; };

protected:
    void decrementHarvesters();

    std::vector<std::shared_ptr<Player> > players;        ///< List of associated players that control this house

    bool    ai;             ///< Is this an ai player?

    Uint8   houseID;        ///< The house number
    Uint8   team;           ///< The team number

    int numStructures;          ///< How many structures does this player have?
    int numUnits;               ///< How many units does this player have?
    int numItem[Num_ItemID];    ///< This array contains the number of structures/units of a certain type this player has
    int numItemBuilt[Num_ItemID];  /// Number of items built by player
    int numItemKills[Num_ItemID]; /// Number of items killed by player
    int numItemLosses [Num_ItemID]; /// Number of items lost by player
    Sint32 numItemDamageInflicted[Num_ItemID]; /// Amount of damage inflicted by a specific unit type owned by the player

    int capacity;             ///< Total spice capacity
    int producedPower;        ///< Power prodoced by this player
    int powerRequirement;     ///< How much power does this player use?

    FixPoint storedCredits;   ///< current number of credits that are stored in refineries/silos
    FixPoint startingCredits; ///< number of starting credits this player still has
    int oldCredits;           ///< amount of credits in the last game cycle (used for playing the credits tick sound)

    int maxUnits;             ///< maximum number of units this house is allowed to build
    int quota;                ///< number of credits to win

    Choam   choam;            ///< the things that are deliverable at the starport

    int powerUsageTimer;      ///< every N ticks you have to pay for your power usage

    // statistic
    int unitBuiltValue;
    int structureBuiltValue;
    int militaryValue;
    int killValue;
    int lossValue;
    int numBuiltUnits;
    int numBuiltStructures;
    int destroyedValue;
    int numDestroyedUnits;
    int numDestroyedStructures;
    FixPoint harvestedSpice;
};

#endif // HOUSE_H
