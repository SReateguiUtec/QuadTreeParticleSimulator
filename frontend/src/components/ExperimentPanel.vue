<script setup>
import { ref, computed } from 'vue';

const BASE = 'http://localhost:8080';

const results   = ref([]);
const running   = ref(false);
const done      = ref(false);
const error     = ref('');
const progress  = ref('');
const frames    = ref(30);

// ── Fetch experiments from backend ──────────────────────────────────────────
const runExperiments = async () => {
  running.value  = true;
  done.value     = false;
  error.value    = '';
  results.value  = [];
  progress.value = 'Ejecutando 9 escenarios (3 tamaños × 3 distribuciones)…';

  try {
    const res = await fetch(
      `${BASE}/experiment?w=1000&h=1000&r=4&cap=4&frames=${frames.value}`
    );
    if (!res.ok) throw new Error(`HTTP ${res.status}`);
    results.value  = await res.json();
    done.value     = true;
    progress.value = '';
  } catch (e) {
    error.value    = e.message;
    progress.value = '';
  } finally {
    running.value = false;
  }
};

// ── Helpers ──────────────────────────────────────────────────────────────────
const sizes = [1000, 5000, 10000];
const distNames = ['uniforme', 'clusters', 'alta densidad'];
const distColors = {
  'uniforme':      { bg: 'rgba(14,165,233,0.12)', border: 'rgba(14,165,233,0.4)',  text: '#38bdf8' },
  'clusters':      { bg: 'rgba(168,85,247,0.12)', border: 'rgba(168,85,247,0.4)', text: '#c084fc' },
  'alta densidad': { bg: 'rgba(249,115,22,0.12)', border: 'rgba(249,115,22,0.4)', text: '#fb923c' },
};

const getResult = (n, dist) =>
  results.value.find(r => r.n === n && r.distribution === dist);

// Format helpers
const fmt  = (v) => v == null ? '—' : Number(v).toLocaleString('es', { maximumFractionDigits: 0 });
const fmtUs = (v) => v == null ? '—' : (v < 1000 ? `${Math.round(v)} µs` : `${(v/1000).toFixed(2)} ms`);
const fmtX  = (v) => v == null ? '—' : `${Number(v).toFixed(1)}×`;

// ── Bar chart data per metric ─────────────────────────────────────────────────
const chartMetric = ref('qtComparisons');
const chartMetrics = [
  { key: 'qtComparisons',  label: 'Comparaciones QuadTree' },
  { key: 'bfComparisons',  label: 'Comparaciones Fuerza Bruta' },
  { key: 'qtTimeUs',       label: 'Tiempo QuadTree (µs)' },
  { key: 'bfTimeUs',       label: 'Tiempo Fuerza Bruta (µs)' },
  { key: 'candidatesPerParticle', label: 'Candidatos / Partícula' },
  { key: 'speedup',        label: 'Speedup (BF / QT)' },
];

const maxBarValue = computed(() => {
  if (!results.value.length) return 1;
  return Math.max(...results.value.map(r => r[chartMetric.value] ?? 0), 1);
});

const barWidth = (r) => {
  const v = r[chartMetric.value] ?? 0;
  return `${(v / maxBarValue.value) * 100}%`;
};

const barLabel = (r) => {
  const v = r[chartMetric.value];
  if (v == null) return '—';
  if (chartMetric.value.includes('Time')) return fmtUs(v);
  if (chartMetric.value === 'speedup') return fmtX(v);
  if (chartMetric.value === 'candidatesPerParticle') return v.toFixed(2);
  return fmt(v);
};

// Group bars by size
const chartRows = computed(() => sizes.map(n => ({
  n,
  items: distNames.map(dist => {
    const r = getResult(n, dist);
    return { dist, r, width: r ? barWidth(r) : '0%', label: r ? barLabel(r) : '—' };
  })
})));
</script>

