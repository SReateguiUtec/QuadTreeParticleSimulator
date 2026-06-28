<script setup>
import { ref, watch, onMounted, onUnmounted } from 'vue';

const props = defineProps({
  tree: Object,
  width: { type: Number, default: 800 },
  height: { type: Number, default: 600 }
});

const canvasRef = ref(null);
let ctx = null;

// Calculate how many leaf nodes are under a node to allocate horizontal space
const countLeaves = (node) => {
  if (!node) return 0;
  if (!node.divided || !node.children) {
    node.leafCount = 1;
    return 1;
  }
  let sum = 0;
  for (let child of node.children) {
    sum += countLeaves(child);
  }
  node.leafCount = sum;
  return sum;
};

// Calculate depth of the tree
const calculateDepth = (node) => {
  if (!node) return 0;
  if (!node.divided || !node.children) return 1;
  let max = 0;
  for (let child of node.children) {
    max = Math.max(max, calculateDepth(child));
  }
  return max + 1;
};

const drawTree = () => {
  if (!canvasRef.value || !props.tree) return;
  if (!ctx) ctx = canvasRef.value.getContext('2d');
  
  const w = canvasRef.value.width;
  const h = canvasRef.value.height;
  ctx.clearRect(0, 0, w, h);
  
  const totalLeaves = countLeaves(props.tree);
  const maxDepth = calculateDepth(props.tree);
  
  ctx.globalCompositeOperation = 'source-over';
  
  const vSpacing = (h - 100) / Math.max(1, maxDepth);
  
  // Parámetros de diseño (estética del amigo)
  const slotSize = 10;
  const slotGap = 3;
  const numSlots = 4; // Dibujamos 4 ranuras visuales por nodo
  const blockWidth = numSlots * slotSize + (numSlots + 1) * slotGap;
  const blockHeight = slotSize + 2 * slotGap;
  
  const drawNode = (node, x, y, availableWidth, depth) => {
    // 1. Dibujar líneas hacia los hijos
    if (node.divided && node.children) {
      let currentX = x - availableWidth / 2;
      for (let i = 0; i < node.children.length; i++) {
        const child = node.children[i];
        const childWidth = (child.leafCount / node.leafCount) * availableWidth;
        const childX = currentX + childWidth / 2;
        const childY = y + vSpacing;
        
        ctx.beginPath();
        ctx.moveTo(x, y + blockHeight / 2);
        ctx.lineTo(childX, childY - blockHeight / 2);
        
        ctx.strokeStyle = '#3498db'; // Azul claro sin neón
        ctx.lineWidth = 1.8;
        ctx.stroke();
        
        drawNode(child, childX, childY, childWidth, depth + 1);
        currentX += childWidth;
      }
    }
    
    // 2. Dibujar el contenedor del nodo
    const bx = x - blockWidth / 2;
    const by = y - blockHeight / 2;
    
    ctx.fillStyle = '#eef4fb';
    ctx.beginPath();
    if (ctx.roundRect) {
        ctx.roundRect(bx, by, blockWidth, blockHeight, 4);
    } else {
        ctx.rect(bx, by, blockWidth, blockHeight);
    }
    ctx.fill();
    
    ctx.strokeStyle = '#3498db';
    ctx.lineWidth = 1.5;
    ctx.stroke();
    
    // 3. Dibujar las ranuras (rojo si hay partícula, gris si está vacío)
    for (let i = 0; i < numSlots; i++) {
      const slotX = bx + slotGap + i * (slotSize + slotGap);
      const slotY = by + slotGap;
      
      const hasParticle = i < node.particles;
      ctx.fillStyle = hasParticle ? '#e74c3c' : '#ecf0f1';
      ctx.strokeStyle = hasParticle ? '#c0392b' : '#95a5a6';
      ctx.lineWidth = 1;
      
      ctx.beginPath();
      if (ctx.roundRect) {
          ctx.roundRect(slotX, slotY, slotSize, slotSize, 2);
      } else {
          ctx.rect(slotX, slotY, slotSize, slotSize);
      }
      ctx.fill();
      ctx.stroke();
    }
  };

  drawNode(props.tree, w / 2, 50, w - 80, 0);
};

// Resize handler
const resizeCanvas = () => {
  if (canvasRef.value) {
    const parent = canvasRef.value.parentElement;
    canvasRef.value.width = parent.clientWidth;
    canvasRef.value.height = parent.clientHeight;
    drawTree();
  }
};

onMounted(() => {
  resizeCanvas();
  window.addEventListener('resize', resizeCanvas);
});

onUnmounted(() => {
  window.removeEventListener('resize', resizeCanvas);
});

watch(() => props.tree, () => {
  drawTree();
}, { deep: false });
</script>

<template>
  <div class="absolute inset-0 w-full h-full pointer-events-none overflow-hidden z-20 flex items-center justify-center bg-white/85 backdrop-blur-[2px] transition-colors duration-500">
    <div class="absolute top-4 left-4 text-xs font-bold text-[#3498db] flex items-center gap-2">
      <span class="w-2 h-2 rounded-full bg-[#3498db]"></span>
      QUADTREE HIERARCHY MAP
    </div>
    <canvas ref="canvasRef" class="w-full h-full"></canvas>
  </div>
</template>
