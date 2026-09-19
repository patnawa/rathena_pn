#!/usr/bin/env python3
"""Remove old, reproducible compiler/cache files from inactive build trees only."""
import argparse,fcntl,json,os,subprocess,time
from pathlib import Path
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--apply',action='store_true');a=p.parse_args()
    root=Path('/app/rathena-builds').resolve(strict=True)
    assert str(root)=='/app/rathena-builds'
    with Path('/run/pn-build-retention.lock').open('a') as lock:
        fcntl.flock(lock,fcntl.LOCK_EX|fcntl.LOCK_NB)
        ids=subprocess.check_output(['docker','ps','-aq'],text=True).split()
        containers=json.loads(subprocess.check_output(['docker','inspect',*ids],text=True)) if ids else []
        # Stopped containers can be rollback/build dependencies too.
        mounts=[Path(m['Source']).resolve() for d in containers for m in d['Mounts'] if m['Type']=='bind']
        trees=sorted((p for p in root.iterdir() if p.is_dir() and not p.is_symlink()),key=lambda p:p.stat().st_mtime,reverse=True)
        kept=set(trees[:3]);cutoff=time.time()-30*86400;plan=[]
        mapped=set()
        for proc in Path('/proc').glob('[0-9]*/maps'):
            try:
                for line in proc.read_text().splitlines():
                    parts=line.split(maxsplit=5)
                    if len(parts)==6:mapped.add(parts[5])
            except (OSError,UnicodeError):pass
        for tree in trees:
            if tree in kept or any(tree.is_relative_to(m) or m.is_relative_to(tree) for m in mounts):continue
            for parent,dirs,files in os.walk(tree,followlinks=False):
                dirs[:]=[n for n in dirs if n!='.git' and not (Path(parent)/n).is_symlink()]
                for name in files:
                    path=Path(parent)/name
                    if path.suffix not in ('.o','.pyc') or path.is_symlink() or str(path) in mapped:continue
                    actual=path.resolve(strict=True);assert actual.is_relative_to(root)
                    st=path.stat()
                    if st.st_mtime>=cutoff:continue
                    plan.append({'path':str(path),'bytes':st.st_size,'mtime_ns':st.st_mtime_ns})
        removed=0
        if a.apply:
            for row in plan:
                path=Path(row['path']);assert path.resolve(strict=True).is_relative_to(root) and not path.is_symlink()
                assert path.stat().st_mtime_ns==row['mtime_ns'];path.unlink();removed+=row['bytes']
        print(json.dumps({'applied':a.apply,'eligible_files':len(plan),'eligible_bytes':sum(r['bytes'] for r in plan),
            'removed_bytes':removed,'age_days':30,'newest_trees_kept':[p.name for p in kept],
            'preserved':'all sources, executables, SQL/volumes/backups, Docker images and container dependencies'},indent=2))
if __name__=='__main__':main()
