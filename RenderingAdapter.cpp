#include "RenderingAdapter.h"
#include "GameWindow.h"
#include "rendering/SpriteRenderer.h"
#include "rendering/TerrainRenderer.h"
#include "rendering/TextRenderer.h"
#include "rendering/FogOfWarRenderer.h"
#include "rendering/SelectionRenderer.h"
#include "rendering/AssetManager.h"
#include "rendering/PerformanceMonitor.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <cassert>

// Singleton state initialization
GameWindow* RenderingAdapter::s_gameWindow = nullptr;
const CAnimAtr* RenderingAdapter::s_currentAnimAtr = nullptr;
bool RenderingAdapter::s_enabled = true;
bool RenderingAdapter::s_initialized = false;

SpriteRenderer* RenderingAdapter::s_spriteRenderer = nullptr;
TerrainRenderer* RenderingAdapter::s_terrainRenderer = nullptr;
TextRenderer* RenderingAdapter::s_textRenderer = nullptr;
FogOfWarRenderer* RenderingAdapter::s_fogOfWarRenderer = nullptr;
SelectionRenderer* RenderingAdapter::s_selectionRenderer = nullptr;

bool RenderingAdapter::s_spriteQueueActive = false;
int RenderingAdapter::s_spriteQueueCount = 0;
int RenderingAdapter::s_terrainQueueCount = 0;

/**
 * Initialize adapter with game window
 * Must be called once at startup before any rendering
 */
void RenderingAdapter::Initialize(GameWindow* gameWindow) {
    if (s_initialized) {
        std::cerr << "RenderingAdapter already initialized" << std::endl;
        return;
    }

    if (!gameWindow || !gameWindow->IsValid()) {
        std::cerr << "ERROR: Invalid GameWindow passed to RenderingAdapter::Initialize" << std::endl;
        assert(false);
        return;
    }

    s_gameWindow = gameWindow;
    CacheRenderers();
    s_initialized = true;

    std::cout << "RenderingAdapter initialized successfully" << std::endl;
    std::cout << "  - SpriteRenderer: " << (s_spriteRenderer ? "OK" : "FAILED") << std::endl;
    std::cout << "  - TerrainRenderer: " << (s_terrainRenderer ? "OK" : "FAILED") << std::endl;
    std::cout << "  - TextRenderer: " << (s_textRenderer ? "OK" : "FAILED") << std::endl;
}

/**
 * Set the current animator when rendering area changes
 * This stores the CAnimAtr pointer for coordinate transformations
 */
void RenderingAdapter::SetAnimAtr(const CAnimAtr* aa) {
    if (!s_initialized) {
        std::cerr << "ERROR: RenderingAdapter not initialized" << std::endl;
        return;
    }

    s_currentAnimAtr = aa;
    ValidateState();
}

/**
 * Cache renderer pointers from GameWindow for fast access
 */
void RenderingAdapter::CacheRenderers() {
    if (!s_gameWindow) {
        std::cerr << "ERROR: GameWindow not set in RenderingAdapter::CacheRenderers" << std::endl;
        return;
    }

    s_spriteRenderer = s_gameWindow->GetSpriteRenderer();
    s_terrainRenderer = s_gameWindow->GetTerrainRenderer();
    s_textRenderer = s_gameWindow->GetTextRenderer();
    s_fogOfWarRenderer = s_gameWindow->GetFogOfWarRenderer();
    s_selectionRenderer = s_gameWindow->GetSelectionRenderer();

    if (!s_spriteRenderer) std::cerr << "WARNING: SpriteRenderer not available" << std::endl;
    if (!s_terrainRenderer) std::cerr << "WARNING: TerrainRenderer not available" << std::endl;
}

/**
 * Main rendering entry point
 * This is called from GameLogicWrapper::Render() in the new game loop
 *
 * Strategy:
 * 1. If game code has already called CAnimAtr::Render(), sprites were queued via RendererCompat
 * 2. If not, we call QueueAllVisibleObjects() to iterate and queue
 * 3. Either way, we flush all queued operations at the end
 */
