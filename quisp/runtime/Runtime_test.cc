#include "Runtime.h"
#include <gtest/gtest.h>
#include <modules/QRSA/RuleEngine/QubitRecord/QubitRecord.h>
#include "modules/QRSA/QRSA.h"
#include "rules/Active/ActiveRuleSet.h"
#include "runtime/InstructionVisitor.h"
#include "runtime/types.h"
#include "test.h"

using namespace quisp::runtime;
using namespace quisp::rules;
using namespace quisp::rules::active;
using namespace quisp_test;
using quisp::modules::qubit_record::QubitRecord;
namespace {

class RuntimeTest : public testing::Test {
 protected:
  void SetUp() {
    runtime = new Runtime();
    runtime->callback = new MockRuntimeCallback();
    runtime->rule_id = 0;
    runtime->cleanup();
    qubit = new QubitRecord{QNIC_E, 2, 3};
    qubit2 = new QubitRecord{QNIC_E, 2, 4};
    qubit3 = new QubitRecord{QNIC_E, 2, 5};
  }

  void TearDown() { delete runtime; }

 public:
  Runtime* runtime;
  qrsa::IQubitRecord* qubit;
  qrsa::IQubitRecord* qubit2;
  qrsa::IQubitRecord* qubit3;
};

TEST_F(RuntimeTest, initialize) { ASSERT_NE(runtime, nullptr); }

TEST_F(RuntimeTest, execSimpleRuleSet) {
  RuleSet rs{"1st ruleset",
             {
                 Rule{Program{"cond2",
                              {
                                  // clang-format off
INSTR_SET_RegId_int_{{RegId::REG0, 3}},
INSTR_DEBUG_RegId_{RegId::REG0},
INSTR_ADD_RegId_RegId_RegId_{{RegId::REG0, RegId::REG0, RegId::REG0}},
INSTR_DEBUG_RegId_{RegId::REG0},
INSTR_ADD_RegId_RegId_int_{{RegId::REG0, RegId::REG0, 1}},
INSTR_DEBUG_RegId_{RegId::REG0},
                                  // clang-format on
                              }},
                      Program{"cond2", {}}},

             }};
  rs.finalize();
  runtime->exec(std::move(rs));
}

TEST_F(RuntimeTest, evalQubitIdOperation) {
  QubitId q0{0};
  auto count = RegId::REG0;
  QNodeAddr partner_addr{1};
  auto qubit_index = 0;  // former 'resource'
  runtime->debugging = false;
  Program program{
      "RandomMeasureAction",
      {
          // clang-format off
INSTR_LOAD_RegId_MemoryKey_{{count, MemoryKey{"count"}}},
INSTR_GET_QUBIT_QubitId_QNodeAddr_int_{{q0, partner_addr, qubit_index}},
INSTR_BNERR_Label_{Label{"L1"}},
INSTR_ERROR_String_{"Qubit not found for mesaurement"},
INSTR_MEASURE_RANDOM_MemoryKey_QubitId_{{MemoryKey{"outcome"}, q0}, "L1"},
INSTR_INC_RegId_{count},
INSTR_STORE_MemoryKey_RegId_{{MemoryKey{"count"}, count}},
INSTR_FREE_QUBIT_QubitId_{q0},
          // clang-format on
      }};
  runtime->assignQubitToRule(partner_addr, runtime->rule_id, qubit);
  runtime->eval(program);
}

TEST_F(RuntimeTest, jump) {
  auto r0 = RegId::REG0;
  Program program{"JumpTest",
                  {
                      // clang-format off
INSTR_SET_RegId_int_{{r0, 10}},
INSTR_JMP_Label_{Label{"test"}},
INSTR_ADD_RegId_RegId_int_{{r0, r0, 1}},
INSTR_NOP_None_{nullptr, "test"}
                      // clang-format on
                  }};
  runtime->cleanup();
  runtime->eval(program);
  EXPECT_EQ(runtime->getRegVal(r0), 10);
}

TEST_F(RuntimeTest, branch_if_no_error) {
  auto r0 = RegId::REG0;
  Program program{"BNErrTest",
                  {
                      // clang-format off
INSTR_SET_RegId_int_{{r0, 10}},
INSTR_BNERR_Label_{Label{"test"}},
// skip until "test" label
INSTR_ADD_RegId_RegId_int_{{r0, r0, 3}},
INSTR_NOP_None_{nullptr, "test"}
                      // clang-format on
                  }};
  runtime->cleanup();
  runtime->eval(program);
  EXPECT_EQ(runtime->getRegVal(r0), 10);
}

TEST_F(RuntimeTest, memoryOperations) {
  auto r0 = RegId::REG0;
  Program program{"MemoryTest",
                  {
                      // clang-format off
INSTR_SET_RegId_int_{{r0, 10}},
INSTR_STORE_MemoryKey_RegId_{{MemoryKey{"count"}, r0}, "INIT"},
INSTR_SUB_RegId_RegId_int_{{r0, r0, 1}},
INSTR_BNZ_Label_RegId_{{Label{"INIT"}, r0}},
INSTR_LOAD_RegId_MemoryKey_{{r0,MemoryKey{ "count"}}}
                      // clang-format on
                  }};
  runtime->cleanup();
  runtime->eval(program);
  EXPECT_EQ(runtime->getRegVal(r0), 1);
}

TEST_F(RuntimeTest, getMultipleQubits) {
  QubitId q0{0};
  QubitId q1{1};
  QNodeAddr partner_addr{1};
  runtime->debugging = true;
  Program program{"get multiple qubits",
                  {
                      // clang-format off
INSTR_GET_QUBIT_QubitId_QNodeAddr_int_{{q0, partner_addr, 0}},
INSTR_GET_QUBIT_QubitId_QNodeAddr_int_{{q1, partner_addr, 1}},
                      // clang-format on
                  }};
  runtime->assignQubitToRule(partner_addr, runtime->rule_id, qubit);
  runtime->assignQubitToRule(partner_addr, runtime->rule_id + 1, qubit2);
  runtime->assignQubitToRule(partner_addr, runtime->rule_id, qubit3);
  runtime->eval(program);

  EXPECT_EQ(runtime->getQubitByQubitId(q0), qubit);
}
}  // namespace
