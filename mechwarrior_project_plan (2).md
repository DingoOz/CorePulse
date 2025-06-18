# CorePulse - Open Source Project Plan

## Project Overview

**Project Name:** CorePulse  
**Target Platforms:** Linux, Windows  
**Programming Language:** C++20/C++23  
**Graphics API:** OpenGL 4.3+ with modern C++ wrappers  
**Audio:** Custom low-level audio system with SDL2 for platform abstraction  
**License:** MIT or GPL v3 (to be determined)

### Vision Statement
Create an authentic homage to the classic 1990s mech combat games whilst leveraging modern C++ features and cross-platform compatibility. Focus on immersive giant robot combat, deep customisation, and compelling narrative within an original universe of mercenary warfare.

## Technical Architecture

### Core Engine Components

#### 1. Platform Abstraction Layer
- **Window Management:** SDL2 for window creation and input handling
- **File System:** `std::filesystem` (C++17) with custom extensions
- **Threading:** `std::thread`, `std::async` for multi-threaded systems
- **Memory Management:** Custom allocators using C++20 concepts

#### 2. Rendering Engine
- **Graphics API:** OpenGL 4.3+ with modern shader pipeline
- **Asset Loading:** glTF 2.0 with extensions support
- **Texture Management:** DDS/PNG support with mipmapping
- **Lighting:** Deferred rendering pipeline for multiple light sources
- **Post-Processing:** HDR, bloom, and atmospheric effects

#### 3. Audio System
- **Backend:** SDL2_mixer for cross-platform audio
- **3D Audio:** Custom spatial audio implementation
- **Music:** Streaming .OGG playback system
- **SFX:** Sample-based effects with distance attenuation

#### 4. Physics & Collision
- **Custom Physics:** Simplified rigid body dynamics for mechs
- **Collision Detection:** AABB trees with sphere/capsule primitives
- **Terrain:** Heightmap-based collision with smooth interpolation

#### 5. Entity Component System (ECS)
- **Modern C++20 Implementation:** Using concepts and ranges
- **Components:** Transform, Mesh, Physics, Weapon, AI, Player
- **Systems:** Rendering, Physics, Combat, AI, Input

### Development Tools

#### Asset Pipeline
- **3D Models:** Blender → Custom .mesh format
- **Textures:** GIMP/Photoshop → DDS compression
- **Audio:** Audacity → .OGG compression
- **Build System:** CMake 3.20+ with vcpkg for dependencies

#### Debug & Profiling
- **Logging:** Custom logging system with severity levels
- **Profiler:** Integrated frame time and memory profiler
- **Console:** In-game debug console with command system

## Game Architecture

### Core Game Systems

#### 1. Game State Manager
```cpp
enum class GameState {
    MainMenu,
    Briefing,
    MechLab,
    Mission,
    Debriefing,
    Settings
};
```

#### 2. Mission System
- **Mission Loader:** glTF-based scene definitions with game-specific extensions
- **Objective Tracking:** Dynamic objective system
- **Scripting:** Simple Lua integration for mission events
- **Save System:** Binary save format with version compatibility

#### 3. Mech Customisation (MechLab)
- **Hardpoint System:** Weapon mounting with tonnage limits
- **Heat Management:** Realistic heat dissipation simulation
- **Armour Configuration:** Location-based damage modelling
- **Performance Calculations:** Speed, turning, heat efficiency

#### 4. Combat System
- **Weapon Types:** Ballistic, energy, missile systems
- **Damage Model:** Component-based damage with criticals
- **Heat System:** Weapon heat generation and cooling
- **Targeting:** Lead calculation and lock-on mechanics

## User Interface Design

### Main Menu System
- **Title Screen:** Animated 3D mech with atmospheric background
- **Navigation:** Mouse and keyboard support
- **Options:** Graphics, audio, controls configuration
- **Continue/New Game:** Save slot management

### HUD Design (In-Mission)
- **Radar:** 360-degree threat detection
- **Weapon Groups:** Configurable weapon assignments
- **Heat Gauge:** Real-time heat monitoring
- **Damage Display:** Mech schematic with damage indicators
- **Targeting Reticle:** Distance and target information

