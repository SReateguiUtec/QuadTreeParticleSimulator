<script setup>
import { ref, onMounted, watch, onUnmounted } from 'vue';

const props = defineProps({
  frame: { type: Object, required: true },
  data:  { type: Object, required: true },
  spaceMode: { type: Boolean, default: true }
});

const canvasRef = ref(null);
let ctx = null;
let frameCount = 0; // for animated star twinkle

const W = () => canvasRef.value.width;
const H = () => canvasRef.value.height;
const sx = (v) => (v / props.data.width)  * W();
const sy = (v) => (v / props.data.height) * H();
const sl = (v) => (v / props.data.width)  * W();

/* ─────────────── BACKGROUND ─────────────── */
const drawBackground = () => {
  // Deep-space base gradient
  const bg = ctx.createLinearGradient(0, 0, W(), H());
  bg.addColorStop(0,    '#020510');
  bg.addColorStop(0.5,  '#050d1a');
  bg.addColorStop(1,    '#020510');
  ctx.fillStyle = bg;
  ctx.fillRect(0, 0, W(), H());

  // Subtle nebula cloud (background atmosphere)
  const neb = ctx.createRadialGradient(W() * 0.38, H() * 0.45, 0, W() * 0.5, H() * 0.5, W() * 0.7);
  neb.addColorStop(0,    'rgba(13, 40, 100, 0.18)');
  neb.addColorStop(0.4,  'rgba(6, 20, 60, 0.10)');
  neb.addColorStop(1,    'rgba(0,0,0,0)');
  ctx.fillStyle = neb;
  ctx.fillRect(0, 0, W(), H());

  // Static star field (deterministic positions)
  for (let i = 0; i < 200; i++) {
    const x = ((Math.sin(i * 127.1) * 43758.5453) % 1 + 1) % 1 * W();
    const y = ((Math.sin(i * 311.7) * 24634.6345) % 1 + 1) % 1 * H();
    const twinkle = 0.3 + 0.5 * Math.abs(Math.sin(frameCount * 0.03 + i * 2.4));
    const sz = i % 20 === 0 ? 1.5 : i % 7 === 0 ? 1.0 : 0.55;
    // colour varies from cool white to warm blue
    const hue = 200 + (i % 40);
    ctx.beginPath();
    ctx.arc(x, y, sz, 0, Math.PI * 2);
    ctx.fillStyle = `hsla(${hue}, 60%, 90%, ${twinkle * 0.7})`;
    ctx.fill();
  }
};

/* ─────────────── QUAD BOUNDARIES ─────────────── */
const drawBoundaries = () => {
  ctx.save();
  // Hologram grid: very faint, screen blending so it lights on top
  ctx.globalCompositeOperation = 'screen';
  ctx.strokeStyle = 'rgba(0, 210, 255, 0.055)';
  ctx.lineWidth = 0.8;
  ctx.beginPath();
  for (const b of props.frame.boundaries) {
    ctx.rect(sx(b.x), sy(b.y), sl(b.w), sl(b.h));
  }
  ctx.stroke();
  ctx.restore();
};

