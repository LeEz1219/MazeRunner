#!/usr/bin/env python3
"""Verify the committed package (release, payload hashes, line guide, links, screenshots) before rerunning tests or editing files."""
from pathlib import Path
import hashlib, html, json, re, struct, sys
ROOT = Path(__file__).resolve().parents[1]
EXPECTED_RELEASE = '5.4.2'
DOCUMENTS = ['01-Source-Line-by-Line.md','02-AI-Graphics-and-Redesign.md','03-Build-Test-and-Reproduce.md']

def require(condition, message):
    if not condition: raise RuntimeError(message)

def main():
    manifest=json.loads((ROOT/'manifest.json').read_text(encoding='utf-8'))
    require(manifest['release']==EXPECTED_RELEASE,'Incorrect manifest release')
    for entry in manifest['files']:
        path=(ROOT/entry['path']).resolve()
        require(path.is_relative_to(ROOT), 'Unsafe manifest path: '+entry['path'])
        require(path.is_file(), 'Missing payload: '+entry['path'])
        raw=path.read_bytes()
        require(len(raw)==entry['bytes'], 'Size mismatch: '+entry['path'])
        require(hashlib.sha256(raw).hexdigest()==entry['sha256'], 'SHA-256 mismatch: '+entry['path'])
    sums=(ROOT/'SHA256SUMS').read_text(encoding='utf-8').splitlines()
    for line in sums:
        digest,name=line.split('  ',1)
        path=(ROOT/name).resolve()
        require(path.is_relative_to(ROOT),'Unsafe checksum path')
        require(path.is_file() and hashlib.sha256(path.read_bytes()).hexdigest()==digest,'Checksum mismatch: '+name)
    require(sorted(p.name for p in ROOT.glob('*.md'))==DOCUMENTS,'Expected exactly the three documented Markdown files')
    source_path=f'src/mazerunnerV{EXPECTED_RELEASE}.cpp'
    require(manifest['source']['path']==source_path,'Manifest names an unexpected source file')
    source=(ROOT/source_path).read_bytes()
    require(hashlib.sha256(source).hexdigest()==manifest['source']['sha256'],f'Production source differs from the V{EXPECTED_RELEASE} manifest')
    lines=source.decode('utf-8').splitlines()
    guide=(ROOT/DOCUMENTS[0]).read_text(encoding='utf-8')
    entries=re.findall(r'^\*\*L(\d{4})\*\* — <code>(.*?)</code>$',guide,re.M)
    require(len(entries)==len(lines),'Annotation count differs from physical source-line count')
    for expected,(number,code) in enumerate(entries,1):
        require(int(number)==expected,'Annotation ordering/duplication at '+number)
        decoded=html.unescape(code)
        if decoded=='[blank line]': decoded=''
        require(decoded==lines[expected-1],f'Quoted source differs at L{expected:04d}')
        require(f'<a id="l{expected:04d}"></a>' in guide,f'Missing source anchor L{expected:04d}')
    for document in DOCUMENTS:
        data=(ROOT/document).read_text(encoding='utf-8')
        data=re.sub(r'<code>.*?</code>', '', data, flags=re.S)
        data=re.sub(r'```.*?```', '', data, flags=re.S)
        data=re.sub(r'`[^`\n]*`', '', data)
        for target in re.findall(r'!?\[[^\]\n]*\]\(([^)\n]+)\)',data):
            if target.startswith(('https://','http://','mailto:')):continue
            path,_,anchor=target.partition('#')
            resolved=(ROOT/path).resolve() if path else ROOT/document
            require(resolved.is_relative_to(ROOT),'Local link escapes the package: '+target)
            require(resolved.is_file(),f'Broken local link in {document}: {target}')
            if anchor and re.fullmatch(r'l\d{4}',anchor):
                require(f'<a id="{anchor}"></a>' in resolved.read_text(encoding='utf-8'),'Missing link anchor: '+target)
    pngs=sorted((ROOT/'artifacts/screenshots').glob('*.png'))
    require(len(pngs)==18,'Expected eighteen screenshots')
    for path in pngs:
        raw=path.read_bytes()
        require(raw[:8]==b'\x89PNG\r\n\x1a\n','Invalid PNG signature: '+path.name)
        w,h=struct.unpack('>II',raw[16:24])
        require(w>0 and h>0,'Invalid PNG dimensions: '+path.name)
    print(f'PASS: V{EXPECTED_RELEASE}; {len(manifest["files"])} payload hashes; source matches manifest; {len(lines)} exact line annotations; three Markdown guides; local links; eighteen PNGs.')
    print('This validates package integrity, not native-platform build or gameplay behavior.')

if __name__=='__main__':
    try:main()
    except Exception as error:
        print('FAIL: '+str(error),file=sys.stderr)
        sys.exit(1)
