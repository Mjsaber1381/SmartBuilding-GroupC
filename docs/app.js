
'use strict';

// ============================================================
// SmartBuilding Group C — Final Student Dashboard
// Device: ENV_GROUP_C_01
// Public GitHub Pages JavaScript
//
// SECURITY:
// Do not hardcode dashboard access codes, ESP32 tokens,
// Supabase service_role keys, or other secrets.
// ============================================================

const API_URL =
  'https://izbtxfmkfmkfledrqtpu.supabase.co/functions/v1/read-group-data';

const DEVICE_ID = 'ENV_GROUP_C_01';

const MAX_POINTS = 60;
const REFRESH_MS = 30000;
const REQUEST_TIMEOUT_MS = 12000;
const STALE_AFTER_MS = 120000;
const FUTURE_TOLERANCE_MS = 60000;

const $ = id => document.getElementById(id);

let accessCode = '';
let rows = [];
let timer = null;
let inFlight = false;
let connecting = false;
let sessionId = 0;
let activeController = null;

// ============================================================
// Data Formatting
// ============================================================

const num = v => {
  if (
    v === null ||
    v === undefined ||
    v === '' ||
    typeof v === 'boolean'
  ) {
    return null;
  }

  const n = Number(v);
  return Number.isFinite(n) ? n : null;
};

const fmt = (v, digits = 1) => {
  const n = num(v);
  return n === null ? '—' : n.toFixed(digits);
};

const timeText = v => {
  if (!v) return '—';

  const d = new Date(v);

  return Number.isFinite(d.getTime())
    ? d.toLocaleString()
    : '—';
};

const difference = (r, a, b) => {
  const x = num(r[a]);
  const y = num(r[b]);

  return x === null || y === null
    ? null
    : y - x;
};

// ============================================================
// UI Status
// ============================================================

function status(text, css = '') {
  $('status').textContent = text;
  $('status').className = 'status ' + css;
  document.body.dataset.state = css || 'offline';
}

function message(text) {
  $('message').textContent = text;
}

function clearReadings() {
  const ids = [
    't1',
    't2',
    'dt',
    'rh1',
    'rh2',
    'drh',
    'lux',
    'motion',
    'sampletime',
    'recorded',
    'received'
  ];

  for (const id of ids) {
    $(id).textContent = '—';
  }
}

// ============================================================
// API Communication
// ============================================================

async function api(action, key, signal) {
  const response = await fetch(API_URL, {
    method: 'POST',
    cache: 'no-store',
    signal,
    headers: {
      'Content-Type': 'application/json',
      'x-dashboard-key': key
    },
    body: JSON.stringify({
      action,
      device_id: DEVICE_ID
    })
  });

  let result;

  try {
    result = await response.json();
  } catch {
    throw new Error('Invalid server response.');
  }

  if (!response.ok) {
    if (response.status === 401) {
      throw new Error(
        'Incorrect Group C access code.'
      );
    }

    if (response.status === 403) {
      throw new Error(
        'Access denied. Check the access code and allowed website origin.'
      );
    }

    throw new Error(
      result?.error || `HTTP ${response.status}`
    );
  }

  if (
    !result ||
    typeof result !== 'object' ||
    result.device_id !== DEVICE_ID
  ) {
    throw new Error(
      'Unexpected device ID returned by server.'
    );
  }

  if (
    action === 'recent' &&
    !Array.isArray(result.rows)
  ) {
    throw new Error(
      'Invalid recent-data response.'
    );
  }

  if (
    action === 'latest' &&
    !Object.prototype.hasOwnProperty.call(
      result,
      'latest'
    )
  ) {
    throw new Error(
      'Invalid latest-data response.'
    );
  }

  return result;
}

// ============================================================
// Historical Data Management
// ============================================================