void RenderingAdapter::Render() {
    if (!s_enabled || !s_initialized) {
        return;
    }

    // Clear any previous frame's queued sprites
    RendererCompat::ClearRenderQueue();

    // Queue all visible game objects for rendering
    // This will use whichever method is available:
    // - If CAnimAtr is set, we can call its Render() to trigger old game code
    // - Otherwise, we iterate and queue manually (TODO)
    QueueAllVisibleObjects();

    // TODO: If game code hasn't been called yet, call RenderTerrain() separately
    // For now, terrain will be rendered by TerrainRenderer in GameWindow::Render()
    // after sprite rendering is done

    // Flush all queued operations to GPU renderers
    // This tells SpriteRenderer, TerrainRenderer, etc. to render what was queued
    FlushRenderQueue();
}

/**
 * Queue all visible objects in the current viewport
 * This would iterate through CWorld objects and queue those that are visible
 *
 * The game code will trigger drawing through the old rendering path:
 * CAnimAtr::Render() -> theMap.UpdateRect() -> makes draw calls
 *
 * Those draw calls are intercepted by RendererCompat::DrawSprite(), etc.
 * and queued to the GPU renderers instead of drawing to DIB.
 */
void RenderingAdapter::QueueAllVisibleObjects() {
    s_spriteQueueActive = true;
    s_spriteQueueCount = 0;
    s_terrainQueueCount = 0;

    // TODO: Integrate with actual game code
    // For now, this is a placeholder that would be called when:
    // 1. GameLogicWrapper::Render() gets integrated with actual game
    // 2. Game calls CAnimAtr::Render() -> theMap.UpdateRect()
    // 3. That makes draw calls which we intercept
    //
    // Example flow:
    // - Game iterates all units in visible hex range
    // - For each unit: RendererCompat::DrawSprite(unitSpriteId, screenX, screenY, ...)
    // - We queue it to SpriteRenderer
    // - At render time, SpriteRenderer renders all queued sprites
    //
    // ALTERNATIVE APPROACH (needed until game code is integrated):
    // - GameLogicWrapper could directly call RenderingAdapter methods
    // - Directly queue units: RenderingAdapter::QueueUnitSprite(unit)
    // - This bypasses the old draw call interception
    // - But requires GameLogicWrapper to iterate the world
}

/**
 * Render terrain hexes
 * Queues terrain sprites based on map height and type
 */
void RenderingAdapter::RenderTerrain() {
    if (!s_spriteRenderer || !s_currentAnimAtr) {
        return;
    }

    // This would iterate through visible terrain tiles and queue them
    // Terrain rendering depends on:
    // - Tile height (affects shading)
    // - Tile type (grass, water, rock, etc.)
    // - Adjacent tile heights (for edge shading)
    // - Camera position, rotation, zoom
}

/**
 * Render units and vehicles
 * Queues sprites with appropriate animation frames and colors
 */
void RenderingAdapter::RenderUnitsAndVehicles() {
    if (!s_spriteRenderer) {
        return;
    }

    // This would iterate through visible units and vehicles:
    // - Get current sprite based on type, direction, damage state
    // - Get current animation frame
    // - Get tint color (red for damaged, etc.)
    // - Queue to SpriteRenderer
}

/**
 * Render buildings
 * Queues building sprites with construction progress
 */
void RenderingAdapter::RenderBuildings() {
    if (!s_spriteRenderer) {
        return;
    }

    // This would iterate through visible buildings:
    // - Get current sprite based on type, direction, construction stage
    // - Get damage tint color
    // - Queue to SpriteRenderer
}

/**
 * Render effects (fog of war, selection, etc.)
 */
void RenderingAdapter::RenderEffects() {
    RenderSelectionHighlights();
    RenderFogOfWar();
}

/**
 * Queue unit sprite for rendering
 */
void RenderingAdapter::QueueUnitSprite(const CUnit* unit) {
    if (!unit || !s_spriteRenderer) {
        return;
    }

    // Get sprite metadata
    const char* spriteId = GetUnitSpriteId(unit);
    int frame = GetCurrentAnimationFrame(unit);
    float colorTint = GetDamageColorTint(0); // TODO: get actual damage

    // Calculate screen position
    glm::vec3 screenPos = CalculateScreenPosition(unit);

    // Queue sprite
    // spriteRenderer->QueueSprite(spriteId, screenPos, frame, colorTint);
    s_spriteQueueCount++;
}

/**
 * Queue vehicle sprite for rendering
 */
