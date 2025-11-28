#include "ConsoleRenderer.h"
#include <sstream>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#endif

namespace TLS {

// ============================================================================
// Constructor/Destructor
// ============================================================================

ConsoleRenderer::ConsoleRenderer()
    : m_initialized(false)
    , m_colorEnabled(true)
    , m_width(80)
{
}

ConsoleRenderer::~ConsoleRenderer() {
    if (m_initialized) {
        shutdown();
    }
}

// ============================================================================
// Core Display Methods
// ============================================================================

bool ConsoleRenderer::initialize() {
    if (m_initialized) {
        return true;
    }
    
#ifdef _WIN32
    // Enable ANSI escape sequences on Windows 10+
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD dwMode = 0;
        if (GetConsoleMode(hOut, &dwMode)) {
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hOut, dwMode);
        }
    }
    
    // Set console to UTF-8 for better character support
    SetConsoleOutputCP(CP_UTF8);
#endif
    
    m_initialized = true;
    return true;
}

void ConsoleRenderer::shutdown() {
    if (!m_initialized) {
        return;
    }
    
    std::cout << std::endl;
    displayMessage("Game shutting down...");
    m_initialized = false;
}

void ConsoleRenderer::clear() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void ConsoleRenderer::render() {
    // Console renderer doesn't have a frame-based render loop
    // All output is immediate
    std::cout.flush();
}

// ============================================================================
// Text Display
// ============================================================================

void ConsoleRenderer::displayMessage(const std::string& message) {
    std::cout << message << std::endl;
}

void ConsoleRenderer::displayError(const std::string& error) {
    std::cout << colorize("[ERROR] ", COLOR_RED) << error << std::endl;
}

void ConsoleRenderer::displayNotification(const std::string& notification) {
    std::cout << colorize("[!] ", COLOR_YELLOW) << notification << std::endl;
}

// ============================================================================
// Dialogue Display
// ============================================================================

void ConsoleRenderer::displayDialogue(
    const std::string& npcName,
    const std::string& dialogue,
    const std::string& mood
) {
    std::cout << std::endl;
    std::cout << colorize("[", COLOR_CYAN) 
              << colorize(npcName, COLOR_BOLD)
              << colorize("]", COLOR_CYAN);
    
    if (!mood.empty()) {
        std::cout << " (" << mood << ")";
    }
    std::cout << std::endl;
    
    std::cout << "  \"" << wordWrap(dialogue, 3) << "\"" << std::endl;
    std::cout << std::endl;
}

void ConsoleRenderer::displayOptions(const std::vector<std::string>& options) {
    std::cout << std::endl;
    std::cout << colorize("Options:", COLOR_BOLD) << std::endl;
    
    for (size_t i = 0; i < options.size(); ++i) {
        std::cout << "  [" << (i + 1) << "] " << options[i] << std::endl;
    }
    std::cout << std::endl;
}

void ConsoleRenderer::displayActionResult(
    const std::string& action,
    const std::string& result
) {
    std::cout << std::endl;
    displaySeparator();
    std::cout << colorize("[RESULT]", COLOR_GREEN) << " " << action << std::endl;
    std::cout << result << std::endl;
    displaySeparator();
    std::cout << std::endl;
}

// ============================================================================
// World State Display
// ============================================================================

void ConsoleRenderer::displayResources(
    const std::vector<std::shared_ptr<Resource>>& resources
) {
    std::cout << std::endl;
    displayTitle("RESOURCES");
    
    std::cout << std::left 
              << std::setw(15) << "Resource"
              << std::setw(10) << "Qty"
              << std::setw(12) << "Production"
              << std::setw(12) << "Consumption"
              << std::setw(10) << "Status"
              << std::endl;
    
    displaySeparator();
    
    for (const auto& res : resources) {
        std::string status;
        if (res->isScarse()) {
            status = colorize("SCARCE", COLOR_RED);
        } else if (res->getQuantity() < res->getScarcityThreshold() * 1.5) {
            status = colorize("LOW", COLOR_YELLOW);
        } else {
            status = colorize("OK", COLOR_GREEN);
        }
        
        std::cout << std::left
                  << std::setw(15) << res->getName()
                  << std::setw(10) << res->getQuantity()
                  << std::setw(12) << ("+" + std::to_string(res->getProductionRate()))
                  << std::setw(12) << ("-" + std::to_string(res->getConsumptionRate()))
                  << status
                  << std::endl;
    }
    std::cout << std::endl;
}

