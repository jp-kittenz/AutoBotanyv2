/* Entrance reveal with GSAP: as the intro screen fades, the headline, sprout
   and menu rise in one after another. Runs only behind the intro screen (so
   nothing visibly jumps), only while Motion is on, and only if GSAP loaded. */
(function () {
  'use strict';
  var gsap = window.gsap;
  var root = document.documentElement;
  var dialog = document.getElementById('intro-loader');
  if (!gsap || !dialog || typeof MutationObserver !== 'function') return;

  var text = Array.prototype.slice.call(document.querySelectorAll(
    '.welcome-caption h1, .welcome-caption > p'));
  var menu = Array.prototype.slice.call(document.querySelectorAll(
    '.menu-heading, .main-menu .menu-link, .main-menu .settings-link, .menu-footnote'));
  var sprout = Array.prototype.slice.call(document.querySelectorAll('.stage-sprout path'));
  var all = text.concat(menu);
  var waiting = false;
  var timeline = null;

  function motionOn() {
    return root.dataset.motion === 'on';
  }

  function reset() {
    if (timeline) timeline.kill();
    timeline = null;
    waiting = false;
    gsap.set(all, { clearProps: 'opacity,transform' });
    gsap.set(sprout, { clearProps: 'strokeDasharray,strokeDashoffset' });
  }

  // The intro screen is opaque, so the starting state is never seen.
  function hide() {
    if (!motionOn()) return;
    if (timeline) timeline.kill();
    timeline = null;
    waiting = true;
    gsap.set(all, { opacity: 0, y: 22 });
    sprout.forEach(function (path) {
      var length = path.getTotalLength ? path.getTotalLength() : 300;
      gsap.set(path, { strokeDasharray: length, strokeDashoffset: length });
    });
  }

  function reveal() {
    if (!waiting) return;
    waiting = false;
    if (!motionOn()) { reset(); return; }
    timeline = gsap.timeline({ defaults: { ease: 'power3.out' }, onComplete: reset })
      .to(text, { opacity: 1, y: 0, duration: 0.7, stagger: 0.12 }, 0.1)
      .to(sprout, { strokeDashoffset: 0, duration: 1.1, ease: 'power2.inOut', stagger: 0.15 }, 0.15)
      .to(menu, { opacity: 1, y: 0, duration: 0.55, stagger: 0.07 }, 0.3);
  }

  new MutationObserver(function () {
    if (!dialog.open || dialog.classList.contains('is-leaving')) reveal();
    else if (!waiting) hide();
  }).observe(dialog, { attributes: true, attributeFilter: ['open', 'class'] });

  // Keyboard users never wait on the animation. (Not on focus: motion.js
  // focuses the first menu link itself as the intro closes.)
  document.addEventListener('keydown', function (event) {
    if (timeline && event.key === 'Tab') timeline.progress(1);
  });
  // Turning Motion off mid-animation shows everything at once.
  new MutationObserver(function () {
    if (!motionOn() && (waiting || timeline)) reset();
  }).observe(root, { attributes: true, attributeFilter: ['data-motion'] });
  window.addEventListener('pagehide', function () { if (waiting || timeline) reset(); });

  // motion.js (loaded first) may already have opened the intro.
  if (dialog.open && !dialog.classList.contains('is-leaving')) hide();
}());
