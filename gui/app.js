// ═══ SmartServoStepperV2 – Dashboard JS ═══
let ws = null;
let isKilled = true;
let controlMode = 'pos';
let lastRps = 0;
let isCalibrating = false;
let microstepVal = 4;
const sprMap = [200, 400, 800, 1600, 3200, 6400];
let stepsPerRev = sprMap[microstepVal];
const CIRC = 326.7; // 2*PI*52 for gauges
const CAL_CIRC = 263.9; // 2*PI*42

function connectWS() {
    const ip = document.getElementById('ip-input').value.trim();
    if (ws) { ws.onclose = null; ws.close(); }
    ws = new WebSocket('ws://' + ip + ':81');
    ws.onopen = () => {
        document.getElementById('conn-dot').classList.add('active');
        document.getElementById('conn-text').textContent = 'Verbunden (' + ip + ')';
        readMotorId();
    };
    ws.onclose = () => {
        document.getElementById('conn-dot').classList.remove('active');
        document.getElementById('conn-text').textContent = 'Getrennt – Reconnect...';
        setTimeout(connectWS, 3000);
    };
    ws.onerror = () => {};
    ws.onmessage = (evt) => {
        try {
            const d = JSON.parse(evt.data);
            // Encoder angle
            if (d.pos !== undefined) {
                const deg = (d.pos / 4096 * 360).toFixed(0);
                document.getElementById('val-angle').textContent = deg;
                document.getElementById('val-raw').textContent = d.pos;
                setGaugeRing('gauge-pos-ring', d.pos / 4096);
                setGaugeRing('gauge-raw-ring', d.pos / 4096);
            }
            // RPS
            if (d.rps !== undefined) {
                lastRps = parseFloat(d.rps);
                document.getElementById('val-rps').textContent = lastRps.toFixed(2);
                setGaugeRing('gauge-rps-ring', Math.min(Math.abs(lastRps) / 5, 1));
            }
            // PID
            if (d.pid !== undefined) document.getElementById('val-pid').textContent = d.pid;
            if (d.sp !== undefined) document.getElementById('val-setpoint').textContent = d.sp;
            if (d.err !== undefined) document.getElementById('val-error').textContent = d.err;
            // TMC status
            if (d.tmc !== undefined) {
                const el = document.getElementById('val-tmc');
                el.textContent = d.tmc; el.className = 'tele-value ' + (d.tmc === 'OK' ? 'tele-ok' : 'tele-err');
            }
            // Kill state
            if (d.k === 1 && !isKilled) { isKilled = true; updatePowerUI(); }
            else if (d.k === 0 && isKilled) { isKilled = false; updatePowerUI(); }
            // TTL
            if (d.ttl_id !== undefined) { document.getElementById('ttl-id').textContent = d.ttl_id; setTtl('ok', 'Bus-ID: ' + d.ttl_id); }
            if (d.ttl_ack !== undefined) { document.getElementById('ttl-id').textContent = d.ttl_ack; setTtl('ok', 'ID ' + d.ttl_ack + ' gespeichert ✓'); }
        } catch(e) {}
    };
}

function setGaugeRing(id, frac) {
    const el = document.getElementById(id);
    if (el) el.style.strokeDashoffset = CIRC * (1 - Math.max(0, Math.min(1, frac)));
}

function send(obj) { if (ws && ws.readyState === WebSocket.OPEN) ws.send(JSON.stringify(obj)); }

// ── Mode ──
function setMode(m) {
    controlMode = m;
    document.getElementById('btn-mode-pos').classList.toggle('active', m === 'pos');
    document.getElementById('btn-mode-speed').classList.toggle('active', m === 'speed');
    document.getElementById('ctrl-pos').style.display = m === 'pos' ? 'block' : 'none';
    document.getElementById('ctrl-speed').style.display = m === 'speed' ? 'block' : 'none';
    if (m === 'speed') { document.getElementById('slider-speed').value = 0; document.getElementById('lbl-spd').textContent = '0'; send({mode:'speed', val:0}); }
    else { send({mode:'pos', val: parseInt(document.getElementById('slider-pos').value)}); }
}

