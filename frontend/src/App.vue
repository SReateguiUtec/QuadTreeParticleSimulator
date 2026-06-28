<script setup>
import { ref, onMounted, computed, onUnmounted, nextTick } from 'vue';
import QuadCanvas from './components/QuadCanvas.vue';
import MetricsCard from './components/MetricsCard.vue';
import TreeGraphOverlay from './components/TreeGraphOverlay.vue';
import ExperimentPanel from './components/ExperimentPanel.vue';

const BASE = 'http://localhost:8080';

// ── State ──────────────────────────────────────────────────────────────────
const simMeta      = ref(null);
const liveFrame    = ref(null);
const frameBuffer  = ref([]);
const scrubIndex   = ref(0);
const isLive       = ref(true);
const isLoading    = ref(false);
const isStreaming  = ref(false);
const showTree     = ref(false);
const activeTab    = ref('simulator'); // 'simulator' | 'experiments'
const spaceMode    = ref(false);
const frameCount   = ref(0);
const particleCount = ref(0);

// Interactive mode: user clicks to add particles
const interactiveMode = ref(true);

const config = ref({ w: 1000, h: 1000, r: 4.0, cap: 4 });
const genConfig = ref({ n: 300, dist: 0 });

const MAX_BUFFER = 300;
let eventSource = null;

// ── Stream ─────────────────────────────────────────────────────────────────
const startStream = async () => {
  if (eventSource) { eventSource.close(); eventSource = null; }

  simMeta.value    = null;
  liveFrame.value  = null;
  frameBuffer.value = [];
  scrubIndex.value = 0;
  frameCount.value = 0;
  isLive.value     = true;
  isLoading.value  = true;
  isStreaming.value = false;

  // Measure the canvas container to match simulation space to actual render area
  await nextTick();
  const rect = canvasContainerRef.value?.getBoundingClientRect();
  const canvasW = rect ? Math.round(rect.width)  : 1200;
  const canvasH = rect ? Math.round(rect.height) : 700;
  config.value.w = canvasW;
  config.value.h = canvasH;

  // Configure the server dimensions first
  const params = new URLSearchParams({
    w: config.value.w, h: config.value.h,
    r: config.value.r, cap: config.value.cap
  });

  eventSource = new EventSource(`${BASE}/simulate/stream?${params}`);

  eventSource.onmessage = (e) => {
    const msg = JSON.parse(e.data);
    if (msg.type === 'init') {
      simMeta.value     = msg;
      isLoading.value   = false;
      isStreaming.value = true;
      return;
    }
    if (msg.type === 'frame') {
      frameCount.value++;
      particleCount.value = msg.particles?.length ?? 0;
      frameBuffer.value.push(msg);
      if (frameBuffer.value.length > MAX_BUFFER) frameBuffer.value.shift();
      if (isLive.value) {
        liveFrame.value  = msg;
        scrubIndex.value = frameBuffer.value.length - 1;
      }
    }
  };

  eventSource.onerror = () => {
    isLoading.value   = false;
    isStreaming.value = false;
  };
};

const stopStream = () => {
  if (eventSource) { eventSource.close(); eventSource = null; }
  isStreaming.value = false;
};

onMounted(startStream);
onUnmounted(stopStream);

// ── Particle actions ───────────────────────────────────────────────────────
const addParticle = async (simX, simY) => {
  await fetch(`${BASE}/particle`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ x: simX, y: simY, speed: 1.5 })
  });
};

const clearParticles = async () => {
  await fetch(`${BASE}/particles`, { method: 'DELETE' });
};

const generateParticles = async () => {
  await fetch(`${BASE}/particles/generate`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ n: genConfig.value.n, dist: genConfig.value.dist })
  });
};

// ── Canvas click handler ───────────────────────────────────────────────────
const canvasContainerRef = ref(null);

const onCanvasClick = (e) => {
  if (!interactiveMode.value || !simMeta.value) return;

  const rect = canvasContainerRef.value.getBoundingClientRect();
  // The canvas is square and centered inside its container
  const size    = Math.min(rect.width, rect.height);
  const offsetX = (rect.width  - size) / 2;
  const offsetY = (rect.height - size) / 2;

  const pxX = e.clientX - rect.left  - offsetX;
  const pxY = e.clientY - rect.top   - offsetY;

  if (pxX < 0 || pxY < 0 || pxX > size || pxY > size) return;

  const simX = (pxX / size) * simMeta.value.width;
  const simY = (pxY / size) * simMeta.value.height;

  addParticle(simX, simY);
};

