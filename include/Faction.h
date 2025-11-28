#pragma once

#include <string>
#include <vector>

namespace TLS {

class Faction {
public:
    Faction() : id_(-1), name_(""), strength_(0.5f), alignment_(0) {}
    
    int getId() const { return id_; }
    const std::string& getName() const { return name_; }
    float getStrength() const { return strength_; }
    int getAlignment() const { return alignment_; }
    const std::vector<int>& getMemberIds() const { return member_ids_; }
    
    void setId(int id) { id_ = id; }
    void setName(const std::string& name) { name_ = name; }
    void setStrength(float strength) { strength_ = strength; }
    void setAlignment(int alignment) { alignment_ = alignment; }
    void addMember(int npc_id) { member_ids_.push_back(npc_id); }

private:
    int id_;
    std::string name_;
    float strength_;
    int alignment_;
    std::vector<int> member_ids_;
};

}  // namespace TLS