function appendRow(r) {
  if (
    !r ||
    typeof r.record_id !== 'string' ||
    !Number.isFinite(Date.parse(r.recorded_at))
  ) {
    return;
  }

  const index = rows.findIndex(
    x => x.record_id === r.record_id
  );

  if (index >= 0) {
    rows[index] = r;
  } else {
    rows.push(r);
  }

  rows.sort((a, b) =>
    Date.parse(a.recorded_at) -
    Date.parse(b.recorded_at) ||
    String(a.record_id).localeCompare(
      String(b.record_id)
    )
  );

  rows = rows.slice(-MAX_POINTS);
}

// ============================================================
// Latest Sensor Readings
// ============================================================

function showLatest(r) {
  if (!r) {
    clearReadings();
    status('No samples', 'stale');
    message('No records found for Group C.');
    return;
  }

  // Temperature
  $('t1').textContent =
    fmt(r.temperature_1_c);

  $('t2').textContent =
    fmt(r.temperature_2_c);

  $('dt').textContent = fmt(
    difference(
      r,
      'temperature_1_c',
      'temperature_2_c'
    ),
    2
  );

  // Relative humidity
  $('rh1').textContent =
    fmt(r.humidity_1_rh);

  $('rh2').textContent =
    fmt(r.humidity_2_rh);

  $('drh').textContent = fmt(
    difference(
      r,
      'humidity_1_rh',
      'humidity_2_rh'
    ),
    2
  );

  // Illuminance
  $('lux').textContent =
    fmt(r.illuminance_lux, 0);

  // PIR
  $('motion').textContent =
    r.motion_detected === true
      ? 'Detected'
      : r.motion_detected === false
        ? 'Clear'
        : '—';

  // Timestamps
  $('sampletime').textContent =
    timeText(r.recorded_at);

  $('recorded').textContent =
    timeText(r.recorded_at);

  $('received').textContent =
    r.received_at
      ? timeText(r.received_at)
      : 'Not available from API';

  // Freshness
  const age =
    Date.now() - Date.parse(r.recorded_at);

  if (!Number.isFinite(age)) {
    status('Unknown timestamp', 'stale');
  } else if (age < -FUTURE_TOLERANCE_MS) {
    status('Clock mismatch', 'stale');
  } else if (age > STALE_AFTER_MS) {
    status('Data delayed', 'stale');
  } else {
    status('Connected · Live', 'online');
  }
}

// ============================================================
// Six Charts
// ============================================================

const charts = [
  {
    id: 'chartTemp',
    lines: [
      {
        label: 'Point 1',
        f: r => num(r.temperature_1_c),
        color: '#356d55'
      },
      {
        label: 'Point 2',
        dash: [5, 4],
        f: r => num(r.temperature_2_c),
        color: '#6f8978'
      }
    ]
  },
  {
    id: 'chartDT',
    lines: [
      {
        label: 'T2 − T1',
        f: r => difference(
          r,
          'temperature_1_c',
          'temperature_2_c'
        ),
        color: '#356d55'
      }
    ]
  },
  {
    id: 'chartRH',
    lines: [
      {
        label: 'Point 1',
        f: r => num(r.humidity_1_rh),
        color: '#356d55'
      },
      {
        label: 'Point 2',
        dash: [5, 4],
        f: r => num(r.humidity_2_rh),
        color: '#6f8978'
      }
    ]
  },
  {
    id: 'chartDRH',
    lines: [
      {
        label: 'RH2 − RH1',
        f: r => difference(
          r,
          'humidity_1_rh',
          'humidity_2_rh'
        ),
        color: '#356d55'
      }
    ]
  },
  {
    id: 'chartLux',
    lines: [
      {
        label: 'Lux',
        f: r => num(r.illuminance_lux),
        color: '#6f8978'
      }
    ]
  },
  {
    id: 'chartMotion',
    binary: true,
    lines: [
      {
        label: 'PIR',
        f: r =>
          r.motion_detected === true
            ? 1
            : r.motion_detected === false
              ? 0
              : null,
        color: '#356d55'
      }
    ]
  }
];

