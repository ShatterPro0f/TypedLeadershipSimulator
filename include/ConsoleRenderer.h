#pragma once

#include "IGameRenderer.h"
#include <iostream>
#include <string>
#include <vector>
#include <iomanip>

namespace TLS {

/**
 * @class ConsoleRenderer
 * @brief Text-based console implementation of IGameRenderer
 * 
 * Provides a text-based interface for the game using standard console I/O.
 * Designed for:
 * - Initial development and testing
 * - Accessibility (screen readers)
 * - Low-resource systems
 * - Easy future replacement with 3D renderer
 */
class ConsoleRenderer : public IGameRenderer {
public:
    ConsoleRenderer();
    ~ConsoleRenderer() override;
    
    // ========================================================================
    // Core Display Methods
    // ========================================================================
    
    bool initialize() override;
    void shutdown() override;
    void clear() override;
    void render() override;
    
    // ========================================================================
    // Text Display
    // ========================================================================
    
    void displayMessage(const std::string& message) override;
    void displayError(const std::string& error) override;
    void displayNotification(const std::string& notification) override;
    
    // ========================================================================
    // Dialogue Display
    // ========================================================================
    
    void displayDialogue(
        const std::string& npcName,
        const std::string& dialogue,
        const std::string& mood = ""
    ) override;
    
    void displayOptions(const std::vector<std::string>& options) override;
    
    void displayActionResult(
        const std::string& action,
        const std::string& result
    ) override;
    
    // ========================================================================
    // World State Display
    // ========================================================================
    
    void displayResources(
        const std::vector<std::shared_ptr<Resource>>& resources
    ) override;
    
    void displayFactions(
        const std::vector<std::shared_ptr<Faction>>& factions
    ) override;
    
    void displayNPCInfo(const NPC& npc) override;
    
    void displayAdvisorAdvice(
        const std::string& advisorName,
        const std::string& specialty,
        const std::string& advice,
        float influence
    ) override;
    
    // ========================================================================
    // Game Time Display
    // ========================================================================
    
    void displayGameTime(
        int day,
        int hour,
        const std::string& season,
        int year
    ) override;
    
    // ========================================================================
    // Input
    // ========================================================================
    
    std::string getPlayerInput(const std::string& prompt = "> ") override;
    bool supportsInput() const override { return true; }
    
    // ========================================================================
    // Help System
    // ========================================================================
    
    void displayHelp(const std::string& topic = "") override;
    void displayCommands() override;
    
    // ========================================================================
    // Console-specific Methods
    // ========================================================================
    
    /**
     * Set whether to use color output (ANSI codes)
     * @param enabled true to enable colors
     */
    void setColorEnabled(bool enabled) { m_colorEnabled = enabled; }
    
    /**
     * Set console width for formatting
     * @param width Width in characters
     */
    void setWidth(int width) { m_width = width; }
    
    /**
     * Display a separator line
     */
    void displaySeparator();
    
    /**
     * Display a title banner
     * @param title The title text
     */
    void displayTitle(const std::string& title);
    
    /**
     * Display the game header/welcome
     */
    void displayWelcome();
    
private:
    bool m_initialized;
    bool m_colorEnabled;
    int m_width;
    
    // ANSI color codes (used if colorEnabled)
    static constexpr const char* COLOR_RESET = "\033[0m";
    static constexpr const char* COLOR_RED = "\033[31m";
    static constexpr const char* COLOR_GREEN = "\033[32m";
    static constexpr const char* COLOR_YELLOW = "\033[33m";
    static constexpr const char* COLOR_BLUE = "\033[34m";
    static constexpr const char* COLOR_MAGENTA = "\033[35m";
    static constexpr const char* COLOR_CYAN = "\033[36m";
    static constexpr const char* COLOR_BOLD = "\033[1m";
    
    /**
     * Apply color to text if enabled
     */
    std::string colorize(const std::string& text, const char* color) const;
    
    /**
     * Get mood emoji/indicator
     */
    std::string getMoodIndicator(float mood) const;
    
    /**
     * Get loyalty indicator
     */
    std::string getLoyaltyIndicator(float loyalty) const;
    
    /**
     * Format number with padding
     */
    std::string formatNumber(int value, int width = 6) const;
    
    /**
     * Word wrap text to console width
     */
    std::string wordWrap(const std::string& text, int indent = 0) const;
};

}  // namespace TLS
