"""Extract actual weekly/reward user-functions for isolated instance VM tests."""
import re
from episode_party_progression_test import scan_to

def functions(root):
    rows=[]
    for relative in ('npc/custom/main_office/weekly_practice.txt','npc/custom/main_office/reward_progress.txt'):
        source=(root/relative).read_text(encoding='utf-8')
        for match in re.finditer(r'function\s+script\s+(\w+)\s*\{',source):
            start=match.end()-1
            rows.append((match[1],source[start:scan_to(source,start,'{','}')+1],'function'))
    return rows