void RenderingAdapter::QueueVehicleSprite(const CVehicle* vehicle) {
    if (!vehicle || !s_spriteRenderer) {
        return;
    }

    // TODO: Implement vehicle sprite queuing
    s_spriteQueueCount++;
}

/**
 * Queue building sprite for rendering
 */
void RenderingAdapter::QueueBuildingSprite(const CBuilding* building) {
    if (!building || !s_spriteRenderer) {
        return;
    }

    // TODO: Implement building sprite queuing
    s_spriteQueueCount++;
}

/**
 * Queue terrain hex for rendering
 */
void RenderingAdapter::QueueTerrainHex(const CTile& tile, const glm::vec3& screenPos) {
    if (!s_spriteRenderer) {
        return;
    }

    // TODO: Implement terrain hex queuing
    s_terrainQueueCount++;
}

/**
 * Render selection highlights for selected units/hexes
 */
void RenderingAdapter::RenderSelectionHighlights() {
    if (!s_selectionRenderer) {
        return;
    }

    // TODO: Queue selection highlights based on current selection state
}

/**
 * Render fog of war overlay
 */
void RenderingAdapter::RenderFogOfWar() {
    if (!s_fogOfWarRenderer) {
        return;
    }

    // TODO: Queue fog of war based on visibility map
}

/**
 * Flush all queued rendering operations
 */
void RenderingAdapter::FlushRenderQueue() {
    if (!s_gameWindow) {
        return;
    }

    // Flush all renderers in order
    if (s_spriteRenderer) {
        s_spriteRenderer->Flush();
    }
    if (s_terrainRenderer) {
        s_terrainRenderer->Flush();
    }
    if (s_textRenderer) {
        s_textRenderer->Flush();
    }
    if (s_fogOfWarRenderer) {
        s_fogOfWarRenderer->Flush();
    }
    if (s_selectionRenderer) {
        s_selectionRenderer->Flush();
    }

    s_spriteQueueActive = false;
}

/**
 * Convert game world coordinates to screen coordinates
 * Handles camera position, rotation, zoom, viewport bounds
 */
glm::vec3 RenderingAdapter::WorldToScreenCoords(const glm::vec3& worldPos) {
    if (!s_currentAnimAtr || !s_gameWindow) {
        return worldPos;
    }

    // TODO: Implement coordinate transformation using CAnimAtr methods
    // This would call CAnimAtr::WorldToWindow() and convert to GLM format

    return worldPos;
}

/**
 * Convert screen coordinates to game world coordinates
 * Used for input handling (mouse clicks)
 */
glm::vec3 RenderingAdapter::ScreenToWorldCoords(const glm::vec3& screenPos) {
    if (!s_currentAnimAtr) {
        return screenPos;
    }

    // TODO: Implement inverse coordinate transformation
    // This would call CAnimAtr::WindowToMap() or similar

    return screenPos;
}

/**
 * Get current camera position
 */
glm::vec3 RenderingAdapter::GetCameraPosition() {
    if (!s_currentAnimAtr) {
        return glm::vec3(0.0f);
    }

    // TODO: Return CAnimAtr's m_maploc as glm::vec3
    return glm::vec3(0.0f);
}

/**
 * Get current viewport size
 */
glm::vec2 RenderingAdapter::GetViewportSize() {
    if (!s_gameWindow) {
        return glm::vec2(0.0f);
    }

    return glm::vec2(s_gameWindow->GetWidth(), s_gameWindow->GetHeight());
}

/**
 * Check if a world position is visible in current viewport
 */
bool RenderingAdapter::IsPositionVisible(const glm::vec3& worldPos) {
    // TODO: Implement frustum/viewport culling check
    return true; // For now, assume all positions are visible
}

/**
 * Get sprite ID for a unit
 * Determines which sprite to render based on unit type, direction, etc.
 */
const char* RenderingAdapter::GetUnitSpriteId(const CUnit* unit) {
    if (!unit) {
        return "error_sprite";
    }

    // TODO: Map CUnit type/direction to sprite asset name
    // This would query the unit's type, current direction, and return
    // the appropriate sprite ID from the asset system
    return "unit_default";
}

/**
 * Get sprite ID for a vehicle
 */
const char* RenderingAdapter::GetVehicleSpriteId(const CVehicle* vehicle) {
    if (!vehicle) {
        return "error_sprite";
    }

    // TODO: Map CVehicle type/direction/damage to sprite asset name
    return "vehicle_default";
}