<template>
  <div class="flex flex-col gap-6 p-1">

    <!-- ── Header ──────────────────────────────────────────────────────────── -->
    <div class="rounded-2xl border border-indigo-500/20 p-5"
         style="background:rgba(99,102,241,0.06);">
      <div class="flex flex-wrap items-center justify-between gap-4">
        <div>
          <h2 class="text-lg font-black text-indigo-300 flex items-center gap-2">
            <svg class="w-5 h-5" fill="none" stroke="currentColor" viewBox="0 0 24 24">
              <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2"
                    d="M9 19v-6a2 2 0 00-2-2H5a2 2 0 00-2 2v6a2 2 0 002 2h2a2 2 0 002-2zm0 0V9a2 2 0 012-2h2a2 2 0 012 2v10m-6 0a2 2 0 002 2h2a2 2 0 002-2m0 0V5a2 2 0 012-2h2a2 2 0 012 2v14a2 2 0 01-2 2h-2a2 2 0 01-2-2z"/>
            </svg>
            Experimentos: QuadTree vs Fuerza Bruta
          </h2>
          <p class="text-xs text-slate-500 mt-1 font-mono">
            3 tamaños × 3 distribuciones · {{ frames }} frames por caso · mundo 1000×1000
          </p>
        </div>
        <div class="flex items-center gap-3">
          <label class="flex flex-col gap-1">
            <span class="text-[10px] font-semibold tracking-widest text-slate-500 uppercase">Frames/caso</span>
            <input type="number" v-model="frames" min="5" max="200"
                   class="w-20 bg-white/5 border border-slate-800 rounded-lg px-3 py-1.5 text-sm text-white focus:outline-none focus:border-indigo-500"/>
          </label>
          <label class="flex flex-col gap-1">
            <span class="text-[10px] font-semibold tracking-widest text-transparent uppercase select-none">·</span>
            <button @click="runExperiments" :disabled="running"
                    class="flex items-center gap-2 px-5 py-2 rounded-xl text-sm font-bold transition-all hover:brightness-110 active:scale-95 disabled:opacity-40"
                    style="background:linear-gradient(135deg,#6366f1,#8b5cf6);color:#fff;box-shadow:0 0 20px rgba(99,102,241,0.4);">
              <svg v-if="running" class="w-4 h-4 animate-spin" fill="none" viewBox="0 0 24 24">
                <circle class="opacity-25" cx="12" cy="12" r="10" stroke="currentColor" stroke-width="4"/>
                <path class="opacity-75" fill="currentColor" d="M4 12a8 8 0 018-8v8z"/>
              </svg>
              <svg v-else class="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2"
                      d="M14.752 11.168l-3.197-2.132A1 1 0 0010 9.87v4.263a1 1 0 001.555.832l3.197-2.132a1 1 0 000-1.664z"/>
              </svg>
              {{ running ? 'Ejecutando…' : 'Ejecutar Experimentos' }}
            </button>
          </label>
        </div>
      </div>

      <!-- Progress / error -->
      <div v-if="progress" class="mt-4 flex items-center gap-3">
        <div class="flex-1 h-1.5 rounded-full overflow-hidden" style="background:rgba(255,255,255,0.06);">
          <div class="h-full rounded-full animate-pulse"
               style="width:60%;background:linear-gradient(90deg,#6366f1,#8b5cf6);"/>
        </div>
        <span class="text-xs text-slate-500 font-mono">{{ progress }}</span>
      </div>
      <div v-if="error" class="mt-3 text-xs text-rose-400 font-mono">
        {{ error }} — ¿Está el backend corriendo?
      </div>
    </div>

    <!-- ── Main results table ──────────────────────────────────────────────── -->
    <div v-if="done && results.length" class="flex flex-col gap-5">

      <!-- Per-size blocks -->
      <div v-for="n in sizes" :key="n"
           class="rounded-2xl border border-slate-800/60 overflow-hidden"
           style="background:rgba(255,255,255,0.02);">

        <!-- Size header -->
        <div class="px-5 py-3 border-b border-slate-800/60 flex items-center gap-3"
             style="background:rgba(255,255,255,0.03);">
          <span class="text-2xl font-black tabular-nums"
                style="background:linear-gradient(90deg,#00d4ff,#6366f1);-webkit-background-clip:text;-webkit-text-fill-color:transparent;">
            {{ n.toLocaleString() }}
          </span>
          <span class="text-slate-500 font-mono text-xs tracking-widest">PARTÍCULAS</span>
        </div>

        <!-- Table -->
        <div class="overflow-x-auto">
          <table class="w-full text-xs font-mono">
            <thead>
              <tr class="text-[10px] tracking-widest text-slate-600 uppercase border-b border-slate-800/40">
                <th class="text-left px-5 py-2.5 font-semibold">Distribución</th>
                <th class="text-right px-4 py-2.5 font-semibold">T. Build QT</th>
                <th class="text-right px-4 py-2.5 font-semibold">T. QT</th>
                <th class="text-right px-4 py-2.5 font-semibold">T. BF</th>
                <th class="text-right px-4 py-2.5 font-semibold">Comp. QT</th>
                <th class="text-right px-4 py-2.5 font-semibold">Comp. BF</th>
                <th class="text-right px-4 py-2.5 font-semibold">Cand./Part.</th>
                <th class="text-right px-4 py-2.5 font-semibold">Colisiones</th>
                <th class="text-right px-4 py-2.5 font-semibold">Speedup</th>
              </tr>
            </thead>
            <tbody>
              <tr v-for="dist in distNames" :key="dist"
                  class="border-b border-slate-800/30 hover:bg-white/[0.02] transition-colors">
                <template v-if="getResult(n, dist)">
                  <td class="px-5 py-3">
                    <span class="inline-flex items-center gap-1.5 px-2.5 py-1 rounded-lg text-[11px] font-bold capitalize"
                          :style="`background:${distColors[dist].bg};border:1px solid ${distColors[dist].border};color:${distColors[dist].text}`">
                      {{ dist }}
                    </span>
                  </td>
                  <td class="text-right px-4 py-3 text-slate-400">{{ fmtUs(getResult(n,dist).buildTimeUs) }}</td>
                  <td class="text-right px-4 py-3 text-teal-400 font-bold">{{ fmtUs(getResult(n,dist).qtTimeUs) }}</td>
                  <td class="text-right px-4 py-3 text-rose-400">{{ fmtUs(getResult(n,dist).bfTimeUs) }}</td>
                  <td class="text-right px-4 py-3 text-teal-300">{{ fmt(getResult(n,dist).qtComparisons) }}</td>
                  <td class="text-right px-4 py-3 text-rose-300">{{ fmt(getResult(n,dist).bfComparisons) }}</td>
                  <td class="text-right px-4 py-3 text-amber-300">{{ getResult(n,dist).candidatesPerParticle.toFixed(2) }}</td>
                  <td class="text-right px-4 py-3 text-slate-400">{{ fmt(getResult(n,dist).collisions) }}</td>
                  <td class="text-right px-4 py-3">
                    <span class="font-black text-emerald-400"
                          style="text-shadow:0 0 10px rgba(52,211,153,0.5);">
                      {{ fmtX(getResult(n,dist).speedup) }}
                    </span>
                  </td>
                </template>
                <template v-else>
                  <td class="px-5 py-3 text-slate-700">{{ dist }}</td>
                  <td colspan="8" class="px-4 py-3 text-slate-700 text-center">sin datos</td>
                </template>
              </tr>
            </tbody>
          </table>
        </div>
      </div>

      <!-- ── Bar chart section ─────────────────────────────────────────────── -->
      <div class="rounded-2xl border border-slate-800/60 p-5"
           style="background:rgba(255,255,255,0.02);">

        <div class="flex flex-wrap items-center gap-3 mb-5">
          <h3 class="text-sm font-bold text-slate-300">Visualización comparativa</h3>
          <select v-model="chartMetric"
                  class="ml-auto bg-white/5 border border-slate-800/60 rounded-lg px-3 py-1.5 text-xs text-white focus:outline-none focus:border-indigo-500">
            <option v-for="m in chartMetrics" :key="m.key" :value="m.key">{{ m.label }}</option>
          </select>
        </div>

        <!-- Bars grouped by size -->
        <div class="flex flex-col gap-6">
          <div v-for="row in chartRows" :key="row.n">
            <div class="text-[10px] font-semibold tracking-widest text-slate-600 uppercase mb-2">
              {{ row.n.toLocaleString() }} partículas
            </div>
            <div class="flex flex-col gap-1.5">
              <div v-for="item in row.items" :key="item.dist" class="flex items-center gap-3">
                <span class="w-24 text-right text-[10px] font-bold capitalize shrink-0"
                      :style="`color:${distColors[item.dist].text}`">
                  {{ item.dist }}
                </span>
                <div class="flex-1 h-5 rounded-full overflow-hidden relative"
                     style="background:rgba(255,255,255,0.04);">
                  <div class="h-full rounded-full transition-all duration-700"
                       :style="`
                         width:${item.width};
                         background:linear-gradient(90deg,${distColors[item.dist].border},${distColors[item.dist].text});
                         box-shadow:0 0 8px ${distColors[item.dist].text}40;
                       `"/>
                </div>
                <span class="text-xs font-mono text-slate-400 w-24 shrink-0">{{ item.label }}</span>
              </div>
            </div>
          </div>
        </div>

        <!-- Legend -->
        <div class="flex items-center gap-5 mt-5 pt-4 border-t border-slate-800/40">
          <div v-for="(c, dist) in distColors" :key="dist"
               class="flex items-center gap-1.5">
            <span class="w-2.5 h-2.5 rounded-full" :style="`background:${c.text}`"/>
            <span class="text-xs text-slate-500 capitalize">{{ dist }}</span>
          </div>
        </div>
      </div>

      <!-- ── Summary insight cards ─────────────────────────────────────────── -->
      <div class="grid grid-cols-1 sm:grid-cols-3 gap-4">
        <div v-for="n in sizes" :key="'card'+n"
             class="rounded-2xl border border-slate-800/60 p-4"
             style="background:rgba(255,255,255,0.02);">
          <div class="text-xs text-slate-600 font-mono uppercase tracking-widest mb-3">
            {{ n.toLocaleString() }} partículas — max speedup
          </div>
          <div class="flex flex-col gap-2">
            <div v-for="dist in distNames" :key="dist+'c'+n"
                 class="flex items-center justify-between">
              <span class="text-[11px] capitalize" :style="`color:${distColors[dist].text}`">{{ dist }}</span>
              <span class="text-base font-black text-emerald-400 tabular-nums">
                {{ getResult(n, dist) ? fmtX(getResult(n, dist).speedup) : '—' }}
              </span>
            </div>
          </div>
        </div>
      </div>

    </div>

    <!-- ── Empty state ─────────────────────────────────────────────────────── -->
    <div v-if="!done && !running"
         class="rounded-2xl border border-dashed border-slate-800 p-12 flex flex-col items-center gap-4">
      <svg class="w-12 h-12 text-slate-700" fill="none" stroke="currentColor" viewBox="0 0 24 24">
        <path stroke-linecap="round" stroke-linejoin="round" stroke-width="1.5"
              d="M9 19v-6a2 2 0 00-2-2H5a2 2 0 00-2 2v6a2 2 0 002 2h2a2 2 0 002-2zm0 0V9a2 2 0 012-2h2a2 2 0 012 2v10m-6 0a2 2 0 002 2h2a2 2 0 002-2m0 0V5a2 2 0 012-2h2a2 2 0 012 2v14a2 2 0 01-2 2h-2a2 2 0 01-2-2z"/>
      </svg>
      <p class="text-slate-600 text-sm text-center max-w-xs">
        Presiona <strong class="text-indigo-400">Ejecutar Experimentos</strong> para correr los 9 escenarios y generar la tabla comparativa requerida por el proyecto.
      </p>
    </div>

  </div>
</template>
