CREATE TABLE IF NOT EXISTS device_status (
  device_id TEXT PRIMARY KEY,
  payload TEXT NOT NULL,
  updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS daily_consumption (
  device_id TEXT NOT NULL,
  keg_index INTEGER NOT NULL,
  date TEXT NOT NULL,
  liters REAL NOT NULL DEFAULT 0,
  PRIMARY KEY (device_id, keg_index, date)
);

CREATE INDEX IF NOT EXISTS idx_daily_device_date
  ON daily_consumption(device_id, date DESC);
