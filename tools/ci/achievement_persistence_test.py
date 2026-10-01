"""Exact achievement save handlers with deterministic database/transport faults.

This probes control flow, not MariaDB durability. Real SQL acceptance is separate.
"""
from pathlib import Path
import argparse
import subprocess
import tempfile

ROOT=Path(__file__).resolve().parents[2]


def body(path,start):
    text=path.read_text()
    return text[text.index(start):].split('\n/**',1)[0]


def function(path, signature):
    text=path.read_text()
    start=text.index(signature)
    opening=text.index('{',start)
    depth=1
    end=opening+1
    while depth:
        depth+=(text[end]=='{')-(text[end]=='}')
        end+=1
    return text[start:end]+'\n'


def run(work):
    work.mkdir(parents=True,exist_ok=True)
    source=ROOT/'src/char/int_achievement.cpp'
    char=function(source,'static void mapif_achievement_logout_save(')+body(source,'static bool mapif_achievement_rows_valid(') if 'static bool mapif_achievement_rows_valid(' in source.read_text() else body(source,'int32 mapif_parse_achievement_save(')
    char_load=function(source,'void mapif_achievement_load(')+function(source,'int32 mapif_parse_achievement_load(')
    ack=function(ROOT/'src/map/intif.cpp','void intif_parse_achievementsave(')+function(ROOT/'src/map/intif.cpp','void intif_parse_achievementlogoutsave(')
    load=body(ROOT/'src/map/intif.cpp','void intif_parse_achievements(')
    save=(function(ROOT/'src/map/intif.cpp','int32 intif_achievement_save_snapshot(')+
        function(ROOT/'src/map/intif.cpp','int32 intif_achievement_logout_save(')+
        function(ROOT/'src/map/intif.cpp','int32 intif_achievement_save('))
    request=body(ROOT/'src/map/intif.cpp','void intif_request_achievements(')
    reward=body(ROOT/'src/map/intif.cpp','void intif_parse_achievementreward(')
    chrif=(ROOT/'src/map/chrif.cpp').read_text()
    if 'sd->achievement_data.loaded && sd->achievement_data.save' not in chrif:
        raise AssertionError('logout achievement save must require an authoritative loaded cache')
    if 'map_foreachpc(chrif_recover_achievement_channel);' not in chrif:
        raise AssertionError('char-link loss must reconcile every achievement session')
    save_function=function(ROOT/'src/map/chrif.cpp','int32 chrif_save(')
    if save_function.index('intif_achievement_save(sd);') > save_function.index('WFIFOW(char_fd,0) = 0x2b01;'):
        raise AssertionError('final achievement save must precede the quitting/offline character frame')
    protocol=(ROOT/'src/custom/achievement_protocol.hpp').read_text()
    required_logout_tokens=('achievement_snapshot','achievement_generation','achievement_pending',
        'logout_status','final_save_pending','intif_achievement_logout_save(',
        'chrif_send_retained_final(node)')
    combined=chrif+(ROOT/'src/map/chrif.hpp').read_text()+(ROOT/'src/map/intif.cpp').read_text()+protocol
    if any(token not in combined for token in required_logout_tokens):
        raise AssertionError('logout retry must retain, tokenize, reauthorize, and resend both final stages')
    if 'logout_save_request = 0x30A5' not in protocol or 'logout_save_response = 0x38A5' not in protocol:
        raise AssertionError('logout snapshot must use a dedicated packet pair')
    retained_sender=function(ROOT/'src/map/chrif.cpp','static bool chrif_send_retained_logout(')
    if retained_sender.index('node->logout_saves') > retained_sender.index('intif_achievement_logout_save('):
        raise AssertionError('dependent save journal must be acknowledged before the logout snapshot')
    ordinary_ack=function(ROOT/'src/map/intif.cpp','void intif_parse_achievementsave(')
    logout_ack=function(ROOT/'src/map/intif.cpp','void intif_parse_achievementlogoutsave(')
    if 'chrif_auth_achievement_saved' in ordinary_ack or not all(token in logout_ack for token in
        ('account_id','char_id','generation','chrif_auth_achievement_saved')):
        raise AssertionError('ordinary ACKs must be structurally unable to release the logout barrier')
    if 'chrif_save(node->sd' in chrif:
        raise AssertionError('logout retries must never rebuild status from an already-cleaned session')
    reward_disconnect=chrif[chrif.index('static int32 chrif_recover_achievement_channel('):]
    reward_disconnect=reward_disconnect[:reward_disconnect.index('\n}')]
    if 'sd->achievement_data.reward_pending_id' not in reward_disconnect or 'set_eof(sd->fd);' not in reward_disconnect:
        raise AssertionError('lost reward replies must invalidate and disconnect the stale map cache')
    if 'sd->achievement_data.save = true;' not in reward_disconnect:
        raise AssertionError('char-link loss must retry authoritative achievement saves with unknown ACK state')
    (work/'char-handler.inc').write_text(char)
    (work/'char-load-handler.inc').write_text(char_load)
    (work/'map-handler.inc').write_text(ack)
    (work/'map-load-handler.inc').write_text(load)
    (work/'map-save-handler.inc').write_text(save)
    (work/'map-request-handler.inc').write_text(request)
    (work/'map-reward-handler.inc').write_text(reward)
    (work/'chrif-achievement-channel.inc').write_text(
        function(ROOT/'src/map/chrif.cpp','static int32 chrif_recover_achievement_channel('))
    binary=work/'achievement-persistence'
    subprocess.run(['g++','-std=c++17','-O1','-fsanitize=address,undefined',
        '-fno-sanitize=alignment','-fno-sanitize-recover=all','-I'+str(ROOT/'src'),'-I'+str(work),
        str(Path(__file__).with_suffix('.cpp')),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir',type=Path)
    args=parser.parse_args()
    if args.build_dir:run(args.build_dir)
    else:
        with tempfile.TemporaryDirectory(prefix='achievement-persistence-') as directory:run(Path(directory))