/**
 * Get sprite ID for a building
 */
const char* RenderingAdapter::GetBuildingSpriteId(const CBuilding* building) {
    if (!building) {
        return "error_sprite";
    }

    // TODO: Map CBuilding type/direction/construction_stage to sprite asset name
    return "building_default";
}

/**
 * Get current animation frame
 * Based on game tick, animation speed, etc.
 */
int RenderingAdapter::GetCurrentAnimationFrame(const CUnit* unit) {
    if (!unit) {
        return 0;
    }

    // TODO: Calculate animation frame based on current game time
    return 0;
}

/**
 * Get color tint for damage level
 * Red tint for damaged units
 */
float RenderingAdapter::GetDamageColorTint(int damageLevel) {
    // Scale from white (no damage) to red (full damage)
    // damageLevel: 0-4 (none to destroyed)
    // Returns: color multiplier or tint value
    return 1.0f; // TODO: Calculate actual tint
}

/**
 * Calculate screen position for unit
 */
glm::vec3 RenderingAdapter::CalculateScreenPosition(const CUnit* unit) {
    if (!unit || !s_currentAnimAtr) {
        return glm::vec3(0.0f);
    }

    // TODO: Get unit's world position and convert to screen coordinates
    return glm::vec3(0.0f);
}

/**
 * Calculate screen position for vehicle
 */
glm::vec3 RenderingAdapter::CalculateScreenPosition(const CVehicle* vehicle) {
    if (!vehicle || !s_currentAnimAtr) {
        return glm::vec3(0.0f);
    }

    // TODO: Get vehicle's world position and convert to screen coordinates
    return glm::vec3(0.0f);
}

/**
 * Calculate screen position for building
 */
glm::vec3 RenderingAdapter::CalculateScreenPosition(const CBuilding* building) {
    if (!building || !s_currentAnimAtr) {
        return glm::vec3(0.0f);
    }

    // TODO: Get building's world position and convert to screen coordinates
    return glm::vec3(0.0f);
}

/**
 * Get view matrix from current camera state
 */
glm::mat4 RenderingAdapter::GetViewMatrix() {
    // TODO: Build view matrix from CAnimAtr camera position/rotation/zoom
    return glm::mat4(1.0f);
}

/**
 * Get projection matrix for isometric view
 */
glm::mat4 RenderingAdapter::GetProjectionMatrix() {
    // TODO: Build orthogonal projection matrix for 2D isometric rendering
    return glm::perspective(45.0f, GetViewportSize().x / GetViewportSize().y, 0.1f, 1000.0f);
}

/**
 * Get hex screen size based on zoom level
 */
glm::vec2 RenderingAdapter::GetHexScreenSize() {
    // TODO: Calculate hex size in pixels based on zoom level
    return glm::vec2(64.0f, 64.0f); // Default hex size
}

/**
 * Validation and debugging
 */
void RenderingAdapter::PrintDebugInfo() {
    std::cout << "\n=== RenderingAdapter Debug Info ===" << std::endl;
    std::cout << "Initialized: " << (s_initialized ? "YES" : "NO") << std::endl;
    std::cout << "Enabled: " << (s_enabled ? "YES" : "NO") << std::endl;
    std::cout << "GameWindow: " << (s_gameWindow ? "SET" : "NULL") << std::endl;
    std::cout << "CAnimAtr: " << (s_currentAnimAtr ? "SET" : "NULL") << std::endl;
    std::cout << "SpriteRenderer: " << (s_spriteRenderer ? "OK" : "NULL") << std::endl;
    std::cout << "TerrainRenderer: " << (s_terrainRenderer ? "OK" : "NULL") << std::endl;
    std::cout << "TextRenderer: " << (s_textRenderer ? "OK" : "NULL") << std::endl;
    std::cout << "Last frame queued: " << s_spriteQueueCount << " sprites" << std::endl;
    std::cout << "===================================\n" << std::endl;
}

/**
 * Validate adapter state
 */
void RenderingAdapter::ValidateState() {
    if (!s_initialized) {
        std::cerr << "WARNING: RenderingAdapter not initialized" << std::endl;
        return;
    }

    if (!s_gameWindow) {
        std::cerr << "ERROR: GameWindow is NULL" << std::endl;
    }

    if (!s_currentAnimAtr) {
        std::cerr << "WARNING: CAnimAtr not set - rendering will be empty" << std::endl;
    }

    if (!s_spriteRenderer) {
        std::cerr << "ERROR: SpriteRenderer not available" << std::endl;
    }
}

