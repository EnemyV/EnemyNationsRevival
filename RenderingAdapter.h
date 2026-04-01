// NOTE: This file is NOT compiled into the game. Design prototype only.
// The actual working RenderingAdapter is in enations_latest/src/RenderingAdapter.h/cpp.
// Most methods here are stubs/TODOs with no game integration.
#pragma once

#include <memory>
#include <glm/glm.hpp>

// Forward declarations - avoid including old rendering headers
class CAnimAtr;
class CUnit;
class CVehicle;
class CBuilding;
class CTile;
class CWorld;
class GameWindow;
class SpriteRenderer;
class TerrainRenderer;
class TextRenderer;
class FogOfWarRenderer;
class SelectionRenderer;

/**
 * RenderingAdapter - Bridge between old game logic and new SDL2/OpenGL renderer
 *
 * Purpose:
 *   - Intercepts CAnimAtr::Render() calls from old game code
 *   - Queues sprites/terrain to new GPU renderer instead of drawing to DIB
 *   - Converts game object positions to renderer coordinates
 *   - Handles all rendering without modifying game logic
 *
 * Usage:
 *   1. Initialize at startup: RenderingAdapter::Initialize(gameWindow)
 *   2. Set animator when ready: RenderingAdapter::SetAnimAtr(animAtr)
 *   3. Call from old code: Instead of DIB drawing → queue to new renderer
 *
 * Design:
 *   - No modifications to original game code
 *   - All rendering redirected to new system
 *   - Coordinate transformations happen here
 *   - Save/load system unaffected
 */
class RenderingAdapter {
public:
    /**
     * Initialize adapter with game window and rendering systems
     * @param gameWindow Pointer to main GameWindow
     */
    static void Initialize(GameWindow* gameWindow);

    /**
     * Set the current animator (called when rendering area changes)
     * @param aa Pointer to CAnimAtr object
     */
    static void SetAnimAtr(const CAnimAtr* aa);

    /**
     * Main rendering entry point - replaces CAnimAtr::Render()
     * Instead of drawing to DIB, queues all objects to new renderer
     * This is called from the old CAnimAtr::Render() code path
     */
    static void Render();

    /**
     * Queue a unit sprite for rendering
     * Handles position, animation frame, damage state, selection highlight
     */
    static void QueueUnitSprite(const CUnit* unit);

    /**
     * Queue a vehicle/building sprite for rendering
     * Handles different angles, damage states, construction progress
     */
    static void QueueVehicleSprite(const CVehicle* vehicle);

    /**
     * Queue a building sprite for rendering
     * Handles building stages, direction variants, damage
     */
    static void QueueBuildingSprite(const CBuilding* building);

    /**
     * Queue terrain hex for rendering
     * Handles shading based on height, terrain type
     */
    static void QueueTerrainHex(const CTile& tile, const glm::vec3& screenPos);

    /**
     * Render selection highlights for selected units/hexes
     */
    static void RenderSelectionHighlights();

    /**
     * Render fog of war overlay
     * Shows unexplored, explored, and visible areas
     */
    static void RenderFogOfWar();

    /**
     * Flush all queued rendering operations to GPU
     * Called at end of render phase
     */
    static void FlushRenderQueue();

    /**
     * Convert game world coordinates to screen coordinates
     * Handles camera position, rotation, zoom, viewport bounds
     * @param worldPos Game world position (map coordinates)
     * @return Screen position in pixels
     */
    static glm::vec3 WorldToScreenCoords(const glm::vec3& worldPos);

    /**
     * Convert screen coordinates to game world coordinates
     * Used for input handling (mouse clicks)
     * @param screenPos Screen position in pixels
     * @return Game world position (map coordinates)
     */
    static glm::vec3 ScreenToWorldCoords(const glm::vec3& screenPos);

    /**
     * Get current camera position
     * @return Map coordinates of camera center
     */
    static glm::vec3 GetCameraPosition();

    /**
     * Get current viewport size
     * @return Width and height in pixels
     */
    static glm::vec2 GetViewportSize();

