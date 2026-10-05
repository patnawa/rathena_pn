'use strict';
const $=id=>document.getElementById(id);
const serviceNames=['Login','Character','Map','Game web'];
let pending=false,lastSuccess=null;
function text(id,value){$(id).textContent=value;}
function tone(id,value,state){text(id,value);$(id).dataset.tone=state;}
function summary(state,title,detail,pill){$('summary-box').dataset.state=state;text('summary',title);text('summary-detail',detail);text('summary-pill',pill);}
function unknownServices(){for(const name of serviceNames){const el=$('service-'+name.replace(' ','-'));el.dataset.state='error';el.textContent='Status unknown';}}
function validate(s){if(!s||typeof s.game_online!=='boolean'||!s.services||!Number.isFinite(Date.parse(s.checked_utc)))throw new Error('Invalid status report');for(const name of serviceNames)if(typeof s.services[name]!=='boolean')throw new Error('Invalid service status');return s;}
function renderStatus(s){const age=(Date.now()-Date.parse(s.checked_utc))/1000;if(age>90||age < -60)throw new Error('Status report is out of date');
  const online=serviceNames.slice(0,3).every(name=>s.services[name]);
  summary(online?'ok':'warn',online?'Midgard is online. Your party awaits.':'The realm needs a little attention.',online?'Login, character and world services are reachable.':'Some game services are unavailable. Check again before launching.',online?'ONLINE':'SERVICE ALERT');
  for(const name of serviceNames){const el=$('service-'+name.replace(' ','-'));el.dataset.state=s.services[name]?'ok':'warn';el.textContent=s.services[name]?'Reachable':'Unavailable';}
  text('checked','Checked '+new Date(s.checked_utc).toLocaleTimeString([],{hour:'2-digit',minute:'2-digit',second:'2-digit'}));
  const fresh=s.health_fresh===true;
  tone('health',fresh?(s.health_passed===true?'Passed':'Needs review'):'Stale or unavailable',fresh&&s.health_passed===true?'ok':'warn');
  tone('backup',fresh?(s.backup_passed===true?'Verified':'Needs review'):'Not verified',fresh&&s.backup_passed===true?'ok':'warn');
  text('disk',fresh&&typeof s.free_gib==='number'&&Number.isFinite(s.free_gib)?s.free_gib.toFixed(1)+' GiB free':'Unavailable');
  text('health-note',fresh?(s.health_passed===true?'The latest maintenance report passed.':'The latest maintenance report needs review. This is separate from game availability.'):'No fresh maintenance report. Historical values are not shown as current.');
}
async function update(){if(pending)return;pending=true;$('refresh').disabled=true;try{const response=await fetch('/status.json',{cache:'no-store',signal:AbortSignal.timeout(9000)});if(!response.ok)throw new Error('Status request failed');renderStatus(validate(await response.json()));lastSuccess=Date.now();}catch(error){summary('error','We can’t reach the status service.','Connect to the PN LAN, then refresh. The current game status is unknown.','CONNECTION LOST');unknownServices();tone('health','Unknown','warn');tone('backup','Unknown','warn');text('disk','Unavailable');text('health-note','Reconnect to get a fresh maintenance report.');text('checked',lastSuccess?'Last response '+new Date(lastSuccess).toLocaleTimeString():'No status response');}finally{pending=false;$('refresh').disabled=false;}}
async function release(){try{const response=await fetch('/client-info.json',{cache:'no-store',signal:AbortSignal.timeout(9000)});if(!response.ok)throw new Error();const r=await response.json();if(typeof r.release!=='string'||r.release.length>180||!Number.isSafeInteger(r.bytes)||r.bytes<0)throw new Error();const name=r.release.replace(/^client-\d{8}-/,'').replaceAll('-',' ');text('release-name',name.charAt(0).toUpperCase()+name.slice(1));text('release-detail',r.release+' · '+(r.bytes/1073741824).toFixed(2)+' GiB for a fresh install. Updates download changed files only.');}catch{ text('release-name','Your launcher checks the latest release.');text('release-detail','Release details are unavailable. Connect to the PN LAN to check for updates.');}}
$('refresh').addEventListener('click',()=>{update();release();});
$('copy-address').addEventListener('click',async()=>{try{if(navigator.clipboard&&window.isSecureContext)await navigator.clipboard.writeText('192.168.10.18');else{const field=document.createElement('textarea');field.value='192.168.10.18';field.setAttribute('aria-label','Server address');field.style.position='fixed';field.style.top='-1000px';document.body.appendChild(field);field.select();const copied=document.execCommand('copy');field.remove();$('copy-address').focus();if(!copied)throw new Error();}text('copy-result','Address copied.');}catch{text('copy-result','Server address: 192.168.10.18');}});
update();release();setInterval(()=>{if(!document.hidden){update();release();}},30000);document.addEventListener('visibilitychange',()=>{if(!document.hidden){update();release();}});