// ── Scrubbing ──────────────────────────────────────────────────────────────
const onScrub = (e) => {
  const idx = parseInt(e.target.value);
  scrubIndex.value = idx;
  liveFrame.value  = frameBuffer.value[idx] ?? null;
  isLive.value     = (idx === frameBuffer.value.length - 1);
};

const resumeLive = () => {
  isLive.value     = true;
  scrubIndex.value = frameBuffer.value.length - 1;
  liveFrame.value  = frameBuffer.value[scrubIndex.value] ?? null;
};

// ── Metrics ────────────────────────────────────────────────────────────────
const metrics = computed(() => {
  const m = liveFrame.value?.metrics ?? {};
  return {
    particles:     liveFrame.value?.particles?.length ?? 0,
    nodes:         m.quadtreeNodes    ?? 0,
    collisions:    m.collisions       ?? 0,
    candidates:    m.candidates       ?? 0,
    qtComparisons: m.qtComparisons    ?? 0,
    bfComparisons: m.bfComparisons    ?? 0,
    frameTimeMs:   m.frameTimeMs      ?? 0
  };
});

const speedup = computed(() =>
  metrics.value.qtComparisons > 0
    ? (metrics.value.bfComparisons / metrics.value.qtComparisons).toFixed(1)
    : '—'
);
</script>