void ConsoleRenderer::displayFactions(
    const std::vector<std::shared_ptr<Faction>>& factions
) {
    std::cout << std::endl;
    displayTitle("FACTIONS");
    
    for (const auto& faction : factions) {
        std::string alignStr;
        switch (faction->getAlignment()) {
            case Alignment::PLAYER_FRIENDLY: 
                alignStr = colorize("Friendly", COLOR_GREEN); 
                break;
            case Alignment::NEUTRAL: 
                alignStr = colorize("Neutral", COLOR_YELLOW); 
                break;
            case Alignment::HOSTILE: 
                alignStr = colorize("Hostile", COLOR_RED); 
                break;
        }
        
        std::cout << colorize(faction->getName(), COLOR_BOLD) 
                  << " (" << alignStr << ")" << std::endl;
        std::cout << "  Members: " << faction->getMemberCount()
                  << " | Strength: " << std::fixed << std::setprecision(2) 
                  << faction->getStrength() << std::endl;
    }
    std::cout << std::endl;
}

void ConsoleRenderer::displayNPCInfo(const NPC& npc) {
    std::cout << std::endl;
    displaySeparator();
    std::cout << colorize(npc.getName(), COLOR_BOLD) 
              << " (" << npc.getRole() << ")" << std::endl;
    
    std::cout << "  Age: " << npc.getAge() 
              << " | Gender: " << npc.getGender() << std::endl;
    
    std::cout << "  Mood: " << getMoodIndicator(npc.getShortTermMood())
              << " | Loyalty: " << getLoyaltyIndicator(npc.getLoyalty()) << std::endl;
    
    // Show problem description if set
    if (!npc.getProblemDescription().empty()) {
        std::cout << "  Issue: " << wordWrap(npc.getProblemDescription(), 9) << std::endl;
    }
    
    displaySeparator();
    std::cout << std::endl;
}

void ConsoleRenderer::displayAdvisorAdvice(
    const std::string& advisorName,
    const std::string& specialty,
    const std::string& advice,
    float influence
) {
    std::cout << std::endl;
    std::cout << colorize("[ADVISOR]", COLOR_MAGENTA) << " "
              << colorize(advisorName, COLOR_BOLD) 
              << " (" << specialty << ", influence: " 
              << std::fixed << std::setprecision(1) << influence << ")" 
              << std::endl;
    std::cout << "  \"" << wordWrap(advice, 3) << "\"" << std::endl;
    std::cout << std::endl;
}

// ============================================================================
// Game Time Display
// ============================================================================

void ConsoleRenderer::displayGameTime(
    int day,
    int hour,
    const std::string& season,
    int year
) {
    std::cout << colorize("[", COLOR_BLUE) 
              << season << ", Year " << year 
              << " - Day " << day << ", " 
              << std::setw(2) << std::setfill('0') << hour << ":00"
              << colorize("]", COLOR_BLUE) << std::setfill(' ') << std::endl;
}

// ============================================================================
// Input
// ============================================================================

std::string ConsoleRenderer::getPlayerInput(const std::string& prompt) {
    std::cout << colorize(prompt, COLOR_CYAN);
    std::string input;
    std::getline(std::cin, input);
    return input;
}

// ============================================================================
// Help System
// ============================================================================

void ConsoleRenderer::displayHelp(const std::string& topic) {
    std::cout << std::endl;
    displayTitle("HELP");
    
    if (topic.empty() || topic == "general") {
        std::cout << "Welcome to Typed Leadership Simulator!" << std::endl;
        std::cout << std::endl;
        std::cout << "You are the leader of a growing settlement. Your decisions" << std::endl;
        std::cout << "affect the lives of your people, the strength of factions," << std::endl;
        std::cout << "and the prosperity of your community." << std::endl;
        std::cout << std::endl;
        std::cout << "Type commands naturally, like:" << std::endl;
        std::cout << "  - \"allocate more food to the farmers\"" << std::endl;
        std::cout << "  - \"speak with the blacksmith\"" << std::endl;
        std::cout << "  - \"check resources\"" << std::endl;
        std::cout << "  - \"ask advisor for advice\"" << std::endl;
        std::cout << std::endl;
        std::cout << "Type 'commands' for a list of available commands." << std::endl;
    } else if (topic == "resources") {
        std::cout << "Resources are the lifeblood of your settlement." << std::endl;
        std::cout << std::endl;
        std::cout << "Each resource has:" << std::endl;
        std::cout << "  - Quantity: Current amount available" << std::endl;
        std::cout << "  - Production: Amount gained per day" << std::endl;
        std::cout << "  - Consumption: Amount used per day" << std::endl;
        std::cout << "  - Scarcity Threshold: Below this, people worry" << std::endl;
    } else if (topic == "factions") {
        std::cout << "Factions represent groups within your settlement." << std::endl;
        std::cout << std::endl;
        std::cout << "Each faction has:" << std::endl;
        std::cout << "  - Alignment: Friendly, Neutral, or Hostile toward you" << std::endl;
        std::cout << "  - Strength: Their power and influence" << std::endl;
        std::cout << "  - Members: NPCs who belong to the faction" << std::endl;
    } else {
        std::cout << "Unknown help topic: " << topic << std::endl;
        std::cout << "Available topics: general, resources, factions" << std::endl;
    }
    std::cout << std::endl;
}

