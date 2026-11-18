const colors = {
  1: "#ef7a68",
  2: "#668bd7",
  3: "#8d78c7",
  4: "#55a58b",
  5: "#d6a35f",
  6: "#cf759a",
  7: "#4aa7bd",
  8: "#7f9d63",
};

let processes = [
  { id: 1, arrival: 0, burst: 8 },
  { id: 2, arrival: 1, burst: 4 },
  { id: 3, arrival: 2, burst: 2 },
];

function resetProcessMetrics(items) {
  return items.map((p) => ({
    ...p,
    remaining: p.burst,
    start: null,
    completion: null,
  }));
}

function averages(items) {
  if (!items.length) return { wait: 0, turnaround: 0, response: 0 };
  const totals = items.reduce(
    (acc, p) => {
      acc.wait += p.turnaround - p.burst;
      acc.turnaround += p.turnaround;
      acc.response += p.response;
      return acc;
    },
    { wait: 0, turnaround: 0, response: 0 }
  );
  return {
    wait: totals.wait / items.length,
    turnaround: totals.turnaround / items.length,
    response: totals.response / items.length,
  };
}

function calculateStats(items) {
  const maxTime = items.reduce((max, p) => Math.max(max, p.completion || 0), 0);
  const burstTotal = items.reduce((sum, p) => sum + p.burst, 0);
  return {
    maxTime,
    utilization: maxTime ? burstTotal / maxTime * 100 : 0,
    throughput: maxTime ? items.length / maxTime : 0,
  };
}

function agingThreshold(items) {
  const sum = items.reduce((total, p) => total + p.burst, 0);
  return items.length ? Math.floor((sum / items.length) + 0.5) : 0;
}

function precedesSJF(a, b) {
  if (a.burst !== b.burst) return a.burst < b.burst;
  if (a.arrival !== b.arrival) return a.arrival < b.arrival;
  return a.id < b.id;
}

function scheduleNonPreemptive(items, type) {
  const ps = resetProcessMetrics(items);
  const order = [];
  const gantt = [];
  let time = 0;
  const threshold = agingThreshold(ps);

  while (ps.some((p) => p.completion === null)) {
    const ready = ps.filter((p) => p.completion === null && p.arrival <= time);
    let selected;

    if (ready.length) {
      if (type === "AAFS") {
        const aged = ready.filter((p) => time - p.arrival >= threshold);
        if (aged.length) {
          selected = aged.sort((a, b) =>
            (time - b.arrival) - (time - a.arrival) ||
            a.arrival - b.arrival ||
            a.id - b.id
          )[0];
        } else {
          selected = ready.sort(precedesSJF)[0];
        }
      } else if (type === "SJF") {
        selected = ready.sort(precedesSJF)[0];
      } else {
        selected = ready.sort((a, b) => {
          const ra = ((time - a.arrival) + a.burst) / a.burst;
          const rb = ((time - b.arrival) + b.burst) / b.burst;
          if (ra !== rb) return rb - ra;
          if (a.arrival !== b.arrival) return a.arrival - b.arrival;
          return a.id - b.id;
        })[0];
      }
    }

    if (!selected) {
      const next = ps
        .filter((p) => p.completion === null)
        .sort((a, b) => a.arrival - b.arrival)[0];
      if (next.arrival > time) {
        gantt.push({ processId: 0, start: time, end: next.arrival });
        time = next.arrival;
      }
      continue;
    }

    if (selected.start === null) selected.start = time;
    selected.response = time - selected.arrival;
    gantt.push({ processId: selected.id, start: time, end: time + selected.burst });
    time += selected.burst;
    selected.completion = time;
    selected.turnaround = time - selected.arrival;
    order.push(selected.id);
  }

  return { ps, gantt, order, threshold };
}

function scheduleSRTF(items) {
  const ps = resetProcessMetrics(items);
  const gantt = [];
  const order = [];
  let time = 0;
  let completed = 0;

  while (completed < ps.length) {
    const ready = ps.filter((p) => p.completion === null && p.arrival <= time);
    if (!ready.length) {
      const next = ps
        .filter((p) => p.completion === null)
        .sort((a, b) => a.arrival - b.arrival)[0];
      if (next.arrival > time) {
        gantt.push({ processId: 0, start: time, end: next.arrival });
        time = next.arrival;
      }
      continue;
    }

    const selected = ready.sort((a, b) =>
      a.remaining - b.remaining ||
      a.arrival - b.arrival ||
      a.id - b.id
    )[0];

    if (selected.start === null) {
      selected.start = time;
      selected.response = time - selected.arrival;
    }

    const futureArrival = ps
      .filter((p) => p.completion === null && p.arrival > time)
      .sort((a, b) => a.arrival - b.arrival)[0];
    let run = selected.remaining;
    if (futureArrival) run = Math.min(run, futureArrival.arrival - time);
    if (run <= 0) run = 1;

    const start = time;
    time += run;
    selected.remaining -= run;
    if (gantt.length && gantt[gantt.length - 1].processId === selected.id &&
        gantt[gantt.length - 1].end === start) {
      gantt[gantt.length - 1].end = time;
    } else {
      gantt.push({ processId: selected.id, start, end: time });
    }

    if (selected.remaining === 0) {
      selected.completion = time;
      selected.turnaround = time - selected.arrival;
      order.push(selected.id);
      completed++;
    }
  }

  return { ps, gantt, order, threshold: agingThreshold(ps) };
}