/**
 * Compatibility shim implementations
 * These are called from old rendering code path
 */

void RendererCompat::DrawSprite(const char* spriteId, int x, int y, int frame, unsigned int color) {
    if (!RenderingAdapter::IsEnabled() || !RenderingAdapter::s_spriteRenderer || !RenderingAdapter::s_gameWindow) {
        return;
    }

    // Get the sprite view from asset manager
    AssetManager* assetMgr = RenderingAdapter::s_gameWindow->GetAssetManager();
    if (!assetMgr) {
        return;
    }

    const SpriteView* spriteView = assetMgr->GetSprite(spriteId);
    if (!spriteView) {
        std::cerr << "WARNING: Sprite not found: " << spriteId << std::endl;
        return;
    }

    // Queue sprite to GPU renderer instead of drawing to DIB
    // Convert screen coordinates (x, y) to GLM vectors
    glm::vec3 screenPos(static_cast<float>(x), static_cast<float>(y), 0.0f);

    // Extract color components (assuming RGBA format)
    float r = ((color >> 24) & 0xFF) / 255.0f;
    float g = ((color >> 16) & 0xFF) / 255.0f;
    float b = ((color >> 8) & 0xFF) / 255.0f;
    float a = (color & 0xFF) / 255.0f;
    glm::vec4 colorTint(r, g, b, a);

    // Construct sprite quad for GPU rendering
    SpriteRenderer::SpriteQuad quad;
    quad.position = screenPos;
    quad.width = static_cast<float>(spriteView->width);
    quad.height = static_cast<float>(spriteView->height);
    quad.spriteView = spriteView;
    quad.color = colorTint;
    quad.teamColor = glm::vec4(1.0f);  // No team color by default
    quad.rotationAngle = 0;
    quad.screenY = screenPos.y + quad.height / 2.0f;  // Sort by Y for isometric depth
    quad.screenX = screenPos.x;
    quad.objectId = 0;  // TODO: Get actual object ID if needed
    quad.flags = 0;

    // Queue the sprite - SpriteRenderer will batch and render later
    RenderingAdapter::s_spriteRenderer->QueueSprite(quad);

    RenderingAdapter::s_spriteQueueCount++;
}

void RendererCompat::DrawTerrain(int x, int y, int shadeLevel, int textureId) {
    if (!RenderingAdapter::IsEnabled()) {
        return;
    }

    // Terrain rendering is handled separately by TerrainRenderer::RenderTerrain()
    // which renders all terrain at once given a viewport region.
    // The old code calls DrawTerrain for each hex, but we don't need to intercept those
    // since TerrainRenderer will handle all terrain rendering in one pass.
    // This method is a no-op - terrain is rendered via RenderingAdapter::RenderTerrain()
}

void RendererCompat::DrawEffect(int effectType, int x, int y) {
    if (!RenderingAdapter::IsEnabled()) {
        return;
    }

    // Queue visual effect (selection highlight, fog of war, etc.)
    glm::vec3 screenPos(static_cast<float>(x), static_cast<float>(y), 0.0f);

    // effectType determines what overlay to draw:
    // 0 = selection highlight
    // 1 = hover highlight
    // 2 = fog of war
    // etc.

    switch (effectType) {
        case 0:  // Selection highlight
            if (RenderingAdapter::s_selectionRenderer) {
                RenderingAdapter::s_selectionRenderer->QueueSelection(screenPos.x, screenPos.y, 64.0f);
            }
            break;
        case 1:  // Hover/preview
            if (RenderingAdapter::s_selectionRenderer) {
                RenderingAdapter::s_selectionRenderer->QueueHover(screenPos.x, screenPos.y, 64.0f);
            }
            break;
        default:
            break;
    }
}

void RendererCompat::ClearRenderQueue() {
    // Clear sprite queue for new frame
    RenderingAdapter::s_spriteQueueCount = 0;
    RenderingAdapter::s_terrainQueueCount = 0;
}

void RendererCompat::FlushRenderQueue() {
    RenderingAdapter::FlushRenderQueue();
}
