const json = (data, status = 200) => new Response(JSON.stringify(data), {
  status,
  headers: {
    "content-type": "application/json; charset=utf-8",
    "cache-control": "no-store",
    "x-content-type-options": "nosniff"
  }
});

function bearer(request) {
  const value = request.headers.get("authorization") || "";
  return value.startsWith("Bearer ") ? value.slice(7) : "";
}

function norwegianDate() {
  const parts = new Intl.DateTimeFormat("en-CA", {
    timeZone: "Europe/Oslo",
    year: "numeric",
    month: "2-digit",
    day: "2-digit"
  }).formatToParts(new Date());
  const get = type => parts.find(part => part.type === type)?.value;
  return `${get("year")}-${get("month")}-${get("day")}`;
}

async function saveTelemetry(request, env) {
  if (!env.DEVICE_TOKEN || bearer(request) !== env.DEVICE_TOKEN)
    return json({ error: "Ikke autorisert" }, 401);

  let payload;
  try {
    payload = await request.json();
  } catch {
    return json({ error: "Ugyldig JSON" }, 400);
  }

  if (!payload || typeof payload.deviceId !== "string" || !Array.isArray(payload.kegs))
    return json({ error: "Mangler deviceId eller kegs" }, 400);

  const deviceId = payload.deviceId.slice(0, 64);
  const serialized = JSON.stringify(payload);
  if (serialized.length > 24000)
    return json({ error: "Datapakken er for stor" }, 413);

  const statements = [env.DB.prepare(`
    INSERT INTO device_status (device_id, payload, updated_at)
    VALUES (?, ?, CURRENT_TIMESTAMP)
    ON CONFLICT(device_id) DO UPDATE SET
      payload = excluded.payload,
      updated_at = CURRENT_TIMESTAMP
  `).bind(deviceId, serialized)];

  const date = norwegianDate();
  for (const keg of payload.kegs) {
    const index = Number(keg.index);
    const liters = Math.max(0, Number(keg.consumptionToday) || 0);
    if (!Number.isInteger(index) || index < 0 || index > 15) continue;

    statements.push(env.DB.prepare(`
      INSERT INTO daily_consumption (device_id, keg_index, date, liters)
      VALUES (?, ?, ?, ?)
      ON CONFLICT(device_id, keg_index, date) DO UPDATE SET
        liters = MAX(daily_consumption.liters, excluded.liters)
    `).bind(deviceId, index, date, liters));
  }

  await env.DB.batch(statements);
  return json({ ok: true, date });
}

async function getStatus(request, env) {
  if (!env.APP_TOKEN || bearer(request) !== env.APP_TOKEN)
    return json({ error: "Feil app-nøkkel" }, 401);

  const row = await env.DB.prepare(`
    SELECT device_id, payload, updated_at
    FROM device_status ORDER BY updated_at DESC LIMIT 1
  `).first();

  if (!row) return json({ error: "Ingen data mottatt ennå" }, 404);
  return json({ ...JSON.parse(row.payload), cloudUpdatedAt: row.updated_at });
}

async function getHistory(request, env) {
  if (!env.APP_TOKEN || bearer(request) !== env.APP_TOKEN)
    return json({ error: "Feil app-nøkkel" }, 401);

  const url = new URL(request.url);
  const days = Math.min(62, Math.max(1, Number(url.searchParams.get("days")) || 30));
  const result = await env.DB.prepare(`
    SELECT device_id, keg_index, date, liters
    FROM daily_consumption
    WHERE date >= date('now', ?)
    ORDER BY date ASC, keg_index ASC
  `).bind(`-${days - 1} days`).all();

  return json({ days, records: result.results || [] });
}

export default {
  async fetch(request, env) {
    const url = new URL(request.url);

    if (url.pathname === "/api/telemetry" && request.method === "POST")
      return saveTelemetry(request, env);
    if (url.pathname === "/api/status" && request.method === "GET")
      return getStatus(request, env);
    if (url.pathname === "/api/history" && request.method === "GET")
      return getHistory(request, env);
    if (url.pathname.startsWith("/api/"))
      return json({ error: "Ikke funnet" }, 404);

    return env.ASSETS.fetch(request);
  }
};
