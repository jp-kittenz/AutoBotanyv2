(function () {
  'use strict';
  var root = document.documentElement;
  var reduced = window.matchMedia('(prefers-reduced-motion: reduce)');
  var finePointer = window.matchMedia('(hover: hover) and (pointer: fine)');
  var motionButton = document.getElementById('motion-toggle');
  var motionLabel = document.getElementById('motion-label');
  var replay = document.getElementById('replay-intro');
  var preference = readStorage('localStorage', 'autobotany-motion');
  var enabled = false;
  var frame = 0;
  var reveals = Array.prototype.slice.call(document.querySelectorAll('.reveal'));
  var chapters = Array.prototype.slice.call(document.querySelectorAll('[data-chapter]'));
  var chapterLinks = Array.prototype.slice.call(document.querySelectorAll('.chapter-nav a'));
  var progressBar = document.getElementById('reading-fill');
  var ending = document.getElementById('trinity');
  var stage = document.getElementById('botanical-stage');
  var dialog = document.getElementById('intro-loader');
  var logo = document.getElementById('hero-logo');
  var introGeneration = 0;
  var introDeadline = 0;
  var introHold = 0;
  var introClosing = 0;

  function readStorage(storage, key) {
    try { return window[storage].getItem(key); } catch (error) { return null; }
  }

  function writeStorage(storage, key, value) {
    try { window[storage].setItem(key, value); } catch (error) {}
  }

  function clamp(value, low, high) {
    return Math.max(low, Math.min(high, value));
  }

  function schedule() {
    if (!frame && !document.hidden) frame = window.requestAnimationFrame(draw);
  }

  function draw() {
    frame = 0;
    var height = window.innerHeight || 800;
    var y = Math.max(0, window.scrollY || 0);
    var range = Math.max(1, root.scrollHeight - height);
    var progress = clamp(y / range, 0, 1);
    var heroProgress = enabled ? clamp(y / height, 0, 1) : 0;
    var endProgress = enabled && ending ? clamp((height - ending.getBoundingClientRect().top) / (height + ending.offsetHeight), 0, 1) : 0;
    var readings = reveals.map(function (element) {
      var previous = parseFloat(element.style.getPropertyValue('--reveal-y')) || 0;
      var top = element.getBoundingClientRect().top - previous;
      return { element: element, amount: clamp((height * .98 - top) / (height * .29), 0, 1) };
    });
    var current = chapters.length ? chapters[0].id : '';
    chapters.forEach(function (element) {
      if (element.getBoundingClientRect().top <= height * .45) current = element.id;
    });
    if (progressBar) progressBar.style.transform = 'scaleX(' + progress.toFixed(4) + ')';
    root.style.setProperty('--hero-progress', heroProgress.toFixed(4));
    root.style.setProperty('--end-progress', endProgress.toFixed(4));
    readings.forEach(function (item) {
      var amount = enabled ? item.amount : 1;
      item.element.style.setProperty('--reveal-y', ((1 - amount) * 34).toFixed(2) + 'px');
      item.element.style.setProperty('--reveal-opacity', (.2 + amount * .8).toFixed(3));
    });
    chapterLinks.forEach(function (link) {
      if (link.getAttribute('href') === '#' + current) link.setAttribute('aria-current', 'location');
      else link.removeAttribute('aria-current');
    });
  }

  function applyMotion() {
    enabled = preference !== 'off' && !reduced.matches;
    root.dataset.motion = enabled ? 'on' : 'off';
    root.classList.add('motion-ready');
    if (motionButton) {
      motionButton.hidden = false;
      motionButton.disabled = reduced.matches;
      motionButton.setAttribute('aria-pressed', String(enabled));
      motionButton.title = reduced.matches ? 'Reduced motion is enabled in your device settings' : 'Turn decorative animation ' + (enabled ? 'off' : 'on');
      motionLabel.textContent = enabled ? 'on' : 'off';
    }
    if (replay) {
      replay.hidden = !dialog || typeof dialog.showModal !== 'function';
      replay.disabled = !enabled;
      replay.title = enabled ? 'Replay the introduction' : 'Introduction animation is turned off';
    }
    if (!enabled) {
      if (stage) {
        stage.style.setProperty('--pointer-x', '0px');
        stage.style.setProperty('--pointer-y', '0px');
      }
      finishIntro(false);
    }
    schedule();
  }

  function finishIntro(focusMenu) {
    if (!dialog || !dialog.open) return;
    introGeneration++;
    clearTimeout(introDeadline);
    clearTimeout(introHold);
    clearTimeout(introClosing);
    dialog.classList.add('is-leaving');
    writeStorage('sessionStorage', 'autobotany-intro-seen', '1');
    introClosing = setTimeout(function () {
      var focused = dialog.contains(document.activeElement);
      dialog.close();
      dialog.classList.remove('is-leaving');
      var firstLink = document.querySelector('.menu-link');
      if ((focusMenu || focused) && firstLink) firstLink.focus({ preventScroll: true });
    }, enabled ? 400 : 0);
  }

  function artworkReady() {
    return new Promise(function (resolve) {
      if (!logo) { resolve(false); return; }
      function decoded() {
        if (!logo.naturalWidth) { resolve(false); return; }
        if (typeof logo.decode === 'function') logo.decode().then(function () { resolve(true); }, function () { resolve(true); });
        else resolve(true);
      }
      if (logo.complete) decoded();
      else {
        logo.addEventListener('load', decoded, { once: true });
        logo.addEventListener('error', function () { resolve(false); }, { once: true });
      }
    });
  }

  function openIntro() {
    if (!enabled || !dialog || typeof dialog.showModal !== 'function' || dialog.open) return;
    clearTimeout(introClosing);
    var generation = ++introGeneration;
    var started = performance.now();
    var status = document.getElementById('loader-status');
    var bar = document.getElementById('loader-bar');
    dialog.classList.remove('is-leaving');
    bar.style.transform = 'scaleX(.04)';
    status.textContent = 'Preparing the artwork';
    dialog.showModal();
    introDeadline = setTimeout(function () { finishIntro(false); }, 4200);
    artworkReady().then(function (loaded) {
      if (generation !== introGeneration || !dialog.open) return;
      status.textContent = loaded ? 'Ready to explore' : 'The menu is ready';
      bar.style.transform = 'scaleX(1)';
      introHold = setTimeout(function () { finishIntro(false); }, Math.max(250, 1400 - (performance.now() - started)));
    });
  }

  Array.prototype.forEach.call(document.querySelectorAll('.brand-art img, .seal-image img'), function (image) {
    function fallback() {
      image.hidden = true;
      var alternative = image.parentElement.querySelector('.art-fallback, .seal-fallback');
      if (alternative) alternative.hidden = false;
    }
    image.addEventListener('error', fallback, { once: true });
    if (image.complete && !image.naturalWidth) fallback();
  });

  if (motionButton) motionButton.addEventListener('click', function () {
    preference = enabled ? 'off' : 'on';
    writeStorage('localStorage', 'autobotany-motion', preference);
    applyMotion();
  });
  if (reduced.addEventListener) reduced.addEventListener('change', applyMotion);
  else if (reduced.addListener) reduced.addListener(applyMotion);
  window.addEventListener('scroll', schedule, { passive: true });
  window.addEventListener('resize', schedule, { passive: true });
  window.addEventListener('pageshow', schedule);
  window.addEventListener('pagehide', function () {
    finishIntro(false);
    if (frame) window.cancelAnimationFrame(frame);
    frame = 0;
  });
  document.addEventListener('visibilitychange', function () {
    root.classList.toggle('page-hidden', document.hidden);
    if (document.hidden) {
      finishIntro(false);
      if (frame) window.cancelAnimationFrame(frame);
      frame = 0;
    } else schedule();
  });
  document.addEventListener('focusin', function (event) {
    var element = event.target.closest ? event.target.closest('.reveal') : null;
    if (element) {
      element.style.setProperty('--reveal-opacity', '1');
      element.style.setProperty('--reveal-y', '0px');
    }
  });
  if (stage) {
    stage.addEventListener('pointermove', function (event) {
      if (!enabled || !finePointer.matches) return;
      var rect = stage.getBoundingClientRect();
      stage.style.setProperty('--pointer-x', (clamp((event.clientX - rect.left) / rect.width - .5, -.5, .5) * 13).toFixed(2) + 'px');
      stage.style.setProperty('--pointer-y', (clamp((event.clientY - rect.top) / rect.height - .5, -.5, .5) * 9).toFixed(2) + 'px');
    }, { passive: true });
    stage.addEventListener('pointerleave', function () {
      stage.style.setProperty('--pointer-x', '0px');
      stage.style.setProperty('--pointer-y', '0px');
    });
  }
  if (dialog) {
    document.getElementById('skip-intro').addEventListener('click', function () { finishIntro(true); });
    dialog.addEventListener('cancel', function (event) { event.preventDefault(); finishIntro(true); });
    if (replay) replay.addEventListener('click', openIntro);
  }
  applyMotion();
  if (!readStorage('sessionStorage', 'autobotany-intro-seen')) openIntro();
}());
