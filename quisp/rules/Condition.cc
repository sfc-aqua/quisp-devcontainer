#include "Condition.h"
#include <memory>
#include "Clause.h"
namespace quisp::rules {
void Condition::addClause(std::unique_ptr<Clause> clause) { clauses.push_back(std::move(clause)); }
}  // namespace quisp::rules
