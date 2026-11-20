#!/usr/bin/env python3
"""Verify the committed package (release, payload hashes, function guide, links, screenshots) before rerunning tests or editing files."""
from pathlib import Path
from urllib.parse import unquote
import hashlib, json, re, struct, sys
ROOT = Path(__file__).resolve().parents[1]
EXPECTED_RELEASE = '5.4.4'
DOCUMENTS = ['01-SourceCode-Deepdive-and- Explanation.md', '02-AI-Design-Deepdive.md', '03-Graphics-Design.md', '04-Build-Test-and-Reproduce.md']

def require(condition, message):
    if not condition: raise RuntimeError(message)

def release_paths():
    names = (ROOT / 'scripts/devkit-files.txt').read_text(encoding='utf-8').splitlines()
    require(len(names) == len(set(names)), 'Duplicate release path')
    for name in names:
        rel = Path(name)
        path = ROOT / rel
        require(name and rel.as_posix() == name and not rel.is_absolute() and '..' not in rel.parts,
                'Invalid release path: ' + name)
        require(path.resolve() == path, 'Release paths must not be symlinks: ' + name)
    return sorted(names)

def source_functions(source):
    """Index named file and class functions in this single translation unit, including GLSL."""
    tokens = r'//[^\n]*|/\*.*?\*/|R"(?P<delim>\w*)\(.*?\)(?P=delim)"|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\''
    masked = re.sub(tokens, lambda m: re.sub(r'[^\n]', ' ', m.group()), source, flags=re.S)
    entries, scopes, depth = [], [], 0
    for number, line in enumerate(masked.splitlines(), 1):
        code = line.strip()
        if not code or code.startswith('#'):
            continue
        if depth == len(scopes):
            owner = re.match(r'(?:class|struct)\s+(\w+)[^{;]*\{', code)
            if owner:
                scopes.append(owner[1])
            elif '(' in code:
                prefix = code.split('(', 1)[0].strip()
                name = re.search(r'(operator[!=]+|~?\w+)$', prefix)
                if (name and not any(c in prefix[:name.start()] for c in '=;{}') and
                    (name.start() > 0 or (scopes and name[0].lstrip('~') == scopes[-1])) and
                    not prefix.startswith((':', 'return '))):
                    entries.append((number, '::'.join([*scopes, name[0]])))
        depth += line.count('{') - line.count('}')
        del scopes[depth:]
    for stage, shader in re.findall(r'(vertex|fragment)_source\s*=\s*R"glsl\((.*?)\)glsl"', source, re.S):
        first = source.index(shader, source.index(stage + '_source'))
        position = first + shader.index('void main(')
        entries.append((source.count('\n', 0, position) + 1, 'GLSL ' + stage + '::main'))
    return sorted(entries)


def function_coverage(source, guide, source_path):
    """Match each purpose and design rationale to a real definition at its linked line."""
    rows = re.findall(r'^\| \[`([^`]+)`\]\(' + re.escape(source_path) +
                      r'#L(\d+)\) \| ([^|\n]+) \| ([^|\n]+) \|$', guide, re.M)
    documented = [(int(line), name) for name, line, purpose, reason in rows if purpose.strip() and reason.strip()]
    expected = source_functions(source)
    return dict(defined_functions=len(expected), documented_functions=len(documented),
                missing=[name for line, name in expected if (line, name) not in documented],
                exact_match=bool(expected) and documented == expected)