// ============================================================
// Chart Drawing
// ============================================================

function draw(cfg) {
  const canvas = $(cfg.id);
  if (!canvas) return;

  const rect = canvas.getBoundingClientRect();

  const w = Math.max(260, rect.width);
  const h = Math.max(180, rect.height);

  const dpr = Math.min(
    window.devicePixelRatio || 1,
    2
  );

  canvas.width = Math.round(w * dpr);
  canvas.height = Math.round(h * dpr);

  const c = canvas.getContext('2d');
  if (!c) return;

  c.setTransform(dpr, 0, 0, dpr, 0, 0);
  c.clearRect(0, 0, w, h);

  const L = 52;
  const R = 14;
  const T = 24;
  const B = 33;

  const pw = w - L - R;
  const ph = h - T - B;

  const all = cfg.lines.map(line => ({
    ...line,
    pts: rows.map(r => ({
      t: Date.parse(r.recorded_at),
      v: line.f(r)
    }))
  }));

  const valid = all.flatMap(line =>
    line.pts.filter(p =>
      Number.isFinite(p.t) &&
      p.v !== null &&
      Number.isFinite(p.v)
    )
  );

  c.font = '11px "Kode Mono", monospace';
  c.fillStyle = '#6c8072';

  if (!valid.length) {
    c.textAlign = 'center';
    c.fillText(
      accessCode ? 'No readings in this window' : 'Connect to view readings',
      w / 2,
      h / 2
    );
    return;
  }

  const tMin = Math.min(
    ...valid.map(p => p.t)
  );

  const tMax = Math.max(
    ...valid.map(p => p.t)
  );

  let lo = cfg.binary
    ? -0.15
    : Math.min(...valid.map(p => p.v));

  let hi = cfg.binary
    ? 1.15
    : Math.max(...valid.map(p => p.v));

  if (!cfg.binary) {
    const pad = Math.max(
      (hi - lo) * 0.12,
      Math.abs(hi) * 0.01,
      0.5
    );

    lo -= pad;
    hi += pad;
  }

  const X = t =>
    L + (
      tMax === tMin
        ? pw / 2
        : (t - tMin) / (tMax - tMin) * pw
    );

  const Y = v =>
    T + (hi - v) / (hi - lo) * ph;

  // Grid and Y-axis
  c.textAlign = 'right';
  c.lineWidth = 1;
  c.strokeStyle = '#e4ebe6';

  for (let j = 0; j <= 4; j++) {
    const y = T + ph * j / 4;

    c.beginPath();
    c.moveTo(L, y);
    c.lineTo(w - R, y);
    c.stroke();

    const tick =
      hi - (hi - lo) * j / 4;

    c.fillStyle = '#6c8072';

    c.fillText(
      tick.toFixed(
        cfg.binary
          ? 1
          : hi - lo > 100
            ? 0
            : 1
      ),
      L - 7,
      y + 4
    );
  }

  // Draw each series independently.
  for (const line of all) {
    c.strokeStyle = line.color;
    c.fillStyle = line.color;
    c.lineWidth = 2.2;
    c.setLineDash(line.dash || []);

    let segment = [];

    const flush = () => {
      if (!segment.length) return;

      if (segment.length > 1) {
        c.beginPath();

        segment.forEach((p, i) => {
          if (i === 0) {
            c.moveTo(X(p.t), Y(p.v));
          } else {
            c.lineTo(X(p.t), Y(p.v));
          }
        });

        c.stroke();
      }

      // Show isolated valid points.
      if (
        segment.length === 1 ||
        rows.length <= 30
      ) {
        for (const p of segment) {
          c.beginPath();
          c.arc(
            X(p.t),
            Y(p.v),
            3,
            0,
            Math.PI * 2
          );
          c.fill();
        }
      }

      segment = [];
    };

    let previousTime = null;
    for (const p of line.pts) {
      if (
        !Number.isFinite(p.t) ||
        p.v === null ||
        !Number.isFinite(p.v)
      ) {
        flush();
        previousTime = null;
        continue;
      }

      // Never imply continuous measurement across a missing sampling period.
      if (previousTime !== null && p.t - previousTime > 90000) flush();
      segment.push(p);
      previousTime = p.t;
    }

    flush();
  }

  c.setLineDash([]);

  // X-axis timestamps
  c.textAlign = 'left';
  c.fillStyle = '#6c8072';

  c.fillText(
    tMax - tMin > 86400000
      ? new Date(tMin).toLocaleDateString([], {month: 'short', day: 'numeric'})
      : new Date(tMin).toLocaleTimeString([], {
      hour: '2-digit',
      minute: '2-digit'
    }),
    L,
    h - 7
  );

  c.textAlign = 'right';

  c.fillText(
    tMax - tMin > 86400000
      ? new Date(tMax).toLocaleDateString([], {month: 'short', day: 'numeric'})
      : new Date(tMax).toLocaleTimeString([], {
      hour: '2-digit',
      minute: '2-digit'
    }),
    w - R,
    h - 7
  );

  // Legend
  let lx = L;
  c.textAlign = 'left';

  for (const line of all) {
    c.fillStyle = line.color;
    c.fillRect(lx, T - 17, 10, 3);

    c.fillStyle = '#617567';
    c.fillText(
      line.label,
      lx + 14,
      T - 11
    );

    lx +=
      c.measureText(line.label).width + 35;
  }
}

