/*
 * This file is part of the WarheadCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _GAMEOBJECT_MODEL_H
#define _GAMEOBJECT_MODEL_H

#include "Define.h"
#include <G3D/AABox.h>
#include <G3D/Matrix3.h>
#include <G3D/Ray.h>
#include <G3D/Vector3.h>

namespace VMAP
{
    class WorldModel;
}

class GameObject;
struct GameObjectDisplayInfoEntry;

class WH_COMMON_API GameObjectModelOwnerBase
{
public:
    virtual ~GameObjectModelOwnerBase() = default;

    virtual bool IsSpawned() const = 0;
    virtual uint32 GetDisplayId() const = 0;
    virtual uint32 GetPhaseMask() const = 0;
    virtual G3D::Vector3 GetPosition() const = 0;
    virtual float GetOrientation() const = 0;
    virtual float GetScale() const = 0;
    virtual void DebugVisualizeCorner(G3D::Vector3 const& /*corner*/) const = 0;
};

class WH_COMMON_API GameObjectModel
{
    GameObjectModel() : phasemask(0), iInvScale(0), iScale(0), iModel(nullptr) { }

public:
    std::string name;

    [[nodiscard]] const G3D::AABox& GetBounds() const { return iBound; }

    ~GameObjectModel();

    [[nodiscard]] const G3D::Vector3& GetPosition() const { return iPos; }

    /** Enables\disables collision. */
    void disable() { phasemask = 0; }
    void enable(uint32 ph_mask) { phasemask = ph_mask; }

    [[nodiscard]] bool isEnabled() const { return phasemask != 0; }

    bool intersectRay(const G3D::Ray& Ray, float& MaxDist, bool StopAtFirstHit, uint32 ph_mask) const;

    static GameObjectModel* Create(std::unique_ptr<GameObjectModelOwnerBase> modelOwner, std::string const& dataPath);

    bool UpdatePosition();

private:
    bool initialize(std::unique_ptr<GameObjectModelOwnerBase> modelOwner, std::string const& dataPath);

    uint32 phasemask;
    G3D::AABox iBound;
    G3D::Matrix3 iInvRot;
    G3D::Vector3 iPos;
    float iInvScale;
    float iScale;
    VMAP::WorldModel* iModel;
    std::unique_ptr<GameObjectModelOwnerBase> owner;
};

WH_COMMON_API void LoadGameObjectModelList(std::string const& dataPath);

#endif // _GAMEOBJECT_MODEL_H
