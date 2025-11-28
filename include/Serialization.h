#ifndef TLS_SERIALIZATION_H
#define TLS_SERIALIZATION_H

#include <string>
#include <vector>
#include <cstdint>
#include <memory>
#include "Core.h"

namespace TLS {

// ============================================================================
// Binary Serialization System
// ============================================================================

struct SaveFileHeader {
    static constexpr uint32_t MAGIC_NUMBER = 0x544C5332;  // "TLS2" in hex
    static constexpr uint32_t FORMAT_VERSION = 2;
    
    uint32_t magic = MAGIC_NUMBER;
    uint32_t formatVersion = FORMAT_VERSION;
    uint32_t timestamp = 0;
    uint32_t tickNumber = 0;
    uint32_t npcCount = 0;
    uint32_t advisorCount = 0;
    uint32_t resourceCount = 0;
    uint32_t factionCount = 0;
    uint32_t eventCount = 0;
    char playerName[128] = {};
    char gameDescription[256] = {};
};

// ============================================================================
// Binary Serialization Base Class
// ============================================================================

class ISerializable {
public:
    virtual ~ISerializable() = default;
    
    // Write to binary buffer
    virtual std::vector<uint8_t> toBinary() const = 0;
    
    // Read from binary buffer
    virtual bool fromBinary(const std::vector<uint8_t>& data, size_t& offset) = 0;
};

// ============================================================================
// Serialization Helpers
// ============================================================================

class BinaryWriter {
public:
    BinaryWriter() = default;
    
    // Write primitive types
    void writeUInt32(uint32_t value);
    void writeInt32(int32_t value);
    void writeFloat(float value);
    void writeUInt8(uint8_t value);
    void writeString(const std::string& str);
    void writeVector3(const Vector3& v);
    
    // Get resulting buffer
    std::vector<uint8_t> getBuffer() const { return buffer_; }
    void clear() { buffer_.clear(); }
    
private:
    std::vector<uint8_t> buffer_;
};

class BinaryReader {
public:
    explicit BinaryReader(const std::vector<uint8_t>& data) : data_(data), offset_(0) {}
    
    // Read primitive types
    uint32_t readUInt32();
    int32_t readInt32();
    float readFloat();
    uint8_t readUInt8();
    std::string readString();
    Vector3 readVector3();
    
    // Position tracking
    size_t getOffset() const { return offset_; }
    void setOffset(size_t offset) { offset_ = offset; }
    bool isValid() const { return offset_ <= data_.size(); }
    
private:
    const std::vector<uint8_t>& data_;
    size_t offset_ = 0;
};

// ============================================================================
// NPC Serialization Specialization
// ============================================================================

class NPCSerializer {
public:
    static std::vector<uint8_t> serialize(const NPC& npc);
    static NPC deserialize(const std::vector<uint8_t>& data, size_t& offset);
};

class AdvisorSerializer {
public:
    static std::vector<uint8_t> serialize(const Advisor& advisor);
    static Advisor deserialize(const std::vector<uint8_t>& data, size_t& offset);
};

class ResourceSerializer {
public:
    static std::vector<uint8_t> serialize(const Resource& resource);
    static Resource deserialize(const std::vector<uint8_t>& data, size_t& offset);
};

class FactionSerializer {
public:
    static std::vector<uint8_t> serialize(const Faction& faction);
    static Faction deserialize(const std::vector<uint8_t>& data, size_t& offset);
};

class EventSerializer {
public:
    static std::vector<uint8_t> serialize(const Event& event);
    static Event deserialize(const std::vector<uint8_t>& data, size_t& offset);
};

// ============================================================================
// SaveGame System
// ============================================================================

struct SaveGame {
    SaveFileHeader header;
    std::vector<std::shared_ptr<NPC>> npcs;
    std::vector<std::shared_ptr<Advisor>> advisors;
    std::vector<std::shared_ptr<Resource>> resources;
    std::vector<std::shared_ptr<Faction>> factions;
    std::vector<std::shared_ptr<Event>> events;
};

class SaveGameManager {
public:
    static SaveGameManager* instance();
    
    // Saving
    bool saveGameToBinary(const std::string& filePath, const SaveGame& saveData, bool compress = false);
    bool saveGameToJSON(const std::string& filePath, const SaveGame& saveData);  // For debugging/editing
    
    // Loading
    bool loadGameFromBinary(const std::string& filePath, SaveGame& saveData, bool decompress = false);
    bool loadGameFromJSON(const std::string& filePath, SaveGame& saveData);
    
    // Quick save/load using current game state
    bool quickSave(const std::string& fileName);
    bool quickLoad(const std::string& fileName);
    
    // Game state snapshot
    SaveGame createSaveGameFromCurrentState(const std::string& playerName, int tickNumber);
    bool applySaveGameToCurrentState(const SaveGame& saveData);
    
    // Version migration
    bool migrateV1_to_V2(const SaveGame& oldData, SaveGame& newData);
    
    // File management
    bool saveFileExists(const std::string& filePath) const;
    std::string getLatestSaveFile() const;
    std::vector<std::string> listSaveFiles() const;
    
private:
    SaveGameManager() = default;
    
    // Helper functions
    std::vector<uint8_t> compressData(const std::vector<uint8_t>& data);
    std::vector<uint8_t> decompressData(const std::vector<uint8_t>& data);
};

// ============================================================================
// Save File Format Documentation
// ============================================================================

/*
 * Binary Save Format (*.dat):
 * 
 * [SaveFileHeader]
 *   - Magic: 0x544C5332 ("TLS2")
 *   - Format Version: 2
 *   - Timestamp, Tick Number
 *   - Entity counts
 *   - Player name, description
 *
 * [NPCs] × npcCount
 *   - Each NPC serialized as binary (ID, name, stats, position, etc.)
 *   - Size: ~50 bytes per NPC (vs 500+ bytes in JSON)
 *
 * [Advisors] × advisorCount
 *   - Each Advisor serialized (includes NPC data + specialty, trust, etc.)
 *   - Size: ~70 bytes per Advisor
 *
 * [Resources] × resourceCount
 *   - Each Resource serialized (name, quantity, rates, threshold)
 *   - Size: ~30 bytes per Resource
 *
 * [Factions] × factionCount
 *   - Each Faction serialized (name, members, strength, location)
 *   - Size: ~100 bytes + 4 bytes per member
 *
 * [Events] × eventCount
 *   - Each Event serialized (name, type, impact, affected entities)
 *   - Size: ~40 bytes + 8 bytes per affected entity
 *
 * Optional: [Compressed Data] (if compression enabled)
 *   - Entire payload after header compressed with zlib/snappy
 *   - Can reduce 50KB save to 10-15KB
 */

// ============================================================================
// Migration System (for format updates)
// ============================================================================

class MigrationLog {
public:
    struct Entry {
        int fromVersion = 0;
        int toVersion = 0;
        std::string description;
        uint32_t timestamp = 0;
    };
    
    static bool recordMigration(const Entry& entry);
    static std::vector<Entry> getMigrationHistory();
};

}  // namespace TLS

#endif  // TLS_SERIALIZATION_H