function runAlgorithm(name) {
  if (name === "SRTF") return scheduleSRTF(processes);
  return scheduleNonPreemptive(processes, name);
}

function renderProcessRows() {
  const container = document.getElementById("process-rows");
  container.innerHTML = processes.map((p, index) => `
    <div class="process-row">
      <div class="process-name"><i class="process-dot" style="background:${colors[p.id] || colors[(p.id % 8) + 1]}"></i>P${p.id}</div>
      <input type="number" min="0" value="${p.arrival}" aria-label="P${p.id} arrival time" data-index="${index}" data-field="arrival">
      <input type="number" min="1" value="${p.burst}" aria-label="P${p.id} burst time" data-index="${index}" data-field="burst">
      <button class="remove-process" type="button" aria-label="Remove P${p.id}" data-remove="${index}">×</button>
    </div>
  `).join("");

  document.getElementById("process-count-status").textContent =
    `${processes.length} process${processes.length === 1 ? "" : "es"} loaded`;
}

function renderChart(result) {
  const chart = document.getElementById("gantt-chart");
  const axis = document.getElementById("chart-axis");
  if (!result.gantt.length) {
    chart.innerHTML = "";
    axis.innerHTML = "";
    return;
  }

  const start = Math.min(...result.gantt.map((s) => s.start));
  const end = Math.max(...result.gantt.map((s) => s.end));
  const span = end - start || 1;
  axis.innerHTML = "";
  const ticks = [...new Set([start, ...result.gantt.map((s) => s.end)])];
  ticks.forEach((tick) => {
    const left = (tick - start) / span * 100;
    const el = document.createElement("span");
    el.className = "axis-tick";
    el.style.left = `${left}%`;
    el.textContent = tick;
    axis.appendChild(el);
  });

  chart.innerHTML = '<span class="gantt-label">CPU</span>' + result.gantt.map((segment) => {
    const width = (segment.end - segment.start) / span * 100;
    if (segment.processId === 0) {
      return `<div class="gantt-block idle" style="flex-basis:${width}%">Idle</div>`;
    }
    return `<div class="gantt-block" style="flex-basis:${width}%;background:${colors[segment.processId] || "#ef7a68"}">P${segment.processId}</div>`;
  }).join("");
}

function renderResults() {
  const name = document.getElementById("algorithm-select").value;
  const result = runAlgorithm(name);
  const avgs = averages(result.ps);
  const stats = calculateStats(result.ps);

  document.getElementById("threshold-value").textContent = agingThreshold(processes);
  document.getElementById("avg-wait").textContent = avgs.wait.toFixed(2);
  document.getElementById("avg-turnaround").textContent = avgs.turnaround.toFixed(2);
  document.getElementById("avg-response").textContent = avgs.response.toFixed(2);
  document.getElementById("cpu-utilization").textContent = `${stats.utilization.toFixed(0)}%`;
  renderChart(result);

  const comparison = ["AAFS", "SJF", "SRTF", "HRRN"].map((algorithm) => {
    const current = runAlgorithm(algorithm);
    const avg = averages(current.ps);
    const stat = calculateStats(current.ps);
    return { algorithm, avg, stat };
  });

  document.getElementById("comparison-body").innerHTML = comparison.map((row) => `
    <tr>
      <td><span class="algo-name"><i class="algo-dot" style="background:${row.algorithm === "AAFS" ? "#ef7a68" : row.algorithm === "SJF" ? "#668bd7" : row.algorithm === "SRTF" ? "#8d78c7" : "#55a58b"}"></i>${row.algorithm}</span></td>
      <td>${row.avg.wait.toFixed(2)}</td>
      <td>${row.avg.turnaround.toFixed(2)}</td>
      <td>${row.avg.response.toFixed(2)}</td>
      <td>${row.stat.utilization.toFixed(1)}%</td>
      <td>${row.stat.throughput.toFixed(4)}</td>
    </tr>
  `).join("");
}

document.getElementById("process-rows").addEventListener("input", (event) => {
  if (!event.target.matches("input")) return;
  const index = Number(event.target.dataset.index);
  const field = event.target.dataset.field;
  const value = Number(event.target.value);
  if (Number.isFinite(value) && value >= 0) processes[index][field] = value;
});

document.getElementById("process-rows").addEventListener("click", (event) => {
  const removeButton = event.target.closest("[data-remove]");
  if (!removeButton) return;
  processes.splice(Number(removeButton.dataset.remove), 1);
  processes = processes.map((p, index) => ({ ...p, id: index + 1 }));
  renderProcessRows();
  renderResults();
});

document.getElementById("add-process").addEventListener("click", () => {
  const nextId = processes.length ? Math.max(...processes.map((p) => p.id)) + 1 : 1;
  processes.push({ id: nextId, arrival: 0, burst: 1 });
  renderProcessRows();
  renderResults();
});

document.getElementById("run-simulation").addEventListener("click", () => {
  renderResults();
  document.querySelector(".chart-card").animate(
    [{ transform: "scale(.995)", opacity: .75 }, { transform: "scale(1)", opacity: 1 }],
    { duration: 260 }
  );
});

document.getElementById("algorithm-select").addEventListener("change", renderResults);
document.getElementById("theme-toggle").addEventListener("click", () => {
  document.body.classList.toggle("dark");
});

renderProcessRows();
renderResults();
