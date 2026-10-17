"""Writes no-op definitions for imgui.h, linked in place of ImGui wherever the developer UI is compiled out."""

import os
import re

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
HEADER = os.path.join(ROOT, 'Source', 'ThirdParty', 'imgui', 'imgui.h')
OUTPUT = os.path.join(ROOT, 'Source', 'ThirdParty', 'imgui', 'stubs', 'imgui_stubs.cpp')

HASH = chr(35)
API = 'IMGUI_API'


def strip_comments(text):
    out = []
    index = 0
    in_string = False
    while index < len(text):
        char = text[index]
        if in_string:
            out.append(char)
            if char == '\\' and index + 1 < len(text):
                out.append(text[index + 1])
                index += 2
                continue
            if char == '"':
                in_string = False
            index += 1
            continue
        if char == '"':
            in_string = True
            out.append(char)
            index += 1
            continue
        if text.startswith('//', index):
            end = text.find('\n', index)
            index = len(text) if end < 0 else end
            continue
        if text.startswith('/*', index):
            end = text.find('*/', index + 2)
            index = len(text) if end < 0 else end + 2
            continue
        out.append(char)
        index += 1
    return ''.join(out)


def split_params(params):
    parts = []
    depth = 0
    in_string = False
    current = []
    for char in params:
        if in_string:
            current.append(char)
            if char == '"':
                in_string = False
            continue
        if char == '"':
            in_string = True
        elif char in '(<[{':
            depth += 1
        elif char in ')>]}':
            depth -= 1
        elif char == ',' and depth == 0:
            parts.append(''.join(current).strip())
            current = []
            continue
        current.append(char)
    tail = ''.join(current).strip()
    if tail:
        parts.append(tail)
    return parts


def drop_default(param):
    depth = 0
    in_string = False
    for index, char in enumerate(param):
        if in_string:
            if char == '"':
                in_string = False
            continue
        if char == '"':
            in_string = True
        elif char in '(<[{':
            depth += 1
        elif char in ')>]}':
            depth -= 1
        elif char == '=' and depth == 0:
            return param[:index].strip()
    return param.strip()


def body_for(return_type, qualified):
    kind = return_type.replace('const ', '').strip()
    if kind == 'void':
        return '{}'
    if kind.endswith('*'):
        return '{ return nullptr; }'
    if kind.endswith('&'):
        return '{ LuminaImGuiStubUnavailable("' + qualified + '"); }'
    return '{ return {}; }'


def drop_leading_macros(text):
    """Bare macros such as IM_MSVC_RUNTIME_CHECKS_OFF end without a semicolon, so they lead the next statement."""
    while True:
        match = re.match(r'^([A-Z_][A-Z0-9_]+)\s+', text)
        if not match or match.group(1) == API:
            return text
        text = text[match.end():]


DECL = re.compile(
    r'^' + API + r'\s+(?P<ret>.*?)\s*(?P<name>~?[A-Za-z_]\w*)\s*\((?P<params>.*)\)\s*(?P<const>const)?\s*'
    r'(?:IM_FMTARGS\(\d+\)|IM_FMTLIST\(\d+\))?\s*;$')


def main():
    with open(HEADER, encoding='utf-8') as handle:
        source = strip_comments(handle.read())

    # Statements are rebuilt from the token stream so a declaration split over lines reads as one.
    lines = source.splitlines()
    out = []
    scopes = []
    pending_scope = None
    statement = ''

    for raw in lines:
        line = raw.strip()
        if not line:
            continue

        if line.startswith(HASH):
            directive = line[1:].strip()
            if re.match(r'^(if|ifdef|ifndef|elif|else|endif)\b', directive):
                out.append(line)
            continue

        statement = (statement + ' ' + line).strip() if statement else line

        while statement:
            open_index = statement.find('{')
            close_index = statement.find('}')
            semi_index = statement.find(';')
            candidates = [i for i in (open_index, close_index, semi_index) if i >= 0]
            if not candidates:
                break
            first = min(candidates)
            head = drop_leading_macros(statement[:first].strip())

            if first == open_index:
                match = re.match(r'^(?:struct|class)\s+(?:IMGUI_API\s+)?(\w+)', head)
                if match and '(' not in head:
                    scopes.append(('struct', match.group(1)))
                elif re.match(r'^namespace\s+(\w+)', head):
                    scopes.append(('namespace', re.match(r'^namespace\s+(\w+)', head).group(1)))
                else:
                    scopes.append(('block', head))
                statement = statement[first + 1:].strip()
                continue

            if first == close_index:
                if scopes:
                    scopes.pop()
                statement = statement[first + 1:].strip()
                continue

            declaration = drop_leading_macros(statement[:first + 1].strip())
            declaration = re.sub(r'\s*IM_FMT(?:ARGS|LIST)\(\d+\)', '', declaration)
            statement = statement[first + 1:].strip()
            if any(kind == 'block' for kind, _ in scopes):
                continue
            if not declaration.startswith(API) or 'static ' in declaration:
                continue

            match = DECL.match(declaration)
            if not match:
                continue

            names = [name for kind, name in scopes if kind in ('struct', 'namespace')]
            owner = '::'.join(names)
            name = match.group('name')
            ret = match.group('ret').strip()
            is_member = any(kind == 'struct' for kind, _ in scopes)
            is_special = is_member and (name == names[-1] or name == '~' + names[-1])

            params = ', '.join(drop_default(p) for p in split_params(match.group('params')))
            qualified = (owner + '::' + name) if owner else name
            const = ' const' if match.group('const') else ''

            if is_special:
                out.append(qualified + '(' + params + ')' + const + ' {}')
            else:
                out.append(ret + ' ' + qualified + '(' + params + ')' + const + ' ' + body_for(ret, qualified))

    text = [
        '// <auto-generated/> by Engine/Tools/ImGuiGen/generate_imgui_stubs.py from ThirdParty/imgui/imgui.h.',
        HASH + 'include "imgui.h"',
        '',
        HASH + 'include <cstdio>',
        HASH + 'include <cstdlib>',
        '',
        '[[noreturn]] static void LuminaImGuiStubUnavailable(const char* Call)',
        '{',
        '    std::fprintf(stderr, "%s returns a reference ImGui cannot give where the developer UI is compiled out. Guard it with USING(WITH_DEV_UI).\\n", Call);',
        '    std::abort();',
        '}',
        '',
    ]
    text.extend(out)

    os.makedirs(os.path.dirname(OUTPUT), exist_ok=True)
    with open(OUTPUT, 'w', encoding='utf-8', newline='\n') as handle:
        handle.write('\n'.join(text) + '\n')
    print('wrote ' + OUTPUT + ' with ' + str(sum(1 for l in out if not l.startswith(HASH))) + ' definitions')


if __name__ == '__main__':
    main()
