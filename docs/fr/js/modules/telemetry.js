export function initTelemetry() {
  const key='docs-telemetry-optin';
  if (localStorage.getItem(key)!=='yes') return;
  const day=new Date().toISOString().slice(0,10);
  const page=location.pathname.replace(/^.*\//,'')||'index.html';
  const bucketKey=`docs-telemetry-${day}`;
  const data=JSON.parse(localStorage.getItem(bucketKey)||'{}');
  data[page]=(data[page]||0)+1;
  localStorage.setItem(bucketKey, JSON.stringify(data));
}
export function initTelemetryToggle(){
  const header=document.querySelector('.site-header'); if(!header) return;
  const key='docs-telemetry-optin';
  const b=document.createElement('button'); b.type='button'; b.className='telemetry-toggle';
  const sync=()=>{b.textContent=localStorage.getItem(key)==='yes'?'Telemetry: on':'Telemetry: off';};
  sync(); b.addEventListener('click',()=>{localStorage.setItem(key, localStorage.getItem(key)==='yes'?'no':'yes'); sync();});
  header.appendChild(b);
}