void ConsoleRenderer::displayCommands() {
    std::cout << std::endl;
    displayTitle("COMMANDS");
    
    std::cout << "Game Commands:" << std::endl;
    std::cout << "  status        - View settlement overview" << std::endl;
    std::cout << "  resources     - View resource status" << std::endl;
    std::cout << "  factions      - View faction status" << std::endl;
    std::cout << "  people        - List NPCs in settlement" << std::endl;
    std::cout << "  advisors      - Consult your advisors" << std::endl;
    std::cout << std::endl;
    
    std::cout << "Actions:" << std::endl;
    std::cout << "  allocate      - Distribute resources" << std::endl;
    std::cout << "  speak [name]  - Talk to an NPC" << std::endl;
    std::cout << "  wait          - Advance time" << std::endl;
    std::cout << std::endl;
    
    std::cout << "System:" << std::endl;
    std::cout << "  help [topic]  - Show help" << std::endl;
    std::cout << "  commands      - Show this list" << std::endl;
    std::cout << "  save          - Save game" << std::endl;
    std::cout << "  quit          - Exit game" << std::endl;
    std::cout << std::endl;
}

// ============================================================================
// Console-specific Methods
// ============================================================================

void ConsoleRenderer::displaySeparator() {
    std::cout << std::string(m_width, '-') << std::endl;
}

void ConsoleRenderer::displayTitle(const std::string& title) {
    displaySeparator();
    
    int padding = (m_width - static_cast<int>(title.length()) - 4) / 2;
    std::cout << std::string(padding, ' ') 
              << colorize("[ " + title + " ]", COLOR_BOLD) 
              << std::endl;
    
    displaySeparator();
}

void ConsoleRenderer::displayWelcome() {
    clear();
    
    std::cout << std::endl;
    std::cout << colorize("╔══════════════════════════════════════════════════════════════╗", COLOR_CYAN) << std::endl;
    std::cout << colorize("║                                                              ║", COLOR_CYAN) << std::endl;
    std::cout << colorize("║         ", COLOR_CYAN) 
              << colorize("TYPED LEADERSHIP SIMULATOR", COLOR_BOLD) 
              << colorize("                       ║", COLOR_CYAN) << std::endl;
    std::cout << colorize("║                                                              ║", COLOR_CYAN) << std::endl;
    std::cout << colorize("║   Lead your settlement through prosperity and crisis.        ║", COLOR_CYAN) << std::endl;
    std::cout << colorize("║   Your words shape the world. Your decisions matter.         ║", COLOR_CYAN) << std::endl;
    std::cout << colorize("║                                                              ║", COLOR_CYAN) << std::endl;
    std::cout << colorize("╚══════════════════════════════════════════════════════════════╝", COLOR_CYAN) << std::endl;
    std::cout << std::endl;
}

// ============================================================================
// Private Helper Methods
// ============================================================================

std::string ConsoleRenderer::colorize(const std::string& text, const char* color) const {
    if (!m_colorEnabled) {
        return text;
    }
    return std::string(color) + text + COLOR_RESET;
}

std::string ConsoleRenderer::getMoodIndicator(float mood) const {
    if (mood >= 0.8f) return colorize("Excellent", COLOR_GREEN);
    if (mood >= 0.6f) return colorize("Good", COLOR_GREEN);
    if (mood >= 0.4f) return colorize("Neutral", COLOR_YELLOW);
    if (mood >= 0.2f) return colorize("Poor", COLOR_RED);
    return colorize("Terrible", COLOR_RED);
}

std::string ConsoleRenderer::getLoyaltyIndicator(float loyalty) const {
    if (loyalty >= 0.8f) return colorize("Devoted", COLOR_GREEN);
    if (loyalty >= 0.6f) return colorize("Loyal", COLOR_GREEN);
    if (loyalty >= 0.4f) return colorize("Neutral", COLOR_YELLOW);
    if (loyalty >= 0.2f) return colorize("Disloyal", COLOR_RED);
    return colorize("Hostile", COLOR_RED);
}

std::string ConsoleRenderer::formatNumber(int value, int width) const {
    std::ostringstream oss;
    oss << std::setw(width) << value;
    return oss.str();
}

std::string ConsoleRenderer::wordWrap(const std::string& text, int indent) const {
    std::string result;
    std::string line;
    std::istringstream iss(text);
    std::string word;
    
    int maxWidth = m_width - indent;
    std::string indentStr(indent, ' ');
    bool firstLine = true;
    
    while (iss >> word) {
        if (static_cast<int>(line.length() + word.length() + 1) > maxWidth) {
            if (!result.empty()) result += "\n" + indentStr;
            result += line;
            line = word;
            firstLine = false;
        } else {
            if (!line.empty()) line += " ";
            line += word;
        }
    }
    
    if (!line.empty()) {
        if (!result.empty()) result += "\n" + indentStr;
        result += line;
    }
    
    return result;
}

}  // namespace TLS
