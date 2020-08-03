/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2015-2020 Petr Ohlidal

    For more information, see http://www.rigsofrods.org/

    Rigs of Rods is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 3, as
    published by the Free Software Foundation.

    Rigs of Rods is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Rigs of Rods. If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include "Application.h"

#include "BeamData.h" // RoR::ActorSpawnRequest
#include "BeamFactory.h"
#include "CameraManager.h" // CameraManager::CameraBehaviors
#include "CharacterFactory.h"
#include "GfxScene.h"
#include "ForceFeedback.h"
#include "OutProtocol.h"
#include "SceneMouse.h"

/// The simulation controller object
/// It's lifetime is tied to single gameplay session. When user returns to main menu, it's destroyed.
///
/// RoR's gameplay is quite simple in structure, it consists of:
///  - static terrain:  static elevation map, managed by `TerrainManager` singleton.
///                     this includes static collision objects (or intrusion detection objects), managed by `TerrainObjectManager`.
///  - softbody actors: a.k.a "trucks" or "vehicles", managed by `ActorManager`. They collide with static terrain and each other.
///                     this includes 'fixes' - actors with partially fixed position.
///  - characters:      player-controlled human figures with their own primitive physics, managed by `CharacterFactory`
///                     these only collide with static terrain, not actors.
/// For convenience and to help manage interactions, this class provides methods to manipulate these elements.
///
/// Architecture is currently undergoing a refactor.
///  OLD: We use threadpool; rendering loop runs on main thread, simulation is updated in between frames via 'Ogre::FrameListener' interface which also provides timing.
///   1. SimController::EnterGameplayLoop() invokes Ogre::Root::renderOneFrame() which invokes SimController::frameStarted()
///   2. Wait for simulation task to finish.
///   3. Gather user inputs; send/receive network data
///   3. Update OGRE scene; Put flexbody tasks to threadpool and wait for them to finish.
///   4. Run new sim. task: split time since last frame into constant size chunks and advance simulation
///   5. While sim. task is running, let main thread do the rendering
///
///  FUTURE: Use threadpool, run simulation loop on main thread and render in background.
///   1. Check time since last sim. update and sleep if we're up-to-date.
///   2. Check time since last render; if necessary, copy sim. data and put an "update scene and render" task to threadpool.
///   2. Gather user inputs; send/receive network data
///   3. Update simulation
class SimController: public ZeroedMemoryAllocator
{
public:
    SimController(RoR::ForceFeedback* ff);

    // Actor management interface
    void   UpdateLastSpawnInfo   (RoR::ActorSpawnRequest rq);
    void   RemoveActorByCollisionBox(std::string const & ev_src_instance_name, std::string const & box_name); ///< Scripting utility. TODO: Does anybody use it? ~ only_a_ptr, 08/2017

    // Scripting interface
    float  getTime               () { return m_time; }
    void   UpdateDirectionArrow  (char* text, Ogre::Vector3 position);
    void   ShowLoaderGUI         (int type, const Ogre::String& instance, const Ogre::String& box);
    void   OnLoaderGuiCancel     ();
    void   OnLoaderGuiApply      (RoR::LoaderType type, CacheEntry* entry, std::string sectionconfig);
    void   StartRaceTimer        (int id); // Do not call manually!
    void   StopRaceTimer         (); // Do not call manually!
    void   SetRaceTimeDiff       (float diff) { m_race_time_diff = diff; }; // Do not call manually!
    float  GetRaceTimeDiff       () const { return m_race_time_diff; };
    void   SetRaceBestTime       (float time) { m_race_best_time = time; }; // Do not call manually!
    float  GetRaceBestTime       () const { return m_race_best_time; };
    int    GetRaceId             () const { return m_race_id; }
    float  GetRaceTime           () const { return static_cast<float>(m_time - m_race_start_time); }
    bool   IsRaceInProgress      () const { return m_race_id != -1; }
    bool   LoadTerrain           (std::string terrn_file);

    // GUI interface
    void   TeleportPlayerXZ      (float x, float y); // Teleport UI

    /// @return True if everything was prepared OK and simulation may start.
    bool   SetupGameplayLoop     ();

    RoR::ForceFeedback*          GetForceFeedback()         { return m_force_feedback; }
    RoR::CharacterFactory*       GetCharacterFactory  ()    { return &m_character_factory; }
    RoR::SceneMouse&             GetSceneMouse()            { return m_scene_mouse; }
    Ogre::Vector3                GetDirArrowTarget()        { return m_dir_arrow_pointed; }
    bool                         IsPressurizingTyres() const { return m_pressure_pressed; }
    bool                         AreControlsLocked() const;
    bool                         IsGUIHidden()              { return m_hide_gui; }

    Character* GetPlayerCharacter()                         { return m_player_character; }

    bool GetPhysicsPaused()                                 { return m_physics_simulation_paused; }
    void SetPhysicsPausedInternal(bool paused)              { m_physics_simulation_paused = paused; }

    void SetTerrainEditorMouseRay(Ogre::Ray ray);

    void   UpdateSimulation(float dt);
    void   CleanupAfterSimulation(); /// Unloads all data

private:

    void   UpdateForceFeedback     ();
    void   HandleSavegameShortcuts ();
    void   UpdateInputEvents       (float dt);
    void   HideGUI                 (bool hidden);


    Character*               m_player_character;
    RoR::CharacterFactory    m_character_factory;
    RoR::SceneMouse          m_scene_mouse;
    Ogre::Real               m_time_until_next_toggle; //!< just to stop toggles flipping too fast
    float                    m_last_simulation_speed;  //!< previously used time ratio between real time (evt.timeSinceLastFrame) and physics time ('dt' used in calcPhysics)
    bool                     m_is_pace_reset_pressed;
    float                    m_physics_simulation_time; //!< Amount of time the physics simulation is going to be advanced
    bool                     m_physics_simulation_paused;
    int                      m_stats_on;
    float                    m_time;
    RoR::ForceFeedback*      m_force_feedback;
    bool                     m_hide_gui;
    bool                     m_pressure_pressed;

    RoR::ActorSpawnRequest   m_pending_spawn_rq;
    CacheEntry*              m_last_cache_selection; //!< Vehicle/load
    CacheEntry*              m_last_skin_selection;
    Ogre::String             m_last_section_config;

    Ogre::Vector3            m_dir_arrow_pointed;

    int                      m_race_id;
    float                    m_race_time_diff;
    float                    m_race_best_time;
    float                    m_race_start_time;

    bool                     m_soft_reset_mode;
    bool                     m_advanced_vehicle_repair;
    float                    m_advanced_vehicle_repair_timer;

    float                    m_pressure_pressed_timer;

    Ogre::Vector3            m_reload_pos;
    Ogre::Quaternion         m_reload_dir;

    Ogre::Ray                m_terrain_editor_mouse_ray;
};
