const data = window.visualizationData;

const canvas = document.getElementById("simulationCanvas");
const context = canvas.getContext("2d");
const frameLabel = document.getElementById("frameLabel");
const frameSlider = document.getElementById("frameSlider");
const playButton = document.getElementById("playButton");
const previousButton = document.getElementById("previousButton");
const nextButton = document.getElementById("nextButton");
const modeButton = document.getElementById("modeButton");

const metricElements = {
    particleCount: document.getElementById("particleCount"),
    nodeCount: document.getElementById("nodeCount"),
    nodesVisited: document.getElementById("nodesVisited"),
    particlesChecked: document.getElementById("particlesChecked"),
    candidateCount: document.getElementById("candidateCount"),
    collisionCount: document.getElementById("collisionCount")
};

let currentFrame = 0;
let playing = true;
let spaceMode = true;
let lastStep = 0;
const frameDurationMs = 90;

function resizeCanvas() {
    const rect = canvas.getBoundingClientRect();
    const size = Math.max(320, Math.floor(Math.min(rect.width, rect.height || rect.width)));

    canvas.width = size;
    canvas.height = size;
    draw();
}

function scaleX(value) {
    return value / data.width * canvas.width;
}

function scaleY(value) {
    return value / data.height * canvas.height;
}

function scaleLength(value) {
    return value / data.width * canvas.width;
}

function drawBoundaries(frame) {
    context.save();
    context.strokeStyle = spaceMode ? "rgba(56, 189, 248, 0.22)" : "rgba(148, 163, 184, 0.55)";
    context.lineWidth = 1;

    for (const boundary of frame.boundaries) {
        context.strokeRect(
            scaleX(boundary.x),
            scaleY(boundary.y),
            scaleLength(boundary.w),
            scaleLength(boundary.h)
        );
    }

    context.restore();
}

function drawQuery(frame) {
    context.save();
    context.beginPath();
    context.arc(
        scaleX(frame.query.x),
        scaleY(frame.query.y),
        scaleLength(frame.query.radius),
        0,
        Math.PI * 2
    );
    context.fillStyle = spaceMode ? "rgba(34, 211, 238, 0.07)" : "rgba(124, 58, 237, 0.10)";
    context.strokeStyle = spaceMode ? "rgba(103, 232, 249, 0.85)" : "#7c3aed";
    context.lineWidth = 3;
    context.fill();
    context.stroke();
    context.restore();
}

function drawCollisions(frame, particleById) {
    context.save();
    context.strokeStyle = spaceMode ? "rgba(248, 113, 113, 0.38)" : "rgba(220, 38, 38, 0.48)";
    context.lineWidth = 1.25;

    for (const collision of frame.collisions) {
        const first = particleById.get(collision.a);
        const second = particleById.get(collision.b);

        if (!first || !second) {
            continue;
        }

        context.beginPath();
        context.moveTo(scaleX(first.x), scaleY(first.y));
        context.lineTo(scaleX(second.x), scaleY(second.y));
        context.stroke();
    }

    context.restore();
}

function drawParticles(frame) {
    for (const particle of frame.particles) {
        let fill = spaceMode ? "#dbeafe" : "#2563eb";
        let stroke = spaceMode ? "rgba(147, 197, 253, 0.7)" : "#1e3a8a";
        let radius = Math.max(spaceMode ? 1.6 : 2.8, scaleLength(particle.r * (spaceMode ? 0.62 : 0.75)));

        if (particle.candidate) {
            fill = spaceMode ? "#facc15" : "#f59e0b";
            stroke = spaceMode ? "rgba(254, 240, 138, 0.9)" : "#92400e";
            radius = Math.max(radius, spaceMode ? 3.2 : 4.8);
        }

        if (particle.colliding) {
            fill = spaceMode ? "#fb7185" : "#ef4444";
            stroke = spaceMode ? "rgba(254, 202, 202, 0.95)" : "#7f1d1d";
            radius = Math.max(radius, spaceMode ? 4.2 : 5.8);
        }

        if (spaceMode) {
            context.save();
            context.shadowBlur = particle.colliding ? 12 : particle.candidate ? 9 : 5;
            context.shadowColor = fill;
        }

        context.beginPath();
        context.arc(scaleX(particle.x), scaleY(particle.y), radius, 0, Math.PI * 2);
        context.fillStyle = fill;
        context.strokeStyle = stroke;
        context.lineWidth = 0.8;
        context.fill();
        context.stroke();

        if (spaceMode) {
            context.restore();
        }
    }
}

