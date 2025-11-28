#include "Serialization.h"
#include "Registries.h"
#include <fstream>
#include <iostream>
#include <cstring>
#include <algorithm>
#include <ctime>
#include <cstdint>

namespace TLS {

// ============================================================================
// BinaryWriter Implementation
// ============================================================================

void BinaryWriter::writeUInt32(uint32_t value) {
    buffer_.push_back(static_cast<uint8_t>((value >> 0) & 0xFF));
    buffer_.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
    buffer_.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
    buffer_.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
}

void BinaryWriter::writeInt32(int32_t value) {
    writeUInt32(static_cast<uint32_t>(value));
}

void BinaryWriter::writeFloat(float value) {
    uint32_t bits;
    std::memcpy(&bits, &value, sizeof(float));
    writeUInt32(bits);
}

void BinaryWriter::writeUInt8(uint8_t value) {
    buffer_.push_back(value);
}

void BinaryWriter::writeString(const std::string& str) {
    uint32_t len = static_cast<uint32_t>(str.length());
    writeUInt32(len);
    buffer_.insert(buffer_.end(), str.begin(), str.end());
}

void BinaryWriter::writeVector3(const Vector3& v) {
    writeFloat(v.x);
    writeFloat(v.y);
    writeFloat(v.z);
}

// ============================================================================
// BinaryReader Implementation
// ============================================================================

uint32_t BinaryReader::readUInt32() {
    if (offset_ + 4 > data_.size()) return 0;
    
    uint32_t value = 0;
    value |= static_cast<uint32_t>(data_[offset_++]) << 0;
    value |= static_cast<uint32_t>(data_[offset_++]) << 8;
    value |= static_cast<uint32_t>(data_[offset_++]) << 16;
    value |= static_cast<uint32_t>(data_[offset_++]) << 24;
    
    return value;
}

int32_t BinaryReader::readInt32() {
    return static_cast<int32_t>(readUInt32());
}

float BinaryReader::readFloat() {
    uint32_t bits = readUInt32();
    float value;
    std::memcpy(&value, &bits, sizeof(float));
    return value;
}

uint8_t BinaryReader::readUInt8() {
    if (offset_ >= data_.size()) return 0;
    return data_[offset_++];
}

std::string BinaryReader::readString() {
    uint32_t len = readUInt32();
    if (offset_ + len > data_.size()) return "";
    
    std::string str(data_.begin() + offset_, data_.begin() + offset_ + len);
    offset_ += len;
    return str;
}

Vector3 BinaryReader::readVector3() {
    float x = readFloat();
    float y = readFloat();
    float z = readFloat();
    return Vector3(x, y, z);
}

// ============================================================================
// NPC Serialization
// ============================================================================

std::vector<uint8_t> NPCSerializer::serialize(const NPC& npc) {
    BinaryWriter writer;
    
    // Basic info (32 bytes minimum)
    writer.writeInt32(npc.id_);
    writer.writeString(npc.name_);
    writer.writeInt32(npc.age_);
    writer.writeString(npc.gender_);
    writer.writeString(npc.role_);
    writer.writeInt32(npc.factionId_);
    
    // Stats (20 bytes)
    writer.writeFloat(npc.loyalty_);
    writer.writeFloat(npc.shortTermMood_);
    writer.writeFloat(npc.longTermAttitude_);
    writer.writeFloat(npc.immediateEmotion_);
    writer.writeUInt8(static_cast<uint8_t>(npc.activity_));
    
    // Position and location (24 bytes)
    writer.writeVector3(npc.position_);
    writer.writeVector3(npc.homeLocation_);
    
    // State
    writer.writeInt32(npc.conversationState_);
    writer.writeUInt8(static_cast<uint8_t>(npc.problemSeverity_ > 0.5f ? 1 : 0));
    
    return writer.getBuffer();
}

NPC NPCSerializer::deserialize(const std::vector<uint8_t>& data, size_t& offset) {
    BinaryReader reader(data);
    reader.setOffset(offset);
    
    NPC npc;
    npc.id_ = reader.readInt32();
    npc.name_ = reader.readString();
    npc.age_ = reader.readInt32();
    npc.gender_ = reader.readString();
    npc.role_ = reader.readString();
    npc.factionId_ = reader.readInt32();
    
    npc.loyalty_ = reader.readFloat();
    npc.shortTermMood_ = reader.readFloat();
    npc.longTermAttitude_ = reader.readFloat();
    npc.immediateEmotion_ = reader.readFloat();
    npc.activity_ = static_cast<Activity>(reader.readUInt8());
    
    npc.position_ = reader.readVector3();
    npc.homeLocation_ = reader.readVector3();
    
    npc.conversationState_ = reader.readInt32();
    npc.problemSeverity_ = reader.readUInt8() > 0 ? 0.7f : 0.3f;
    
    offset = reader.getOffset();
    return npc;
}

// ============================================================================
// Advisor Serialization
// ============================================================================

std::vector<uint8_t> AdvisorSerializer::serialize(const Advisor& advisor) {
    BinaryWriter writer;
    
    // NPC base data
    auto npcData = NPCSerializer::serialize(static_cast<const NPC&>(advisor));
    writer.writeUInt32(npcData.size());
    writer.getBuffer().insert(writer.getBuffer().end(), npcData.begin(), npcData.end());
    
    // Advisor-specific data
    writer.writeUInt8(static_cast<uint8_t>(advisor.specialty_));
    writer.writeFloat(advisor.trustLevel_);
    writer.writeFloat(advisor.factionAlignment_);
    writer.writeUInt8(static_cast<uint8_t>(advisor.agenda_));
    writer.writeFloat(advisor.riskTolerance_);
    writer.writeUInt8(static_cast<uint8_t>(advisor.strategyStyle_));
    
    return writer.getBuffer();
}

Advisor AdvisorSerializer::deserialize(const std::vector<uint8_t>& data, size_t& offset) {
    BinaryReader reader(data);
    reader.setOffset(offset);
    
    // Read NPC base data first
    uint32_t npcDataLen = reader.readUInt32();
    std::vector<uint8_t> npcData(data.begin() + reader.getOffset(), 
                                  data.begin() + reader.getOffset() + npcDataLen);
    size_t npcOffset = 0;
    NPC npcBase = NPCSerializer::deserialize(npcData, npcOffset);
    reader.setOffset(reader.getOffset() + npcDataLen);
    
    // Create Advisor from NPC base
    Advisor advisor;
    static_cast<NPC&>(advisor) = npcBase;
    
    // Read Advisor-specific data
    advisor.specialty_ = static_cast<Specialty>(reader.readUInt8());
    advisor.trustLevel_ = reader.readFloat();
    advisor.factionAlignment_ = reader.readFloat();
    advisor.agenda_ = static_cast<Agenda>(reader.readUInt8());
    advisor.riskTolerance_ = reader.readFloat();
    advisor.strategyStyle_ = static_cast<StrategyStyle>(reader.readUInt8());
    
    offset = reader.getOffset();
    return advisor;
}

// ============================================================================
// Resource Serialization
// ============================================================================

std::vector<uint8_t> ResourceSerializer::serialize(const Resource& resource) {
    BinaryWriter writer;
    
    writer.writeInt32(resource.id_);
    writer.writeString(resource.name_);
    writer.writeInt32(resource.quantity_);
    writer.writeInt32(resource.productionRate_);
    writer.writeInt32(resource.consumptionRate_);
    writer.writeInt32(resource.scarcityThreshold_);
    writer.writeVector3(resource.location_);
    
    return writer.getBuffer();
}

Resource ResourceSerializer::deserialize(const std::vector<uint8_t>& data, size_t& offset) {
    BinaryReader reader(data);
    reader.setOffset(offset);
    
    Resource resource;
    resource.id_ = reader.readInt32();
    resource.name_ = reader.readString();
    resource.quantity_ = reader.readInt32();
    resource.productionRate_ = reader.readInt32();
    resource.consumptionRate_ = reader.readInt32();
    resource.scarcityThreshold_ = reader.readInt32();
    resource.location_ = reader.readVector3();
    
    offset = reader.getOffset();
    return resource;
}

// ============================================================================
// Faction Serialization
// ============================================================================

std::vector<uint8_t> FactionSerializer::serialize(const Faction& faction) {
    BinaryWriter writer;
    
    writer.writeInt32(faction.id_);
    writer.writeString(faction.name_);
    writer.writeFloat(faction.strength_);
    writer.writeFloat(faction.emergenceProbability_);
    writer.writeUInt8(static_cast<uint8_t>(faction.alignment_));
    writer.writeVector3(faction.homeLocation_);
    
    // Member IDs
    writer.writeInt32(static_cast<int32_t>(faction.memberIds_.size()));
    for (int memberId : faction.memberIds_) {
        writer.writeInt32(memberId);
    }
    
    // Leader IDs
    writer.writeInt32(static_cast<int32_t>(faction.leaderIds_.size()));
    for (int leaderId : faction.leaderIds_) {
        writer.writeInt32(leaderId);
    }
    
    return writer.getBuffer();
}

Faction FactionSerializer::deserialize(const std::vector<uint8_t>& data, size_t& offset) {
    BinaryReader reader(data);
    reader.setOffset(offset);
    
    Faction faction;
    faction.id_ = reader.readInt32();
    faction.name_ = reader.readString();
    faction.strength_ = reader.readFloat();
    faction.emergenceProbability_ = reader.readFloat();
    faction.alignment_ = static_cast<Alignment>(reader.readUInt8());
    faction.homeLocation_ = reader.readVector3();
    
    // Read member IDs
    int32_t memberCount = reader.readInt32();
    for (int32_t i = 0; i < memberCount; ++i) {
        faction.memberIds_.push_back(reader.readInt32());
    }
    
    // Read leader IDs
    int32_t leaderCount = reader.readInt32();
    for (int32_t i = 0; i < leaderCount; ++i) {
        faction.leaderIds_.push_back(reader.readInt32());
    }
    
    offset = reader.getOffset();
    return faction;
}

// ============================================================================
// Event Serialization
// ============================================================================

std::vector<uint8_t> EventSerializer::serialize(const Event& event) {
    BinaryWriter writer;
    
    writer.writeInt32(event.id_);
    writer.writeString(event.name_);
    writer.writeUInt8(static_cast<uint8_t>(event.type_));
    writer.writeInt32(event.impactLevel_);
    writer.writeVector3(event.location_);
    
    // Affected NPC IDs
    writer.writeInt32(static_cast<int32_t>(event.affectedNPCIds_.size()));
    for (int npcId : event.affectedNPCIds_) {
        writer.writeInt32(npcId);
    }
    
    // Affected Resource IDs
    writer.writeInt32(static_cast<int32_t>(event.affectedResourceIds_.size()));
    for (int resourceId : event.affectedResourceIds_) {
        writer.writeInt32(resourceId);
    }
    
    return writer.getBuffer();
}

Event EventSerializer::deserialize(const std::vector<uint8_t>& data, size_t& offset) {
    BinaryReader reader(data);
    reader.setOffset(offset);
    
    Event event;
    event.id_ = reader.readInt32();
    event.name_ = reader.readString();
    event.type_ = static_cast<EventType>(reader.readUInt8());
    event.impactLevel_ = reader.readInt32();
    event.location_ = reader.readVector3();
    
    // Read affected NPC IDs
    int32_t npcCount = reader.readInt32();
    for (int32_t i = 0; i < npcCount; ++i) {
        event.affectedNPCIds_.push_back(reader.readInt32());
    }
    
    // Read affected Resource IDs
    int32_t resourceCount = reader.readInt32();
    for (int32_t i = 0; i < resourceCount; ++i) {
        event.affectedResourceIds_.push_back(reader.readInt32());
    }
    
    offset = reader.getOffset();
    return event;
}

// ============================================================================
// SaveGameManager Implementation
// ============================================================================

SaveGameManager* SaveGameManager::instance() {
    static SaveGameManager manager;
    return &manager;
}

bool SaveGameManager::saveGameToBinary(const std::string& filePath, const SaveGame& saveData, bool compress) {
    std::ofstream file(filePath, std::ios::binary);
    if (!file) {
        std::cerr << "[SaveGame] Failed to open file for writing: " << filePath << std::endl;
        return false;
    }
    
    BinaryWriter writer;
    
    // Write header
    writer.writeUInt32(saveData.header.magic);
    writer.writeUInt32(saveData.header.formatVersion);
    writer.writeUInt32(saveData.header.timestamp);
    writer.writeUInt32(saveData.header.tickNumber);
    writer.writeUInt32(saveData.header.npcCount);
    writer.writeUInt32(saveData.header.advisorCount);
    writer.writeUInt32(saveData.header.resourceCount);
    writer.writeUInt32(saveData.header.factionCount);
    writer.writeUInt32(saveData.header.eventCount);
    file.write(saveData.header.playerName, 128);
    file.write(saveData.header.gameDescription, 256);
    
    // Write NPCs
    for (const auto& npc : saveData.npcs) {
        auto data = NPCSerializer::serialize(*npc);
        writer.writeInt32(data.size());
        writer.getBuffer().insert(writer.getBuffer().end(), data.begin(), data.end());
    }
    
    // Write Advisors
    for (const auto& advisor : saveData.advisors) {
        auto data = AdvisorSerializer::serialize(*advisor);
        writer.writeInt32(data.size());
        writer.getBuffer().insert(writer.getBuffer().end(), data.begin(), data.end());
    }
    
    // Write Resources
    for (const auto& resource : saveData.resources) {
        auto data = ResourceSerializer::serialize(*resource);
        writer.writeInt32(data.size());
        writer.getBuffer().insert(writer.getBuffer().end(), data.begin(), data.end());
    }
    
    // Write Factions
    for (const auto& faction : saveData.factions) {
        auto data = FactionSerializer::serialize(*faction);
        writer.writeInt32(data.size());
        writer.getBuffer().insert(writer.getBuffer().end(), data.begin(), data.end());
    }
    
    // Write Events
    for (const auto& event : saveData.events) {
        auto data = EventSerializer::serialize(*event);
        writer.writeInt32(data.size());
        writer.getBuffer().insert(writer.getBuffer().end(), data.begin(), data.end());
    }
    
    // Write payload
    auto payload = writer.getBuffer();
    
    if (compress) {
        payload = compressData(payload);
    }
    
    file.write(reinterpret_cast<const char*>(payload.data()), payload.size());
    file.close();
    
    std::cout << "[SaveGame] Successfully saved game to: " << filePath 
              << " (" << payload.size() << " bytes)" << std::endl;
    
    return true;
}

bool SaveGameManager::loadGameFromBinary(const std::string& filePath, SaveGame& saveData, bool decompress) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        std::cerr << "[SaveGame] Failed to open file for reading: " << filePath << std::endl;
        return false;
    }
    
    BinaryReader reader(std::vector<uint8_t>());  // Will be replaced
    
    // Read header
    std::vector<uint8_t> headerData(384);  // Header size
    file.read(reinterpret_cast<char*>(headerData.data()), headerData.size());
    
    BinaryReader headerReader(headerData);
    saveData.header.magic = headerReader.readUInt32();
    saveData.header.formatVersion = headerReader.readUInt32();
    
    if (saveData.header.magic != SaveFileHeader::MAGIC_NUMBER) {
        std::cerr << "[SaveGame] Invalid save file magic number" << std::endl;
        return false;
    }
    
    saveData.header.timestamp = headerReader.readUInt32();
    saveData.header.tickNumber = headerReader.readUInt32();
    saveData.header.npcCount = headerReader.readUInt32();
    saveData.header.advisorCount = headerReader.readUInt32();
    saveData.header.resourceCount = headerReader.readUInt32();
    saveData.header.factionCount = headerReader.readUInt32();
    saveData.header.eventCount = headerReader.readUInt32();
    
    // Read payload (rest of file)
    std::vector<uint8_t> payload((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    
    if (decompress) {
        payload = decompressData(payload);
    }
    
    // Parse payload
    size_t offset = 0;
    BinaryReader payloadReader(payload);
    
    // Deserialize NPCs
    for (uint32_t i = 0; i < saveData.header.npcCount; ++i) {
        int32_t npcDataLen = payloadReader.readInt32();
        std::vector<uint8_t> npcData(payload.begin() + offset + 4, payload.begin() + offset + 4 + npcDataLen);
        size_t npcOffset = 0;
        NPC npc = NPCSerializer::deserialize(npcData, npcOffset);
        saveData.npcs.push_back(std::make_shared<NPC>(npc));
        offset += 4 + npcDataLen;
    }
    
    // Deserialize Advisors
    for (uint32_t i = 0; i < saveData.header.advisorCount; ++i) {
        int32_t advisorDataLen = payloadReader.readInt32();
        std::vector<uint8_t> advisorData(payload.begin() + offset + 4, payload.begin() + offset + 4 + advisorDataLen);
        size_t advisorOffset = 0;
        Advisor advisor = AdvisorSerializer::deserialize(advisorData, advisorOffset);
        saveData.advisors.push_back(std::make_shared<Advisor>(advisor));
        offset += 4 + advisorDataLen;
    }
    
    // Deserialize Resources
    for (uint32_t i = 0; i < saveData.header.resourceCount; ++i) {
        int32_t resourceDataLen = payloadReader.readInt32();
        std::vector<uint8_t> resourceData(payload.begin() + offset + 4, payload.begin() + offset + 4 + resourceDataLen);
        size_t resourceOffset = 0;
        Resource resource = ResourceSerializer::deserialize(resourceData, resourceOffset);
        saveData.resources.push_back(std::make_shared<Resource>(resource));
        offset += 4 + resourceDataLen;
    }
    
    // Deserialize Factions
    for (uint32_t i = 0; i < saveData.header.factionCount; ++i) {
        int32_t factionDataLen = payloadReader.readInt32();
        std::vector<uint8_t> factionData(payload.begin() + offset + 4, payload.begin() + offset + 4 + factionDataLen);
        size_t factionOffset = 0;
        Faction faction = FactionSerializer::deserialize(factionData, factionOffset);
        saveData.factions.push_back(std::make_shared<Faction>(faction));
        offset += 4 + factionDataLen;
    }
    
    // Deserialize Events
    for (uint32_t i = 0; i < saveData.header.eventCount; ++i) {
        int32_t eventDataLen = payloadReader.readInt32();
        std::vector<uint8_t> eventData(payload.begin() + offset + 4, payload.begin() + offset + 4 + eventDataLen);
        size_t eventOffset = 0;
        Event event = EventSerializer::deserialize(eventData, eventOffset);
        saveData.events.push_back(std::make_shared<Event>(event));
        offset += 4 + eventDataLen;
    }
    
    std::cout << "[SaveGame] Successfully loaded game from: " << filePath << std::endl;
    return true;
}

bool SaveGameManager::saveGameToJSON(const std::string& filePath, const SaveGame& saveData) {
    // Stub: in production, use nlohmann::json
    std::cout << "[SaveGame] JSON serialization not implemented (use binary format)" << std::endl;
    return false;
}

bool SaveGameManager::loadGameFromJSON(const std::string& filePath, SaveGame& saveData) {
    // Stub: in production, use nlohmann::json
    std::cout << "[SaveGame] JSON deserialization not implemented (use binary format)" << std::endl;
    return false;
}

bool SaveGameManager::quickSave(const std::string& fileName) {
    SaveGame saveData = createSaveGameFromCurrentState("Player", 0);
    return saveGameToBinary("saves/" + fileName + ".dat", saveData);
}

bool SaveGameManager::quickLoad(const std::string& fileName) {
    SaveGame saveData;
    bool success = loadGameFromBinary("saves/" + fileName + ".dat", saveData);
    if (success) {
        return applySaveGameToCurrentState(saveData);
    }
    return false;
}

SaveGame SaveGameManager::createSaveGameFromCurrentState(const std::string& playerName, int tickNumber) {
    SaveGame saveData;
    
    // Set header
    saveData.header.timestamp = static_cast<uint32_t>(std::time(nullptr));
    saveData.header.tickNumber = tickNumber;
    
    std::strncpy(reinterpret_cast<char*>(saveData.header.playerName), playerName.c_str(), 127);
    std::strncpy(reinterpret_cast<char*>(saveData.header.gameDescription), "Leadership Simulator Save", 255);
    
    // Gather entities from registries
    auto allNPCs = NPCRegistry::getInstance().getAllNPCs();
    auto allAdvisors = AdvisorRegistry::getInstance().getAllAdvisors();
    auto allResources = ResourceRegistry::getInstance().getAllResources();
    auto allFactions = FactionRegistry::getInstance().getAllFactions();
    auto allEvents = EventRegistry::getInstance().getAllEvents();
    
    // Separate advisors from regular NPCs
    for (const auto& npc : allNPCs) {
        saveData.npcs.push_back(npc);
    }
    
    for (const auto& advisor : allAdvisors) {
        saveData.advisors.push_back(advisor);
    }
    
    for (const auto& resource : allResources) {
        saveData.resources.push_back(resource);
    }
    
    for (const auto& faction : allFactions) {
        saveData.factions.push_back(faction);
    }
    
    for (const auto& event : allEvents) {
        saveData.events.push_back(event);
    }
    
    // Update counts in header
    saveData.header.npcCount = saveData.npcs.size();
    saveData.header.advisorCount = saveData.advisors.size();
    saveData.header.resourceCount = saveData.resources.size();
    saveData.header.factionCount = saveData.factions.size();
    saveData.header.eventCount = saveData.events.size();
    
    return saveData;
}

bool SaveGameManager::applySaveGameToCurrentState(const SaveGame& saveData) {
    // Clear current state
    NPCRegistry::getInstance().clear();
    AdvisorRegistry::getInstance().clear();
    ResourceRegistry::getInstance().clear();
    FactionRegistry::getInstance().clear();
    EventRegistry::getInstance().clear();
    
    // Restore entities
    for (const auto& npc : saveData.npcs) {
        NPCRegistry::getInstance().registerNPC(npc);
    }
    
    for (const auto& advisor : saveData.advisors) {
        AdvisorRegistry::getInstance().registerAdvisor(advisor);
    }
    
    for (const auto& resource : saveData.resources) {
        ResourceRegistry::getInstance().registerResource(resource);
    }
    
    for (const auto& faction : saveData.factions) {
        FactionRegistry::getInstance().registerFaction(faction);
    }
    
    for (const auto& event : saveData.events) {
        EventRegistry::getInstance().registerEvent(event);
    }
    
    std::cout << "[SaveGame] Applied save state to current game" << std::endl;
    return true;
}

bool SaveGameManager::migrateV1_to_V2(const SaveGame& oldData, SaveGame& newData) {
    std::cout << "[SaveGame] Migrating from V1 to V2" << std::endl;
    // Migration logic here
    newData = oldData;
    newData.header.formatVersion = 2;
    MigrationLog::recordMigration({1, 2, "Auto-migration from V1 to V2", static_cast<uint32_t>(std::time(nullptr))});
    return true;
}

bool SaveGameManager::saveFileExists(const std::string& filePath) const {
    std::ifstream f(filePath);
    return f.good();
}

std::string SaveGameManager::getLatestSaveFile() const {
    // Stub: in production, check timestamps
    return "auto_save.dat";
}

std::vector<std::string> SaveGameManager::listSaveFiles() const {
    // Stub: in production, scan directory
    return {"save_slot_1.dat", "save_slot_2.dat", "auto_save.dat"};
}

std::vector<uint8_t> SaveGameManager::compressData(const std::vector<uint8_t>& data) {
    // Stub: in production, use zlib or snappy
    return data;
}

std::vector<uint8_t> SaveGameManager::decompressData(const std::vector<uint8_t>& data) {
    // Stub: in production, use zlib or snappy
    return data;
}

// ============================================================================
// MigrationLog Implementation
// ============================================================================

bool MigrationLog::recordMigration(const Entry& entry) {
    std::cout << "[Migration] V" << entry.fromVersion << " -> V" << entry.toVersion 
              << ": " << entry.description << std::endl;
    return true;
}

std::vector<MigrationLog::Entry> MigrationLog::getMigrationHistory() {
    return {};  // Stub
}

}  // namespace TLS
