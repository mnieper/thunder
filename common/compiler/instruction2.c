










#include "xalloc.h"




Variable *
instruction_dest (Instruction *ins)
{
  return variable_list_first (ins->dests);
}

void instruction_set_dest (Instruction *ins, Variable *var)
{
  var->def = ins;
  variable_list_set_first (ins->dests, var);
}

VariableListPosition *
instruction_add_dest (Instruction *ins, Variable *var)
{
  var->def = ins;
  return variable_list_add (ins->dests, var);
}

VariableListPosition *
instruction_add_source (Instruction *ins, Variable *var)
{
  return variable_list_add (ins->sources, var);
}

void
instruction_replace_source (Instruction *ins, VariableListPosition *pos,
			    Variable *var)
{
  variable_list_replace (ins->sources, pos, var);
}

static Instruction *
create_instruction (void)
{
  Instruction *ins = XMALLOC (Instruction);
  ins->dests = variable_list_create (false);
  ins->sources = variable_list_create (false);
  ins->move = false;
  ins->frob = false;
  ins->phi = false;
  return ins;
}