### MechLab Interface
- **3D Mech View:** Rotatable mech model with hardpoint highlighting
- **Component Lists:** Drag-and-drop equipment installation
- **Statistics Panel:** Real-time performance calculations
- **Loadout Validation:** Tonnage and critical slot checking

## World Building & Narrative

### Setting: The Frontier Rebellions (2387)
The year is 2387, and humanity has spread across the galaxy through a network of jump gates. In the distant Frontier Sectors, beyond the reach of the Core Worlds, mercenary companies fight for survival, profit, and sometimes honour. Advanced technology is scarce, and every combat walker is precious.

### Background Lore

#### The Kepler System Crisis
The remote Kepler system has become a flashpoint between the retreating Terran Colonial Forces and advancing Centauri Federation troops. Local planetary governments, caught in the crossfire, have turned to mercenary companies for protection.

#### Faction Overview
- **Centauri Federation:** Expansionist corporate state using combined arms tactics
- **Terran Colonial Forces:** Retreating but technologically superior military
- **Independent Colonies:** Desperate defenders with obsolete equipment
- **Mercenary Companies:** Opportunistic but honourable warriors

### Campaign Structure: "The Titanfall Company"

#### Protagonist: Commander Sarah "Reaper" Morrison
A former Terran Colonial Forces walker pilot turned mercenary commander after her unit was abandoned during a Centauri offensive. Now leads a small lance of fellow outcasts seeking redemption and profit in the Frontier.

#### Supporting Characters
- **Tech Sergeant Marcus "Wrench" Chen:** Brilliant technician keeping ancient walkers operational
- **Lieutenant Jake "Wildcard" Torres:** Young hotshot pilot with natural talent
- **Captain Elena "Iron" Vasquez:** Veteran pilot and tactical advisor

### Mission Campaign: "Blood and Steel"

#### Act I: Initiation (Missions 1-3)
**Mission 1: "Shakedown Run"**
- **Objective:** Escort supply convoy through pirate territory
- **Walkers Available:** Light walkers only (Scout, Ranger classes)
- **Story:** Introduction to controls and basic combat
- **Location:** Desert canyons of Perdition's Gate

**Mission 2: "Proof of Worth"**
- **Objective:** Defend mining facility from Centauri raiders
- **New Mechanic:** Heat management becomes critical
- **Story:** Earn trust of local colonial commander
- **Location:** Industrial complex with multiple levels

**Mission 3: "Old Debts"**
- **Objective:** Assault former unit's base to recover stolen equipment
- **New Mechanic:** Stealth and reconnaissance elements
- **Story:** Confront past betrayals and secure better walkers
- **Location:** Abandoned military base in toxic swamplands

#### Act II: Escalation (Missions 4-7)
**Mission 4: "Midnight Requisition"**
- **Objective:** Capture Centauri supply depot under cover of darkness
- **New Mechanic:** Night fighting with limited visibility
- **Story:** Acquire medium walkers and advanced weaponry
- **Location:** Moonlit desert with scattered outposts

**Mission 5: "The Betrayer's Lance"**
- **Objective:** Eliminate Morrison's former commanding officer
- **New Mechanic:** Multi-stage boss fight against superior force
- **Story:** Personal vendetta reaches climax
- **Location:** Urban battlefield with civilian considerations

**Mission 6: "Hammer and Anvil"**
- **Objective:** Coordinate with Colonial forces in massive assault
- **New Mechanic:** Command AI allies and combined arms
- **Story:** Temporary alliance against common enemy
- **Location:** Fortress siege across multiple terrain types

**Mission 7: "The Price of Honour"**
- **Objective:** Protect evacuation whilst outnumbered 3:1
- **New Mechanic:** Defensive positioning and resource management
- **Story:** Choose between profit and protecting civilians
- **Location:** Spaceport under bombardment

#### Act III: Resolution (Missions 8-10)
**Mission 8: "Ghost Lance"**
- **Objective:** Infiltrate Centauri command centre for intelligence
- **New Mechanic:** Electronic warfare and sensor management
- **Story:** Discover larger conspiracy threatening the Frontier
- **Location:** High-tech facility with advanced defences