/* ─────────────── QUERY RADAR ─────────────── */
const drawQuery = () => {
  const x = sx(props.frame.query.x);
  const y = sy(props.frame.query.y);
  const isRectangle = props.frame.query.type === 'rectangle';
  const r = sl(props.frame.query.radius);
  const rw = sl(props.frame.query.w ?? props.frame.query.radius * 2);
  const rh = (props.frame.query.h ?? props.frame.query.radius * 2) / props.data.height * H();

  ctx.save();
  ctx.globalCompositeOperation = 'screen';

  if (isRectangle) {
    const left = x - rw / 2;
    const top = y - rh / 2;
    ctx.fillStyle = 'rgba(0, 230, 255, 0.055)';
    ctx.strokeStyle = 'rgba(0, 240, 255, 0.65)';
    ctx.shadowBlur = 14;
    ctx.shadowColor = 'rgba(0, 240, 255, 0.9)';
    ctx.lineWidth = 1.5;
    ctx.beginPath();
    ctx.rect(left, top, rw, rh);
    ctx.fill();
    ctx.stroke();
    ctx.restore();
    return;
  }

  // Outer glow ring 1
  const glow1 = ctx.createRadialGradient(x, y, r * 0.7, x, y, r * 1.25);
  glow1.addColorStop(0, 'rgba(0, 220, 255, 0.0)');
  glow1.addColorStop(1, 'rgba(0, 220, 255, 0.06)');
  ctx.fillStyle = glow1;
  ctx.beginPath();
  ctx.arc(x, y, r * 1.25, 0, Math.PI * 2);
  ctx.fill();

  // Inner fill – subtle energy field
  const fill = ctx.createRadialGradient(x, y, 0, x, y, r);
  fill.addColorStop(0,   'rgba(0, 230, 255, 0.07)');
  fill.addColorStop(0.6, 'rgba(0, 200, 240, 0.03)');
  fill.addColorStop(1,   'rgba(0, 180, 220, 0.0)');
  ctx.fillStyle = fill;
  ctx.beginPath();
  ctx.arc(x, y, r, 0, Math.PI * 2);
  ctx.fill();

  // Hard edge ring with glow
  ctx.shadowBlur  = 14;
  ctx.shadowColor = 'rgba(0, 240, 255, 0.9)';
  ctx.strokeStyle = 'rgba(0, 240, 255, 0.55)';
  ctx.lineWidth   = 1.5;
  ctx.beginPath();
  ctx.arc(x, y, r, 0, Math.PI * 2);
  ctx.stroke();

  ctx.restore();
};

/* ─────────────── COLLISION LINES ─────────────── */
const drawCollisions = (byId) => {
  if (!props.frame.collisions.length) return;
  ctx.save();
  ctx.globalCompositeOperation = 'screen';
  ctx.shadowBlur  = 12;
  ctx.shadowColor = '#ff2060';
  ctx.strokeStyle = 'rgba(255, 32, 96, 0.65)';
  ctx.lineWidth   = 1.5;
  ctx.beginPath();
  for (const c of props.frame.collisions) {
    const a = byId.get(c.a), b = byId.get(c.b);
    if (!a || !b) continue;
    ctx.moveTo(sx(a.x), sy(a.y));
    ctx.lineTo(sx(b.x), sy(b.y));
  }
  ctx.stroke();
  ctx.restore();
};

/* ─────────────── PARTICLES ─────────────── */
const drawParticles = () => {
  ctx.save();
  ctx.globalCompositeOperation = 'screen';

  for (const p of props.frame.particles) {
    const x = sx(p.x);
    const y = sy(p.y);
    const baseR = Math.max(1.5, sl(p.r) * 0.65);

    let coreColor, midColor, glowColor, glowR, coreAlpha;

    if (p.colliding) {
      // 🔴 Collision — hot white core, magenta halo
      coreColor  = 'rgba(255, 255, 255, 1)';
      midColor   = 'rgba(255, 60, 120, 0.85)';
      glowColor  = 'rgba(255, 20, 80, 0)';
      glowR      = baseR * 5;
      coreAlpha  = 1;
    } else if (p.candidate) {
      // 🟡 Candidate — bright amber/gold
      coreColor  = 'rgba(255, 245, 180, 0.95)';
      midColor   = 'rgba(255, 190, 30, 0.7)';
      glowColor  = 'rgba(255, 150, 0, 0)';
      glowR      = baseR * 4;
      coreAlpha  = 0.95;
    } else {
      // 🔵 Normal — cool cyan/blue plasma
      coreColor  = 'rgba(190, 235, 255, 0.85)';
      midColor   = 'rgba(40, 170, 240, 0.5)';
      glowColor  = 'rgba(10, 130, 220, 0)';
      glowR      = baseR * 3.2;
      coreAlpha  = 0.85;
    }

    // Draw volumetric plasma orb (radial gradient)
    const grad = ctx.createRadialGradient(x, y, 0, x, y, glowR);
    grad.addColorStop(0,    coreColor);
    grad.addColorStop(0.25, midColor);
    grad.addColorStop(1,    glowColor);

    ctx.beginPath();
    ctx.arc(x, y, glowR, 0, Math.PI * 2);
    ctx.fillStyle = grad;
    ctx.fill();
  }

  ctx.restore();
};