function drawBackground() {
    if (!spaceMode) {
        context.fillStyle = "#ffffff";
        context.fillRect(0, 0, canvas.width, canvas.height);
        context.strokeStyle = "#111827";
        context.lineWidth = 2;
        context.strokeRect(0, 0, canvas.width, canvas.height);
        return;
    }

    const gradient = context.createRadialGradient(
        canvas.width * 0.5,
        canvas.height * 0.48,
        canvas.width * 0.08,
        canvas.width * 0.5,
        canvas.height * 0.5,
        canvas.width * 0.72
    );

    gradient.addColorStop(0, "#111827");
    gradient.addColorStop(0.45, "#070b18");
    gradient.addColorStop(1, "#020617");
    context.fillStyle = gradient;
    context.fillRect(0, 0, canvas.width, canvas.height);

    for (let i = 0; i < 140; i++) {
        const x = ((Math.sin(i * 12.9898) * 43758.5453) % 1 + 1) % 1 * canvas.width;
        const y = ((Math.sin(i * 78.233) * 24634.6345) % 1 + 1) % 1 * canvas.height;
        const alpha = 0.25 + (((i * 37) % 100) / 100) * 0.55;
        const radius = i % 11 === 0 ? 1.4 : 0.8;

        context.beginPath();
        context.arc(x, y, radius, 0, Math.PI * 2);
        context.fillStyle = `rgba(226, 232, 240, ${alpha})`;
        context.fill();
    }

    context.strokeStyle = "rgba(148, 163, 184, 0.24)";
    context.lineWidth = 2;
    context.strokeRect(0, 0, canvas.width, canvas.height);
}

function updateMetrics(frame) {
    frameLabel.textContent = `Frame ${frame.index + 1} / ${data.frames.length}`;
    metricElements.particleCount.textContent = frame.particles.length.toLocaleString("es-PE");
    metricElements.nodeCount.textContent = frame.metrics.quadtreeNodes.toLocaleString("es-PE");
    metricElements.nodesVisited.textContent = frame.metrics.nodesVisited.toLocaleString("es-PE");
    metricElements.particlesChecked.textContent = frame.metrics.particlesChecked.toLocaleString("es-PE");
    metricElements.candidateCount.textContent = frame.metrics.candidates.toLocaleString("es-PE");
    metricElements.collisionCount.textContent = frame.metrics.collisions.toLocaleString("es-PE");
}

function draw() {
    const frame = data.frames[currentFrame];
    const particleById = new Map(frame.particles.map((particle) => [particle.id, particle]));

    context.clearRect(0, 0, canvas.width, canvas.height);
    drawBackground();

    drawBoundaries(frame);
    drawQuery(frame);
    drawCollisions(frame, particleById);
    drawParticles(frame);
    updateMetrics(frame);
    frameSlider.value = String(currentFrame);
}

function setFrame(index) {
    currentFrame = (index + data.frames.length) % data.frames.length;
    draw();
}

function step(timestamp) {
    if (playing && timestamp - lastStep >= frameDurationMs) {
        setFrame(currentFrame + 1);
        lastStep = timestamp;
    }

    requestAnimationFrame(step);
}

playButton.addEventListener("click", () => {
    playing = !playing;
    playButton.textContent = playing ? "Pausar" : "Reproducir";
});

previousButton.addEventListener("click", () => {
    playing = false;
    playButton.textContent = "Reproducir";
    setFrame(currentFrame - 1);
});

nextButton.addEventListener("click", () => {
    playing = false;
    playButton.textContent = "Reproducir";
    setFrame(currentFrame + 1);
});

modeButton.addEventListener("click", () => {
    spaceMode = !spaceMode;
    document.body.classList.toggle("space-mode", spaceMode);
    modeButton.textContent = spaceMode ? "Modo plano" : "Modo espacio";
    draw();
});

frameSlider.addEventListener("input", () => {
    playing = false;
    playButton.textContent = "Reproducir";
    setFrame(Number(frameSlider.value));
});

if (!data || !Array.isArray(data.frames) || data.frames.length === 0) {
    throw new Error("No se encontro visualization_data.js generado por C++.");
}

frameSlider.max = String(data.frames.length - 1);
document.body.classList.toggle("space-mode", spaceMode);
resizeCanvas();
window.addEventListener("resize", resizeCanvas);
requestAnimationFrame(step);