**Mission 9: "The Killing Ground"**
- **Objective:** Break Centauri siege of planetary capital
- **New Mechanic:** Large-scale battle with assault walkers
- **Story:** Final push to liberate the system
- **Location:** Sprawling metropolitan battlefield

**Mission 10: "Steel Reckoning"**
- **Objective:** Single combat against enemy champion for system's fate
- **New Mechanic:** Arena-style duel with multiple phases
- **Story:** Morrison's ultimate test of skill and honour
- **Location:** Ancient gladiatorial arena beneath the capital

### Broader Universe Connections
Each mission contains subtle references to larger galactic lore:
- Corporate manipulation of interstellar communications
- Rumours of lost terraforming technology
- Political machinations of the Core Worlds
- The precarious nature of Frontier independence

## Technical Implementation Plan

### Phase 1: Foundation (Months 1-3)
- Set up build system and development environment
- Implement basic window management and input handling
- Create OpenGL renderer with basic mesh loading
- Establish ECS architecture with core components

### Phase 2: Core Systems (Months 4-6)
- Implement physics and collision detection
- Create audio system with 3D positioning
- Build UI framework for menus and HUD
- Develop mech simulation systems (movement, heat, damage)

### Phase 3: Game Logic (Months 7-9)
- Mission loading and objective system
- Combat mechanics and weapon systems
- AI behaviour for enemy mechs
- MechLab customisation interface

### Phase 4: Content Creation (Months 10-12)
- Asset creation pipeline and tools
- Mission scripting and level design
- Audio implementation (music, SFX, voice)
- Campaign integration and story implementation

### Phase 5: Polish and Release (Months 13-15)
- Performance optimisation and bug fixing
- Cross-platform testing and compatibility
- Documentation and modding support
- Community beta testing and feedback integration

## Asset Requirements

### 3D Models
- **Walker Chassis:** 12 unique walker designs (4 light, 4 medium, 4 heavy) in glTF format
- **Weapons:** 20+ weapon variants with mounting animations
- **Environment:** Buildings, terrain features, vehicles
- **Effects:** Weapon impacts, explosions, environmental

### Audio Assets
- **Music:** 45 minutes of original soundtrack
- **SFX:** 200+ sound effects for weapons, movement, environment
- **Voice Acting:** Mission briefings and radio chatter
- **Ambient:** Environmental audio for immersion

### Textures and UI
- **Walker Textures:** PBR materials for all walker variants
- **Environment:** Terrain, building, and prop textures
- **UI Elements:** HUD components, menu interfaces
- **Particle Effects:** Weapon trails, smoke, atmospheric

## Risk Assessment and Mitigation

### Technical Risks
- **Cross-platform Compatibility:** Regular testing on both platforms
- **Performance Optimisation:** Profile early and often
- **Asset Pipeline Complexity:** Start with simple formats, iterate

### Scope Risks
- **Feature Creep:** Maintain strict milestone adherence
- **Content Volume:** Focus on quality over quantity for initial release
- **Perfectionism:** Set "good enough" standards for first iteration

### Legal Considerations
- **Original IP:** Create completely original universe and terminology
- **Trademark Issues:** Avoid any references to existing franchises
- **Open Source Compliance:** Choose appropriate license early

## Success Metrics

### Technical Goals
- Stable 60 FPS on mid-range hardware (GTX 1060 equivalent)
- Sub-5 second loading times between missions
- Zero critical bugs in campaign completion
- Cross-platform feature parity

### Gameplay Goals
- 8-12 hour campaign completion time
- Intuitive mech customisation system
- Challenging but fair AI opponents
- Compelling narrative progression

### Community Goals
- Active modding community within 6 months
- Positive reception from classic MechWarrior fans
- Foundation for potential expansion content
- Open source contributions from community

## Future Expansion Possibilities

### Post-Release Content
- Additional campaign storylines
- Multiplayer combat modes
- Extended mech and weapon catalogues
- User-generated content tools

### Technical Enhancements
- VR support for cockpit view
- Advanced physics simulation
- Enhanced AI behaviours
- Improved graphics pipeline

This project plan provides a solid foundation for creating an authentic giant robot combat experience whilst maintaining modern development practices and cross-platform compatibility. The focus on open source development ensures community involvement and long-term sustainability.