    /**
     * Check if a world position is visible in current viewport
     * @param worldPos Position to check
     * @return true if visible, false if off-screen
     */
    static bool IsPositionVisible(const glm::vec3& worldPos);

    /**
     * Set adapter enabled/disabled
     * When disabled, rendering goes through old path for debugging
     */
    static void SetEnabled(bool enabled) { s_enabled = enabled; }
    static bool IsEnabled() { return s_enabled; }

    /**
     * Validation and debugging helpers
     */
    static void PrintDebugInfo();
    static void ValidateState();

private:
    // Singleton state
    static GameWindow* s_gameWindow;
    static const CAnimAtr* s_currentAnimAtr;
    static bool s_enabled;
    static bool s_initialized;

    // Renderer pointers (cached for performance)
    // Made friends with RendererCompat for access
    static SpriteRenderer* s_spriteRenderer;
    static TerrainRenderer* s_terrainRenderer;
    static TextRenderer* s_textRenderer;
    static FogOfWarRenderer* s_fogOfWarRenderer;
    static SelectionRenderer* s_selectionRenderer;

    // RendererCompat (compatibility shim) needs access to render pointers
    // Will be initialized by Initialize() method

    // Internal helpers (not exposed to game code)
    static void CacheRenderers();
    static void QueueAllVisibleObjects();
    static void RenderTerrain();
    static void RenderUnitsAndVehicles();
    static void RenderBuildings();
    static void RenderEffects();

    /**
     * Sprite metadata helpers
     * These determine which sprite to render based on game state
     */
    static const char* GetUnitSpriteId(const CUnit* unit);
    static const char* GetVehicleSpriteId(const CVehicle* vehicle);
    static const char* GetBuildingSpriteId(const CBuilding* building);

    /**
     * Animation frame selection
     * Based on current tick, animation speed, etc.
     */
    static int GetCurrentAnimationFrame(const CUnit* unit);
    static float GetDamageColorTint(int damageLevel);

    /**
     * Coordinate transformations
     * Handle isometric projection, viewport clipping, world wrapping
     */
    static glm::vec3 CalculateScreenPosition(const CUnit* unit);
    static glm::vec3 CalculateScreenPosition(const CVehicle* vehicle);
    static glm::vec3 CalculateScreenPosition(const CBuilding* building);

    /**
     * Viewport and camera calculations
     * Based on CAnimAtr state (position, rotation, zoom)
     */
    static glm::mat4 GetViewMatrix();
    static glm::mat4 GetProjectionMatrix();
    static glm::vec2 GetHexScreenSize();

    // Internal state tracking
    static bool s_spriteQueueActive;
    static int s_spriteQueueCount;
    static int s_terrainQueueCount;
};

/**
 * Compatibility shim - allows old code to call new rendering
 * These functions are called from the old CAnimAtr::Render() path
 */
namespace RendererCompat {
    /**
     * Called by old sprite rendering code - queues instead of drawing
     * @param spriteId Sprite identifier
     * @param x Screen X coordinate
     * @param y Screen Y coordinate
     * @param frame Animation frame (0-based)
     * @param color Tint color (RGBA)
     */
    void DrawSprite(const char* spriteId, int x, int y, int frame, unsigned int color);

    /**
     * Called by old terrain code - queues terrain instead of drawing
     * @param x Screen X coordinate
     * @param y Screen Y coordinate
     * @param shadeLevel Terrain shade (0-7)
     * @param textureId Terrain texture ID
     */
    void DrawTerrain(int x, int y, int shadeLevel, int textureId);

    /**
     * Called by old effect code - queues effects instead of drawing
     * @param effectType Type of effect (selection, highlight, etc.)
     * @param x Screen X coordinate
     * @param y Screen Y coordinate
     */
    void DrawEffect(int effectType, int x, int y);

    /**
     * Clear rendering queue for new frame
     */
    void ClearRenderQueue();

    /**
     * Flush all queued operations to GPU
     */
    void FlushRenderQueue();
}

#endif // RENDERINGADAPTER_H
