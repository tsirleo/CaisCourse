import json


def getAddress(instruction):
    if instruction.get("isForeignBranch") is None:
        return format(instruction["address"], '016X')
    else:
        return instruction["foreignTargetName"]


def getLocalBranchTargetLabels(trace):
    labels = set()
    for i in range(len(trace) - 1):
        if trace[i].get("isBranch") is not None and trace[i].get("isForeignBranch") is None:
            labels.add(getAddress(trace[i + 1]))

    return labels


def create_graph(trace):
    graph = {}
    trace_length = len(trace)
    local_target_labels = getLocalBranchTargetLabels(trace)
    b_prev = None
    i_start = getAddress(trace[0])
    i = start_ind = 0
    asm_instructions_block = []

    while i < trace_length:
        if trace[i].get("isBranch") is None and i + 1 < trace_length and getAddress(trace[i + 1]) not in local_target_labels:
            asm_instructions_block.append(trace[i].get("text"))
            i += 1
            continue

        if b_prev is None:
            asm_instructions_block.append(trace[i].get("text"))
            graph[i_start] = {"size": i - start_ind + 1, "prev": [], "target": [], "block": asm_instructions_block.copy()}
            b_prev = i_start
            asm_instructions_block.clear()
        else:
            asm_instructions_block.append(trace[i].get("text"))
            graph[i_start] = {"size": i - start_ind + 1, "prev": [b_prev], "target": [], "block": asm_instructions_block.copy()}
            graph[b_prev]["target"].append(i_start)
            b_prev = i_start
            asm_instructions_block.clear()

        if trace[i].get("isForeignBranch") is not None:
            foreign_target_name = getAddress(trace[i])

            if graph.get(foreign_target_name) is None:
                graph[foreign_target_name] = {"size": 1, "prev": [b_prev], "target": [], "block": []}
            else:
                graph[foreign_target_name]["prev"].append(b_prev)

            graph[b_prev]["target"].append(foreign_target_name)
            graph[b_prev]["size"] = graph[b_prev]["size"] - 1
            b_prev = foreign_target_name

        i += 1
        if i < trace_length:
            i_start = getAddress(trace[i])
            start_ind = i

        while i < trace_length and graph.get(i_start) is not None:
            if not (i_start in graph[b_prev]["target"]):
                graph[b_prev]["target"].append(i_start)
            b_prev = i_start
            i += graph[i_start]["size"]
            i_start = getAddress(trace[i])
            start_ind = i

    return graph


def generate_dot_code(graph):
    dot_code = "strict digraph {\n"
    for node, data in graph.items():
        if not len(data["block"]):
            dot_code += f'  "{node}" [ label="{node}" ];\n'
        else:
            code = "\n  ".join(data["block"])
            dot_code += f'  "{node}" [ shape = box, label="{node}\n  {code}" ];\n'

        for target in data["target"]:
            dot_code += f'  "{node}" -> "{target}"\n'
    dot_code += "}\n"

    return dot_code


def generate_dot_file(dot_code, output_filename):
    with open(output_filename, 'w') as dot_file:
        dot_file.write(dot_code)


def main(input_filename, output_filename):
    with open(input_filename, 'r') as json_file:
        trace = json.load(json_file)

    graph = create_graph(trace)
    dot_code = generate_dot_code(graph)
    generate_dot_file(dot_code, output_filename)


if __name__ == '__main__':
    import sys

    if len(sys.argv) != 3:
        print("Usage: python3 main.py input.json output.dot")
    else:
        main(sys.argv[1], sys.argv[2])
