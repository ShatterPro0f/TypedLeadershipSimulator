#pragma once

#include <string>
#include <vector>

namespace TLS {

class Resource {
public:
    Resource() : id_(-1), name_(""), quantity_(0), production_rate_(0), consumption_rate_(0) {}
    
    int getId() const { return id_; }
    const std::string& getName() const { return name_; }
    int getQuantity() const { return quantity_; }
    int getProductionRate() const { return production_rate_; }
    int getConsumptionRate() const { return consumption_rate_; }
    
    void setId(int id) { id_ = id; }
    void setName(const std::string& name) { name_ = name; }
    void setQuantity(int quantity) { quantity_ = quantity; }
    void setProductionRate(int rate) { production_rate_ = rate; }
    void setConsumptionRate(int rate) { consumption_rate_ = rate; }

private:
    int id_;
    std::string name_;
    int quantity_;
    int production_rate_;
    int consumption_rate_;
};

}  // namespace TLS
