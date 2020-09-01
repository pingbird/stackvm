#include <map>
#include <unordered_set>

#include "../opt.h"

using namespace IR;

struct ResolutionState {
  std::unordered_set<Inst*> phis;
};

TypeId getType(ResolutionState &state, Inst *inst);

static TypeId rawGetType(ResolutionState &state, Inst *inst) {
  switch (inst->kind) {
    case I_NOP:
    case I_SETREG:
    case I_IF:
    case I_GOTO:
    case I_RET:
    case I_PUTCHAR:
    case I_STR:
      return T_NONE;
    case I_IMM:
      abort(); // Given type by builder
    case I_ADD:
    case I_SUB: {
      TypeId ltype = getType(state, inst->inputs[0]);
      if (ltype == T_INVALID) {
        return T_INVALID;
      }
      assert(ltype != T_NONE);
      TypeId rtype = getType(state, inst->inputs[1]);
      if (rtype == T_INVALID) {
        return T_INVALID;
      }
      assert(rtype != T_NONE);
      return maxType(ltype, rtype);
    } case I_GEP:
      return T_PTR;
    case I_LD:
    case I_GETCHAR:
      return typeForSize(inst->block->graph->config.cellSize);
    case I_REG:
      switch (inst->immReg) {
        case R_PTR:
          return T_PTR;
      }
    case I_PHI: {
      if (state.phis.count(inst)) {
        return T_INVALID;
      }
      state.phis.insert(inst);
      TypeId type = T_INVALID;
      for (Inst *input : inst->inputs) {
        TypeId inputType = getType(state, input);
        assert(inputType != T_NONE);
        if (inputType == T_INVALID) continue;
        if (type == T_INVALID) {
          type = inputType;
        } else {
          type = maxType(type, inputType);
        }
      }
      state.phis.erase(inst);
      return type;
    }
  }
  abort();
}

TypeId getType(ResolutionState &state, Inst *inst) {
  if (inst->type == T_INVALID) {
    inst->type = rawGetType(state, inst);
  }
  return inst->type;
}

TypeId Opt::resolveType(Inst *inst) {
  ResolutionState state;
  TypeId result = getType(state, inst);
  assert(result != T_INVALID);
  return result;
}