/* ─────────────── SCAN LINE EFFECT ─────────────── */
const drawScanLine = () => {
  const scanY = (frameCount * 2) % H();
  const grad = ctx.createLinearGradient(0, scanY - 4, 0, scanY + 4);
  grad.addColorStop(0,   'rgba(0, 230, 255, 0)');
  grad.addColorStop(0.5, 'rgba(0, 230, 255, 0.035)');
  grad.addColorStop(1,   'rgba(0, 230, 255, 0)');
  ctx.fillStyle = grad;
  ctx.fillRect(0, scanY - 4, W(), 8);
};

/* ─────────────── MAIN DRAW ─────────────── */
const draw = () => {
  if (!canvasRef.value || !props.frame) return;
  ctx = canvasRef.value.getContext('2d');
  const byId = new Map(props.frame.particles.map(p => [p.id, p]));

  ctx.clearRect(0, 0, W(), H());

  if (props.spaceMode) {
    drawBackground();
    drawBoundaries();
    drawScanLine();
    drawQuery();
    drawCollisions(byId);
    drawParticles();
  } else {
    // Plain mode
    ctx.fillStyle = '#f8fafc';
    ctx.fillRect(0, 0, W(), H());
    ctx.strokeStyle = 'rgba(148,163,184,0.5)';
    ctx.lineWidth = 0.7;
    ctx.beginPath();
    for (const b of props.frame.boundaries) {
      ctx.rect(sx(b.x), sy(b.y), sl(b.w), sl(b.h));
    }
    ctx.stroke();

    // Plain query region
    ctx.beginPath();
    if (props.frame.query.type === 'rectangle') {
      const x = sx(props.frame.query.x);
      const y = sy(props.frame.query.y);
      const w = sl(props.frame.query.w ?? props.frame.query.radius * 2);
      const h = (props.frame.query.h ?? props.frame.query.radius * 2) / props.data.height * H();
      ctx.rect(x - w / 2, y - h / 2, w, h);
    } else {
      ctx.arc(sx(props.frame.query.x), sy(props.frame.query.y), sl(props.frame.query.radius), 0, Math.PI * 2);
    }
    ctx.fillStyle = 'rgba(99, 102, 241, 0.08)';
    ctx.strokeStyle = '#6366f1';
    ctx.lineWidth = 1.5;
    ctx.fill();
    ctx.stroke();

    // Plain particles
    for (const p of props.frame.particles) {
      ctx.beginPath();
      ctx.arc(sx(p.x), sy(p.y), Math.max(3, sl(p.r) * 0.7), 0, Math.PI * 2);
      ctx.fillStyle = p.colliding ? '#ef4444' : p.candidate ? '#f59e0b' : '#3b82f6';
      ctx.fill();
    }
  }

  frameCount++;
};

/* ─────────────── LIFECYCLE ─────────────── */
const resizeCanvas = () => {
  if (!canvasRef.value) return;
  const el = canvasRef.value.parentElement;
  canvasRef.value.width  = Math.max(320, el.clientWidth);
  canvasRef.value.height = Math.max(240, el.clientHeight);
  draw();
};

onMounted(() => {
  resizeCanvas();
  window.addEventListener('resize', resizeCanvas);
});

onUnmounted(() => {
  window.removeEventListener('resize', resizeCanvas);
});

watch(() => props.frame, draw, { deep: true });
watch(() => props.spaceMode, draw);
</script>

<template>
  <div class="w-full h-full flex items-center justify-center relative overflow-hidden rounded-2xl"
       style="background: #020510;">
    
    <!-- Outer ambient glow behind the canvas -->
    <div class="absolute inset-0 rounded-2xl pointer-events-none"
         style="box-shadow: inset 0 0 80px rgba(0,180,255,0.04), inset 0 0 2px rgba(0,220,255,0.12);"></div>

    <canvas
      ref="canvasRef"
      class="max-w-full max-h-full block"
    ></canvas>
  </div>
</template>
