import collections
import os
import random
import sys

from pysmt.shortcuts import get_model, Solver, And, Not, is_sat
from pysmt.smtlib.parser import SmtLibParser
from six.moves import cStringIO


def collect_symbolic_path(log_path, project_path):
    ppc_list = collections.OrderedDict()
    last_sym_path = ""
    if os.path.exists(log_path):
        source_path = ""
        path_condition = ""
        with open(log_path, 'r') as trace_file:
            for line in trace_file:
                if '[path:ppc]' in line:
                    if project_path in line:
                        source_path = str(line.replace("[path:ppc]", '')).split(" : ")[0]
                        source_path = source_path.strip()
                        source_path = os.path.abspath(source_path)
                        path_condition = str(line.replace("[path:ppc]", '')).split(" : ")[1]
                        continue
                if source_path:
                    if "(exit)" not in line:
                        path_condition = path_condition + line
                    else:
                        if source_path not in ppc_list.keys():
                            ppc_list[source_path] = list()
                        ppc_list[source_path].append((path_condition))
                        last_sym_path = path_condition
                        source_path = ""
                        path_condition = ""
    # constraints['last-sym-path'] = last_sym_path
    # print(constraints.keys())
    return ppc_list, last_sym_path


def analyse_symbolic_path(ppc_list):
    constraint_list = dict()
    for control_loc in reversed(ppc_list):
        ppc = ppc_list[control_loc]
        ppc = "".join(ppc)
        parser = SmtLibParser()
        script = parser.get_script(cStringIO(ppc))
        formula = script.get_last_formula()
        constraint = formula.arg(1)
        print(control_loc, constraint)
        if control_loc not in constraint_list:
            constraint_list[control_loc] = list()
        constraint_list[control_loc].append(constraint)
    return constraint_list


def generate_new_symbolic_path(constraint_list):
    chosen_control_loc = random.choice(list(constraint_list.keys()))
    constraint_list_at_loc = constraint_list[chosen_control_loc]
    chosen_constraint = random.choice(constraint_list_at_loc)

    new_path = Not(chosen_constraint)
    for control_loc in constraint_list:
        constraint_list_at_loc = constraint_list[control_loc]
        for constraint in constraint_list_at_loc:
            if constraint == chosen_constraint and control_loc == chosen_control_loc:
                continue
            new_path = And (new_path, constraint)

    if is_sat(new_path):
        print(new_path)
    else:
        generate_new_symbolic_path(constraint_list)
    return new_path


def generate_new_input(symbolic_path):
    model = get_model(symbolic_path)
    print(model)


log_path = sys.argv[1]
project_path = sys.argv[2]
ppc_list, last_path = collect_symbolic_path(log_path, project_path)
constraint_list = analyse_symbolic_path(ppc_list)
new_path = generate_new_symbolic_path(constraint_list)
generate_new_input(new_path)
