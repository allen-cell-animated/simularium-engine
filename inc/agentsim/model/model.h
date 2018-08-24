#ifndef AICS_MODEL_H
#define AICS_MODEL_H

#include <memory>
#include <json/json.h>

namespace aics {
namespace agentsim {

struct Model
{

};

void parse_model(Json::Value& json_obj, aics::agentsim::Model& model);

} // namespace agentsim
} // namespace aics

#endif // AICS_MODEL_H