function render() {
  $('count').textContent =
    String(rows.length);

  charts.forEach(draw);
  const body = $('recent-rows');
  if (body) {
    body.replaceChildren();
    if (!rows.length) {
      const cell = document.createElement('td');
      cell.colSpan = 8;
      cell.className = 'table-empty';
      cell.textContent = accessCode ? 'No samples found for this sensor.' : 'Connect your sensor to see recent readings here.';
      const tr = document.createElement('tr');
      tr.append(cell);
      body.append(tr);
    } else {
      for (const row of rows.slice(-6).reverse()) {
        const tr = document.createElement('tr');
        const values = [
          timeText(row.recorded_at),
          fmt(row.temperature_1_c), fmt(row.temperature_2_c),
          fmt(difference(row, 'temperature_1_c', 'temperature_2_c'), 2),
          fmt(row.humidity_1_rh), fmt(row.humidity_2_rh),
          fmt(row.illuminance_lux, 0),
          row.motion_detected === true ? 'Detected' : row.motion_detected === false ? 'Clear' : '—'
        ];
        values.forEach((value, index) => {
          const td = document.createElement('td');
          if (index === 3) td.className = 'delta-cell';
          if (index === 7) {
            const pill = document.createElement('span');
            pill.className = 'motion-pill' + (row.motion_detected === true ? ' detected' : '');
            pill.textContent = value;
            td.append(pill);
          } else td.textContent = value;
          tr.append(td);
        });
        body.append(tr);
      }
    }
  }
}

// ============================================================
// Request Lifecycle
// ============================================================

function stopRequests() {
  if (timer !== null) {
    clearInterval(timer);
  }

  timer = null;

  if (activeController) {
    activeController.abort();
  }

  activeController = null;
}