def main():
    manifest=json.loads((ROOT/'manifest.json').read_text(encoding='utf-8'))
    require(manifest['release']==EXPECTED_RELEASE,'Incorrect manifest release')
    expected = {entry['path']: entry['sha256'] for entry in manifest['files']}
    require(len(expected) == len(manifest['files']), 'Duplicate manifest entries')
    approved = release_paths()
    require(set(expected) == set(approved), 'Manifest membership differs from scripts/devkit-files.txt')
    for entry in manifest['files']:
        path=(ROOT/entry['path']).resolve()
        require(path.is_relative_to(ROOT), 'Unsafe manifest path: '+entry['path'])
        require(path.is_file(), 'Missing payload: '+entry['path'])
        raw=path.read_bytes()
        require(len(raw)==entry['bytes'], 'Size mismatch: '+entry['path'])
        require(hashlib.sha256(raw).hexdigest()==entry['sha256'], 'SHA-256 mismatch: '+entry['path'])
    sums=(ROOT/'SHA256SUMS').read_text(encoding='utf-8').splitlines()
    ledger = {}
    for line in sums:
        digest,name=line.split('  ',1)
        require(name not in ledger, 'Duplicate checksum: '+name)
        ledger[name] = digest
        path=(ROOT/name).resolve()
        require(path.is_relative_to(ROOT),'Unsafe checksum path')
        require(path.is_file() and hashlib.sha256(path.read_bytes()).hexdigest()==digest,'Checksum mismatch: '+name)
    expected['manifest.json'] = hashlib.sha256((ROOT/'manifest.json').read_bytes()).hexdigest()
    require(ledger == expected, 'Checksum ledger and manifest do not cover the same payload')
    require(sorted(name for name in approved if '/' not in name and name.endswith('.md'))==DOCUMENTS,'Expected exactly the four documented Markdown files')
    source_path=f'src/mazerunnerV{EXPECTED_RELEASE}.cpp'
    require(manifest['source']['path']==source_path,'Manifest names an unexpected source file')
    source=(ROOT/source_path).read_bytes()
    require(hashlib.sha256(source).hexdigest()==manifest['source']['sha256'],f'Production source differs from the V{EXPECTED_RELEASE} manifest')
    guide=(ROOT/DOCUMENTS[0]).read_text(encoding='utf-8')
    coverage = function_coverage(source.decode('utf-8'), guide, source_path)
    require(coverage['exact_match'], 'Function guide definitions, source links or rationales differ from production')
    for document in [*DOCUMENTS, '.github/README.md']:
        data=(ROOT/document).read_text(encoding='utf-8')
        data=re.sub(r'<code>.*?</code>', '', data, flags=re.S)
        data=re.sub(r'```.*?```', '', data, flags=re.S)
        data=re.sub(r'`[^`\n]*`', '', data)
        for target in re.findall(r'!?\[[^\]\n]*\]\(([^)\n]+)\)',data):
            if target.startswith(('https://','http://','mailto:')):continue
            path,_,anchor=target.partition('#')
            path=unquote(path)
            resolved=(ROOT/document).parent.joinpath(path).resolve() if path else ROOT/document
            require(resolved.is_relative_to(ROOT),'Local link escapes the package: '+target)
            require(resolved.exists(),f'Broken local link in {document}: {target}')
            if resolved.suffix == '.cpp' and anchor.startswith('L'):
                require(re.fullmatch(r'L[1-9]\d*',anchor) and int(anchor[1:]) <= len(resolved.read_text(encoding='utf-8').splitlines()),
                        'Invalid source line: '+target)
    pngs=[ROOT/name for name in approved if Path(name).parent.as_posix()=='artifacts/screenshots' and name.endswith('.png')]
    require(len(pngs)==18,'Expected eighteen screenshots')
    for path in pngs:
        raw=path.read_bytes()
        require(raw[:8]==b'\x89PNG\r\n\x1a\n','Invalid PNG signature: '+path.name)
        w,h=struct.unpack('>II',raw[16:24])
        require(w>0 and h>0,'Invalid PNG dimensions: '+path.name)
    print(f'PASS: V{EXPECTED_RELEASE}; {len(manifest["files"])} payload hashes; source matches manifest; {coverage["documented_functions"]} documented functions; four Markdown guides; local links; eighteen PNGs.')
    print('This validates package integrity, not native-platform build or gameplay behavior.')

if __name__=='__main__':
    try:main()
    except Exception as error:
        print('FAIL: '+str(error),file=sys.stderr)
        sys.exit(1)
