"""Read literal tables from a 32-bit Lua 5.1 chunk without executing Lua.

Only straight-line literal construction is accepted. Uncalled, zero-upvalue
function declarations may follow it; calls, jumps, and other code fail closed.
This is a data reader, not a Lua interpreter. Function bodies are never run.
Layout/opcodes follow Lua 5.1's lundump.c and lopcodes.h:
https://www.lua.org/source/5.1/lundump.c.html
https://www.lua.org/source/5.1/lopcodes.h.html
"""
import json
import struct
import sys
from pathlib import Path


def literal_tables(data):
    if data[:12] != b'\x1bLua\x51\0\1\4\4\4\x08\0':
        raise ValueError('Expected little-endian Lua 5.1 with 32-bit size_t and double numbers')
    position = 12

    def take(size):
        nonlocal position
        if size < 0 or size > len(data) - position:
            raise ValueError('Truncated Lua chunk')
        result = data[position:position + size]
        position += size
        return result

    def u32():
        return struct.unpack('<I', take(4))[0]

    def string():
        size = u32()
        if size == 0:
            return None
        raw = take(size)
        if raw[-1] != 0:
            raise ValueError('Unterminated Lua string')
        return raw[:-1].decode('cp949')

    def prototype(depth=0):
        if depth > 50:
            raise ValueError('Lua prototype nesting is too deep')
        string()  # embedded source filename
        take(8)  # line range
        upvalues = take(4)[0]  # upvalues, parameters, vararg, stack size
        code = tuple(x[0] for x in struct.iter_unpack('<I', take(4 * u32())))
        constants = []
        for _ in range(u32()):
            tag = take(1)[0]
            if tag == 0:
                value = None
            elif tag == 1:
                value = bool(take(1)[0])
            elif tag == 3:
                value = struct.unpack('<d', take(8))[0]
                if value.is_integer():
                    value = int(value)
            elif tag == 4:
                value = string()
            else:
                raise ValueError(f'Unsupported constant type: {tag}')
            constants.append(value)
        children = [prototype(depth + 1) for _ in range(u32())]
        take(4 * u32())  # line-number debug array
        for _ in range(u32()):
            string()
            take(8)  # local-variable scope
        for _ in range(u32()):
            string()  # upvalue debug names
        return code, constants, children, upvalues

    instructions, constants, children, _ = prototype()
    if position != len(data):
        raise ValueError('Unexpected trailing Lua data')
    if not instructions or instructions[-1] & 63 != 30 or (instructions[-1] >> 23) & 511 != 1:
        raise ValueError('Expected final no-value RETURN')
    registers, tables = {}, {}

    def rk(operand):
        return constants[operand & 255] if operand & 256 else registers[operand]

    declarations = False
    skip = set()
    for pc, instruction in enumerate(instructions):
        if pc in skip:
            continue
        op = instruction & 63
        a, b, c = (instruction >> 6) & 255, (instruction >> 23) & 511, (instruction >> 14) & 511
        bx = instruction >> 14
        if op == 36:  # CLOSURE followed by SETGLOBAL; no call or upvalue binding
            if bx >= len(children) or children[bx][3] != 0 or pc + 1 >= len(instructions):
                raise ValueError('Unsupported Lua function declaration')
            following = instructions[pc + 1]
            name_index = following >> 14
            if (following & 63) != 7 or ((following >> 6) & 255) != a or name_index >= len(constants):
                raise ValueError('Expected a named, uncalled function declaration')
            if constants[name_index] in tables:
                raise ValueError('Function declaration overwrites a literal global')
            declarations = True
            skip.add(pc + 1)
        elif op == 30 and b == 1 and pc == len(instructions) - 1:  # RETURN (no values)
            pass
        elif declarations:
            raise ValueError(f'Unexpected code after function declarations at instruction {pc}')
        elif op == 0:  # MOVE
            registers[a] = registers[b]
        elif op == 1:  # LOADK
            registers[a] = constants[bx]
        elif op == 7:  # SETGLOBAL
            tables[constants[bx]] = registers[a]
        elif op == 9:  # SETTABLE
            registers[a][rk(b)] = rk(c)
        elif op == 10:  # NEWTABLE
            registers[a] = {}
        else:
            raise ValueError(f'Non-literal opcode {op} at instruction {pc}')
    return tables


if __name__ == '__main__':
    tables = literal_tables(Path(sys.argv[1]).read_bytes())
    print(json.dumps({name: {'count': len(table), 'sample': list(table.items())[:8]}
                      for name, table in tables.items()}, ensure_ascii=False, indent=2))