async function loadData(session, key) {
  if (inFlight) return false;

  inFlight = true;
  if (!rows.length) document.body.classList.add('is-loading');
  $('refresh').disabled = true;

  const controller = new AbortController();
  activeController = controller;

  const timeout = setTimeout(
    () => controller.abort(),
    REQUEST_TIMEOUT_MS
  );

  try {
    // Both endpoints must succeed.
    const [recent, latest] = await Promise.all([
      api('recent', key, controller.signal),
      api('latest', key, controller.signal)
    ]);

    if (
      session !== sessionId ||
      key !== accessCode
    ) {
      return false;
    }

    rows = [];

    for (const row of recent.rows) {
      appendRow(row);
    }

    if (latest.latest) {
      appendRow(latest.latest);
      showLatest(latest.latest);
      message('');
    } else {
      showLatest(null);
    }

    render();
    return true;

  } catch (err) {
    if (
      session === sessionId &&
      key === accessCode
    ) {
      status('Connection issue', 'stale');

      message(
        err.name === 'AbortError'
          ? 'Request timed out.'
          : err.message || 'Unable to load data.'
      );
    }

    return false;

  } finally {
    clearTimeout(timeout);

    if (activeController === controller) {
      activeController = null;
    }

    if (session === sessionId) {
      inFlight = false;
      document.body.classList.remove('is-loading');
      $('refresh').disabled = !accessCode || connecting;
    }
  }
}

// ============================================================
// Connect
// ============================================================

async function connect() {
  if (connecting || accessCode) return;

  const code = $('access').value.trim();

  if (!code) {
    message(
      'Enter your Group C access code.'
    );
    return;
  }

  connecting = true;
  accessCode = code;

  const session = ++sessionId;

  $('connect').disabled = true;
  message('Connecting…');

  const success = await loadData(
    session,
    code
  );

  if (session !== sessionId) return;

  connecting = false;

  if (!success) {
    accessCode = '';
    rows = [];

    clearReadings();
    render();

    status('Not connected');
    $('connect').disabled = false;
    $('disconnect').disabled = true;

    return;
  }

  $('disconnect').disabled = false;
  $('refresh').disabled = false;
  $('connect').textContent = 'Connected';
  $('connection-form').classList.add('is-connected');
  document.querySelector('.connection-intro h2').textContent = 'Connected to Group C';
  document.querySelector('.connection-intro p').textContent = 'Your readings update automatically every 30 seconds.';

  $('access').value = '';
  $('access').disabled = true;

  if (timer !== null) {
    clearInterval(timer);
  }

  timer = setInterval(
    refresh,
    REFRESH_MS
  );
}

// ============================================================
// Refresh
// ============================================================

async function refresh() {
  if (
    !accessCode ||
    connecting ||
    inFlight
  ) {
    return;
  }

  await loadData(
    sessionId,
    accessCode
  );
}

// ============================================================
// Disconnect
// ============================================================

function disconnect() {
  sessionId++;

  stopRequests();

  accessCode = '';
  rows = [];
  inFlight = false;
  connecting = false;

  $('access').value = '';
  $('access').disabled = false;

  $('connect').disabled = false;
  $('connect').textContent = 'Connect';
  $('disconnect').disabled = true;
  $('refresh').disabled = true;
  $('connection-form').classList.remove('is-connected');
  document.body.classList.remove('is-loading');
  document.querySelector('.connection-intro h2').textContent = 'Connect to your sensor';
  document.querySelector('.connection-intro p').textContent = 'Use your Group C access code to view readings.';

  clearReadings();
  render();

  status('Not connected');
  message('Disconnected.');
}

// ============================================================
// Event Listeners
// ============================================================

$('connection-form').addEventListener('submit', event => {
  event.preventDefault();
  connect();
});
$('disconnect').addEventListener('click', disconnect);
$('refresh').addEventListener('click', refresh);
document.querySelectorAll('.nav-link').forEach(link => {
  link.addEventListener('click', () => {
    document.querySelectorAll('.nav-link').forEach(item => item.classList.toggle('active', item === link));
  });
});
if (document.fonts) document.fonts.ready.then(render);
$('today').textContent = new Date().toLocaleDateString(undefined, {day:'2-digit',month:'short',year:'numeric'});

window.addEventListener(
  'resize',
  render
);

// Initial dashboard state
clearReadings();
status('Not connected');
render();
