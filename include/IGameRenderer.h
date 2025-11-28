#pragma once

#include "Core.h"
#include <string>
#include <vector>
#include <memory>

namespace TLS {

/**
 * @class IGameRenderer
 * @brief Abstract interface for game rendering/display
 * 
 * Provides abstraction layer for game output, enabling:
 * - Text-based console rendering (current implementation)
 * - Future 3D rendering integration
 * - Headless/test mode rendering
 */
class IGameRenderer {
public:
    virtual ~IGameRenderer() = default;
    
    // ========================================================================
    // Core Display Methods
    // ========================================================================
    
    /**
     * Initialize the renderer
     * @return true if initialization successful
     */
    virtual bool initialize() = 0;
    
    /**
     * Shutdown the renderer cleanly
     */
    virtual void shutdown() = 0;
    
    /**
     * Clear the display
     */
    virtual void clear() = 0;
    
    /**
     * Render a single frame
     */
    virtual void render() = 0;
    
    // ========================================================================
    // Text Display
    // ========================================================================
    
    /**
     * Display a message to the player
     * @param message The message to display
     */
    virtual void displayMessage(const std::string& message) = 0;
    
    /**
     * Display an error message
     * @param error The error message
     */
    virtual void displayError(const std::string& error) = 0;
    
    /**
     * Display a system notification
     * @param notification The notification text
     */
    virtual void displayNotification(const std::string& notification) = 0;
    
    // ========================================================================
    // Dialogue Display
    // ========================================================================
    
    /**
     * Display NPC dialogue
     * @param npcName Name of the speaking NPC
     * @param dialogue The dialogue text
     * @param mood Optional mood indicator
     */
    virtual void displayDialogue(
        const std::string& npcName,
        const std::string& dialogue,
        const std::string& mood = ""
    ) = 0;
    
    /**
     * Display player dialogue options
     * @param options List of options to display
     */
    virtual void displayOptions(const std::vector<std::string>& options) = 0;
    
    /**
     * Display result of player action
     * @param action The action taken
     * @param result The result/consequence
     */
    virtual void displayActionResult(
        const std::string& action,
        const std::string& result
    ) = 0;
    
    // ========================================================================
    // World State Display
    // ========================================================================
    
    /**
     * Display resource status summary
     * @param resources List of resources to display
     */
    virtual void displayResources(
        const std::vector<std::shared_ptr<Resource>>& resources
    ) = 0;
    
    /**
     * Display faction status summary
     * @param factions List of factions to display
     */
    virtual void displayFactions(
        const std::vector<std::shared_ptr<Faction>>& factions
    ) = 0;
    
    /**
     * Display NPC information
     * @param npc The NPC to display info about
     */
    virtual void displayNPCInfo(const NPC& npc) = 0;
    
    /**
     * Display advisor recommendation
     * @param advisorName Name of the advisor
     * @param specialty Their specialty
     * @param advice The advice given
     * @param influence Their influence score
     */
    virtual void displayAdvisorAdvice(
        const std::string& advisorName,
        const std::string& specialty,
        const std::string& advice,
        float influence
    ) = 0;
    
    // ========================================================================
    // Game Time Display
    // ========================================================================
    
    /**
     * Display current game time
     * @param day Current day
     * @param hour Current hour
     * @param season Current season
     * @param year Current year
     */
    virtual void displayGameTime(
        int day,
        int hour,
        const std::string& season,
        int year
    ) = 0;
    
    // ========================================================================
    // Input
    // ========================================================================
    
    /**
     * Display input prompt and get player input
     * @param prompt The prompt to display
     * @return Player's input string
     */
    virtual std::string getPlayerInput(const std::string& prompt = "> ") = 0;
    
    /**
     * Check if renderer supports input
     * @return true if input is supported
     */
    virtual bool supportsInput() const = 0;
    
    // ========================================================================
    // Help System
    // ========================================================================
    
    /**
     * Display help menu
     * @param topic Optional specific topic
     */
    virtual void displayHelp(const std::string& topic = "") = 0;
    
    /**
     * Display available commands
     */
    virtual void displayCommands() = 0;
};

}  // namespace TLS