function updatePos(v) { document.getElementById('lbl-pos').textContent = v; send({mode:'pos', val: parseInt(v)}); }
function updateSpeed(v) { document.getElementById('lbl-spd').textContent = v; send({mode:'speed', val: parseInt(v)}); }
function snapZero() { document.getElementById('slider-speed').value = 0; updateSpeed(0); }
function quickPos(v) { document.getElementById('slider-pos').value = v; updatePos(v); }
function setMicrostep(v) { microstepVal = parseInt(v); stepsPerRev = sprMap[microstepVal]; send({cmd:'set_microstep', value: microstepVal}); }

// ── PID ──
function sendPID() { send({cmd:'set_pid', kp: parseFloat(document.getElementById('pid-kp').value), ki: parseFloat(document.getElementById('pid-ki').value), kd: parseFloat(document.getElementById('pid-kd').value)}); }
function setPIDPreset(kp, ki, kd) { document.getElementById('pid-kp').value = kp; document.getElementById('pid-ki').value = ki; document.getElementById('pid-kd').value = kd; sendPID(); }

// ── Power ──
function togglePower() { if (isKilled) { send({cmd:'enable'}); isKilled = false; } else { send({cmd:'kill'}); isKilled = true; } updatePowerUI(); }
function updatePowerUI() {
    const btn = document.getElementById('btn-power');
    const lbl = document.getElementById('power-label');
    if (isKilled) { btn.className = 'power-button power-off'; lbl.textContent = 'MOTOR AKTIVIEREN'; }
    else { btn.className = 'power-button power-on'; lbl.textContent = 'NOT-AUS (MOTOR AKTIV)'; }
}

// ── TTL ──
function setMotorId() {
    const id = parseInt(document.getElementById('ttl-new-id').value);
    if (isNaN(id) || id < 1 || id > 253) { setTtl('error', 'Ungültige ID (1–253)'); return; }
    setTtl('busy', 'Setze ID ' + id + '...'); send({cmd:'set_ttl_id', value: id});
}
function readMotorId() { setTtl('busy', 'Lese...'); send({cmd:'get_ttl_id'}); }
function setTtl(state, text) { document.getElementById('ttl-dot').className = 'ttl-dot ' + state; document.getElementById('ttl-status').textContent = text; }

// ── Auto-Sense ──
async function runAutoSense() {
    if (isCalibrating || isKilled) { alert('Motor erst aktivieren!'); return; }
    isCalibrating = true;
    const btn = document.getElementById('btn-cal');
    const pct = document.getElementById('cal-pct');
    const status = document.getElementById('cal-status');
    const result = document.getElementById('cal-result');
    btn.disabled = true; btn.textContent = 'SCANNEN...'; result.style.display = 'none';
    const freqs = [1000, 2000, 3000, 5000, 7000, 9000, 11000, 13000, 15000];
    let maxHz = 0;
    send({cmd:'tare'}); await sleep(1000);
    for (let i = 0; i < freqs.length; i++) {
        const hz = freqs[i], p = Math.round((i / freqs.length) * 100);
        pct.textContent = p + '%';
        document.getElementById('cal-ring').style.strokeDashoffset = CAL_CIRC * (1 - p / 100);
        status.innerHTML = 'Teste: <b>' + hz + ' Hz</b>...';
        send({mode:'speed', val: hz}); await sleep(1500);
        const expected = hz / stepsPerRev, actual = Math.abs(lastRps);
        if (actual < expected * 0.75) { status.innerHTML = '<span style="color:var(--danger)">LIMIT bei ' + hz + ' Hz!</span>'; break; }
        maxHz = hz;
    }
    send({mode:'speed', val: 0}); isCalibrating = false;
    btn.disabled = false; btn.textContent = 'RE-SCAN'; pct.textContent = '100%';
    document.getElementById('cal-ring').style.strokeDashoffset = 0;
    if (maxHz > 0) { result.style.display = 'block'; document.getElementById('res-hz').textContent = maxHz; }
}
function sleep(ms) { return new Promise(r => setTimeout(r, ms)); }

window.addEventListener('load', connectWS);