<template>
  <div class="min-h-screen text-slate-100 font-sans p-4 md:p-6"
       style="background: radial-gradient(ellipse at 20% 0%, #091428 0%, #030a14 60%, #020510 100%);">

    <!-- ── HEADER ────────────────────────────────────────────────────── -->
    <header class="w-full max-w-[1400px] mx-auto flex flex-wrap items-center justify-between gap-3 mb-5">
      <div>
        <div class="flex items-center gap-3">
          <span class="relative flex h-3 w-3">
            <span v-if="isStreaming" class="animate-ping absolute inline-flex h-full w-full rounded-full opacity-60" style="background:#00d4ff;"></span>
            <span class="relative inline-flex rounded-full h-3 w-3" :style="isStreaming ? 'background:#00d4ff' : 'background:#475569'"></span>
          </span>
          <h1 class="text-2xl font-black tracking-tight"
              style="background:linear-gradient(90deg,#00d4ff,#7b6fff,#00d4ff);background-size:200%;-webkit-background-clip:text;-webkit-text-fill-color:transparent;">
            QuadTree Simulator
          </h1>
        </div>
        <p class="text-[10px] text-slate-500 ml-6 font-mono tracking-widest">
          2D PARTICLE ENGINE · INTERACTIVE · C++ BACKEND
        </p>
      </div>

      <!-- Tab switcher -->
      <div class="flex items-center gap-1 rounded-xl p-1"
           style="background:rgba(255,255,255,0.04);border:1px solid rgba(255,255,255,0.07);">
        <button @click="activeTab = 'simulator'"
                class="px-4 py-1.5 rounded-lg text-xs font-bold transition-all"
                :style="activeTab === 'simulator'
                  ? 'background:linear-gradient(135deg,#0ea5e9,#6366f1);color:#fff;box-shadow:0 0 12px rgba(14,165,233,0.3)'
                  : 'background:transparent;color:#64748b'">
          Simulador
        </button>
        <button @click="activeTab = 'experiments'"
                class="px-4 py-1.5 rounded-lg text-xs font-bold transition-all"
                :style="activeTab === 'experiments'
                  ? 'background:linear-gradient(135deg,#6366f1,#8b5cf6);color:#fff;box-shadow:0 0 12px rgba(99,102,241,0.3)'
                  : 'background:transparent;color:#64748b'">
          Experimentos
        </button>
      </div>

      <!-- Stream controls -->
      <div class="flex items-center gap-2 px-4 py-2 rounded-2xl flex-wrap"
           style="background:rgba(255,255,255,0.04);backdrop-filter:blur(12px);">

        <button @click="startStream" :disabled="isLoading"
                class="flex items-center gap-2 px-4 py-2 rounded-xl text-sm font-bold transition-all hover:brightness-110 active:scale-95 disabled:opacity-40"
                style="background:linear-gradient(135deg,#0ea5e9,#6366f1);color:#fff;box-shadow:0 0 20px rgba(14,165,233,0.35);">
          <svg class="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24">
            <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2"
                  d="M14.752 11.168l-3.197-2.132A1 1 0 0010 9.87v4.263a1 1 0 001.555.832l3.197-2.132a1 1 0 000-1.664z"/>
            <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M21 12a9 9 0 11-18 0 9 9 0 0118 0z"/>
          </svg>
          {{ isLoading ? 'Conectando…' : 'Reiniciar Stream' }}
        </button>

        <button v-if="isStreaming" @click="stopStream"
                class="px-3 py-1.5 rounded-xl text-sm font-bold transition border border-rose-500/40"
                style="background:rgba(244,63,94,0.12);color:#f87171;">
          Detener
        </button>

        <div class="w-px h-7 bg-white/10 mx-1"></div>

        <!-- Interactive mode toggle -->
        <button @click="interactiveMode = !interactiveMode"
                class="flex items-center gap-2 px-3 py-1.5 rounded-xl text-xs font-bold border transition-all"
                :style="interactiveMode
                  ? 'background:rgba(34,197,94,0.15);border-color:rgba(34,197,94,0.4);color:#4ade80;box-shadow:0 0 8px rgba(34,197,94,0.2)'
                  : 'background:transparent;border-color:rgba(255,255,255,0.1);color:#64748b'">
          <svg class="w-3.5 h-3.5" fill="none" stroke="currentColor" viewBox="0 0 24 24">
            <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2"
                  d="M15 15l-2 5L9 9l11 4-5 2zm0 0l5 5"/>
          </svg>
          {{ interactiveMode ? 'Modo Click ON' : 'Modo Click' }}
        </button>

        <button @click="showTree = !showTree"
                class="px-3 py-1.5 rounded-xl text-xs font-bold border transition-all"
                :style="showTree
                  ? 'background:rgba(168,85,247,0.15);border-color:rgba(168,85,247,0.5);color:#c084fc'
                  : 'background:transparent;border-color:rgba(255,255,255,0.1);color:#64748b'">
          Organigrama
        </button>

        <button @click="spaceMode = !spaceMode"
                class="px-3 py-1.5 rounded-xl text-xs font-bold border transition-all"
                :style="spaceMode
                  ? 'background:rgba(6,182,212,0.15);border-color:rgba(6,182,212,0.4);color:#22d3ee'
                  : 'background:transparent;border-color:rgba(255,255,255,0.1);color:#64748b'">
          {{ spaceMode ? 'Space' : 'Plain' }}
        </button>
      </div>
    </header>

    <!-- ── EXPERIMENTS TAB ────────────────────────────────────────────── -->
    <div v-if="activeTab === 'experiments'" class="w-full max-w-[1400px] mx-auto">
      <ExperimentPanel />
    </div>

    <!-- ── BODY ──────────────────────────────────────────────────────── -->
    <div v-if="activeTab === 'simulator'" class="w-full max-w-[1400px] mx-auto flex flex-col lg:flex-row gap-5">

      <!-- Canvas col -->
      <main class="flex-1 flex flex-col gap-4 min-w-0">

        <!-- Config + Actions bar -->
        <div class="rounded-2xl border border-slate-800/60 p-4 flex flex-wrap gap-3 items-end"
             style="background:rgba(255,255,255,0.03);backdrop-filter:blur(12px);">

          <!-- Sim params -->
          <label class="flex flex-col gap-1">
            <span class="text-[10px] font-semibold tracking-widest text-slate-500 uppercase">Capacidad Nodo</span>
            <input type="number" v-model="config.cap" min="1" max="20"
                   class="w-24 bg-white/5 border border-slate-800/60 rounded-lg px-3 py-1.5 text-sm text-white focus:outline-none focus:border-cyan-500"/>
          </label>
          <label class="flex flex-col gap-1">
            <span class="text-[10px] font-semibold tracking-widest text-slate-500 uppercase">Radio Partícula</span>
            <input type="number" step="0.5" v-model="config.r"
                   class="w-24 bg-white/5 border border-slate-800/60 rounded-lg px-3 py-1.5 text-sm text-white focus:outline-none focus:border-cyan-500"/>
          </label>

          <div class="w-px h-8 bg-white/10 self-end mb-1"></div>

          <!-- Bulk generate -->
          <label class="flex flex-col gap-1">
            <span class="text-[10px] font-semibold tracking-widest text-slate-500 uppercase">Generar N</span>
            <input type="number" v-model="genConfig.n" min="1"
                   class="w-24 bg-white/5 border border-slate-800/60 rounded-lg px-3 py-1.5 text-sm text-white focus:outline-none focus:border-cyan-500"/>
          </label>
          <label class="flex flex-col gap-1">
            <span class="text-[10px] font-semibold tracking-widest text-slate-500 uppercase">Distribución</span>
            <select v-model="genConfig.dist"
                    class="bg-white/5 border border-slate-800/60 rounded-lg px-3 py-1.5 text-sm text-white focus:outline-none focus:border-cyan-500">
              <option :value="0">Uniforme</option>
              <option :value="1">Clusters</option>
              <option :value="2">Alta Densidad</option>
              <option :value="3">Galaxia</option>
            </select>
          </label>

          <label class="flex flex-col gap-1">
            <span class="text-[10px] font-semibold tracking-widest text-transparent uppercase select-none">·</span>
            <button @click="generateParticles"
                    class="px-4 py-1.5 rounded-lg text-sm font-bold border transition-all hover:brightness-110"
                    style="background:rgba(99,102,241,0.2);border-color:rgba(99,102,241,0.5);color:#818cf8;">
              Generar
            </button>
          </label>

          <label class="flex flex-col gap-1">
            <span class="text-[10px] font-semibold tracking-widest text-transparent uppercase select-none">·</span>
            <button @click="clearParticles"
                    class="px-4 py-1.5 rounded-lg text-sm font-bold border transition-all hover:brightness-110"
                    style="background:rgba(244,63,94,0.12);border-color:rgba(244,63,94,0.4);color:#f87171;">
              Limpiar
            </button>
          </label>
        </div>

        <!-- CANVAS with click-to-add -->
        <div ref="canvasContainerRef"
             class="relative rounded-2xl overflow-hidden border border-slate-800/60"
             style="background:#020510; height: calc(100vh - 230px); min-height: 420px;"
             :style="interactiveMode ? 'cursor:crosshair' : ''"
             @click="onCanvasClick">

          <!-- Interactive hint overlay -->
          <div v-if="interactiveMode && simMeta && particleCount === 0"
               class="absolute inset-0 flex flex-col items-center justify-center gap-3 pointer-events-none z-10">
            <div class="rounded-2xl px-6 py-4 flex flex-col items-center gap-2"
                 style="background:rgba(0,212,255,0.07);border:1px solid rgba(0,212,255,0.2);">
              <svg class="w-8 h-8 text-cyan-500 opacity-60" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                <path stroke-linecap="round" stroke-linejoin="round" stroke-width="1.5"
                      d="M15 15l-2 5L9 9l11 4-5 2zm0 0l5 5"/>
              </svg>
              <p class="text-sm font-mono text-cyan-400/70 tracking-widest text-center">
                HAZ CLICK PARA INSERTAR PARTÍCULAS
              </p>
              <p class="text-xs text-slate-600 text-center">
                O usa el botón Generar para insertar en grupo
              </p>
            </div>
          </div>

          <QuadCanvas v-if="simMeta && liveFrame"
                      :frame="liveFrame" :data="simMeta" :spaceMode="spaceMode"
                      class="w-full h-full transition-all duration-500"
                      :class="showTree ? 'opacity-25 blur-sm' : 'opacity-100'"/>

          <TreeGraphOverlay v-if="showTree && liveFrame?.tree" :tree="liveFrame.tree"/>

          <!-- Loading -->
          <div v-if="isLoading"
               class="absolute inset-0 flex flex-col items-center justify-center gap-4">
            <div class="relative">
              <div class="w-14 h-14 rounded-full border-2 animate-spin" style="border-color:rgba(0,212,255,0.2);border-top-color:#00d4ff;"></div>
            </div>
            <span class="text-xs font-mono text-slate-500 tracking-widest">CONECTANDO CON C++...</span>
          </div>

          <!-- Stopped -->
          <div v-if="!isLoading && !simMeta" class="absolute inset-0 flex flex-col items-center justify-center gap-3">
            <span class="text-slate-600 text-xs font-mono tracking-widest">SIMULACIÓN DETENIDA</span>
            <button @click="startStream" class="mt-1 px-5 py-2 rounded-xl text-sm font-bold transition"
                    style="background:rgba(0,212,255,0.12);border:1px solid rgba(0,212,255,0.3);color:#22d3ee;">
              Iniciar
            </button>
          </div>

          <!-- Particle counter badge -->
          <div v-if="simMeta" class="absolute top-3 right-3 rounded-xl px-3 py-1.5 flex items-center gap-2 pointer-events-none"
               style="background:rgba(0,0,0,0.5);border:1px solid rgba(255,255,255,0.08);backdrop-filter:blur(8px);">
            <span class="w-1.5 h-1.5 rounded-full" style="background:#38bdf8;box-shadow:0 0 6px #38bdf8;"></span>
            <span class="text-xs font-mono text-slate-300">{{ particleCount }} partículas</span>
          </div>
        </div>

        <!-- Scrubber -->
        <div class="rounded-2xl border border-slate-800/60 px-5 py-3 flex items-center gap-4"
             style="background:rgba(255,255,255,0.03);backdrop-filter:blur(12px);">
          <span class="text-xs font-mono text-slate-500 shrink-0 w-20 text-right">
            {{ isLive ? 'LIVE' : `${scrubIndex+1}/${frameBuffer.length}` }}
          </span>
          <div class="flex-1 relative h-2">
            <div class="absolute inset-0 rounded-full bg-white/8"></div>
            <div class="absolute inset-y-0 left-0 rounded-full pointer-events-none"
                 :style="`width:${frameBuffer.length>1?(scrubIndex/(frameBuffer.length-1))*100:100}%;background:linear-gradient(90deg,#00d4ff,#6366f1);box-shadow:0 0 8px rgba(0,212,255,0.5);`"></div>
            <input type="range" class="absolute inset-0 w-full opacity-0 cursor-pointer"
                   :min="0" :max="Math.max(0, frameBuffer.length-1)"
                   :value="scrubIndex" @input="onScrub"/>
          </div>
          <div v-if="!isLive && frameBuffer.length > 0">
            <button @click="resumeLive" class="text-xs font-bold px-2 py-1 rounded-lg transition"
                    style="background:rgba(34,197,94,0.15);border:1px solid rgba(34,197,94,0.4);color:#4ade80;">LIVE</button>
          </div>
          <span class="text-xs font-mono text-slate-600 shrink-0">{{ frameCount }} frames</span>
        </div>
      </main>

      <!-- ── SIDEBAR ──────────────────────────────────────────────────── -->
      <aside class="w-full lg:w-72 flex flex-col gap-4">

        <!-- Interactive instructions -->
        <div class="rounded-2xl border border-green-500/20 p-4"
             style="background:rgba(34,197,94,0.06);">
          <h2 class="text-sm font-bold text-green-400 mb-3 flex items-center gap-2">
            <svg class="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24">
              <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2"
                    d="M15 15l-2 5L9 9l11 4-5 2zm0 0l5 5"/>
            </svg>
            Modo Interactivo
          </h2>
          <ul class="space-y-2 text-xs text-slate-400">
            <li class="flex items-start gap-2">
              <span class="text-green-500 mt-0.5">·</span>
              <span>Activa <strong class="text-slate-300">Modo Click</strong> y haz click en el canvas para insertar partículas.</span>
            </li>
            <li class="flex items-start gap-2">
              <span class="text-indigo-400 mt-0.5">·</span>
              <span>Usa <strong class="text-slate-300">Generar</strong> para insertar múltiples partículas de golpe.</span>
            </li>
            <li class="flex items-start gap-2">
              <span class="text-rose-400 mt-0.5">·</span>
              <span><strong class="text-slate-300">Limpiar</strong> elimina todas las partículas y reinicia el árbol.</span>
            </li>
            <li class="flex items-start gap-2">
              <span class="text-cyan-400 mt-0.5">·</span>
              <span><strong class="text-slate-300">Organigrama</strong> muestra la jerarquía del árbol en tiempo real.</span>
            </li>
          </ul>
        </div>

        <!-- Performance -->
        <div class="rounded-2xl border border-slate-800/60 p-5 flex flex-col gap-4"
             style="background:rgba(255,255,255,0.03);backdrop-filter:blur(12px);">
          <div class="flex items-center gap-2">
            <svg class="w-4 h-4 text-amber-400" fill="none" stroke="currentColor" viewBox="0 0 24 24">
              <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M13 10V3L4 14h7v7l9-11h-7z"/>
            </svg>
            <h2 class="text-sm font-bold text-slate-300">Performance · Tiempo Real</h2>
          </div>

          <div class="grid grid-cols-1 gap-2">
            <div class="rounded-xl p-3 border border-teal-500/20" style="background:rgba(20,184,166,0.06);">
              <span class="text-[10px] font-semibold tracking-widest text-teal-600 uppercase block mb-1">Comparaciones QuadTree</span>
              <span class="text-2xl font-black text-teal-400 tabular-nums">{{ metrics.qtComparisons.toLocaleString() }}</span>
            </div>
            <div class="rounded-xl p-3 border border-rose-500/20 relative overflow-hidden" style="background:rgba(244,63,94,0.06);">
              <span class="text-[10px] font-semibold tracking-widest text-rose-600 uppercase block mb-1">Fuerza Bruta O(N²)</span>
              <span class="text-2xl font-black text-rose-400 tabular-nums">{{ metrics.bfComparisons.toLocaleString() }}</span>
              <div v-if="metrics.qtComparisons > 0" class="absolute top-3 right-3 text-xs font-black text-emerald-400" style="text-shadow:0 0 8px rgba(52,211,153,0.6);">
                {{ speedup }}× faster
              </div>
            </div>
            <div class="rounded-xl p-3 border border-cyan-500/20" style="background:rgba(6,182,212,0.06);">
              <span class="text-[10px] font-semibold tracking-widest text-cyan-600 uppercase block mb-1">Tiempo C++ / frame</span>
              <span class="text-2xl font-black text-cyan-300 tabular-nums">{{ metrics.frameTimeMs.toFixed(2) }}<span class="text-sm font-normal text-cyan-600 ml-1">ms</span></span>
            </div>
          </div>

          <div class="grid grid-cols-2 gap-2 pt-2 border-t border-slate-800/60">
            <MetricsCard label="Partículas"  :value="metrics.particles.toLocaleString()"  colorClass="text-blue-400"/>
            <MetricsCard label="Nodos QT"    :value="metrics.nodes.toLocaleString()"      colorClass="text-violet-400"/>
            <MetricsCard label="Colisiones"  :value="metrics.collisions.toLocaleString()" colorClass="text-rose-400"/>
            <MetricsCard label="Candidatos"  :value="metrics.candidates.toLocaleString()" colorClass="text-amber-400"/>
          </div>
        </div>

        <!-- Legend -->
        <div class="rounded-2xl border border-slate-800/60 p-4" style="background:rgba(255,255,255,0.03);backdrop-filter:blur(12px);">
          <h2 class="text-sm font-bold text-slate-300 mb-3">Leyenda</h2>
          <ul class="space-y-2 text-xs">
            <li class="flex items-center gap-2"><span class="w-3 h-3 rounded-full" style="background:radial-gradient(circle,#a5f3fc,#38bdf8,transparent);box-shadow:0 0 6px #38bdf8;"></span><span class="text-slate-400">Partícula normal</span></li>
            <li class="flex items-center gap-2"><span class="w-3 h-3 rounded-full" style="background:radial-gradient(circle,#fef9c3,#fbbf24,transparent);box-shadow:0 0 6px #fbbf24;"></span><span class="text-slate-400">Candidata (consulta)</span></li>
            <li class="flex items-center gap-2"><span class="w-3 h-3 rounded-full" style="background:radial-gradient(circle,#fff,#f43f5e,transparent);box-shadow:0 0 8px #f43f5e;"></span><span class="text-slate-400">Colisión</span></li>
          </ul>
        </div>
      </aside>
    </div>
  </div>
</template>

<style scoped>
input[type=range] { appearance: none; -webkit-appearance: none; }
</style>
