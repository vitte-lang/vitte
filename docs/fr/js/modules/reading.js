export function initReadingMode() {
  const header = document.querySelector('.site-header'); if (!header) return;
  const key='docs-reading-mode';
  const b=document.createElement('button'); b.type='button'; b.className='reading-toggle'; b.textContent='Lecture';
  if (localStorage.getItem(key)==='on') document.body.classList.add('reading-mode');
  b.addEventListener('click',()=>{document.body.classList.toggle('reading-mode');localStorage.setItem(key,document.body.classList.contains('reading-mode')?'on':'off');});
  header.appendChild(b);
}
