# KegSense

KegSense is an ESP32-based monitoring system for one or two beverage kegs. It
measures keg weight, estimates the remaining volume, monitors kegerator
temperature, records consumption history and provides a responsive local web
dashboard. An optional Cloudflare service makes the current status and history
available remotely through an installable web app.

> **Project status:** KegSense 2.0 is currently an alpha release. Keep a known
> working firmware image before updating a device.

## Features

- One or two independently calibrated keg scales
- Four 3-wire load cells and one HX711 module per scale
- Remaining volume in litres and percent
- DS18B20 kegerator temperature monitoring
- Up to 62 days of local consumption history
- Captive Wi-Fi setup portal with selectable features
- Local dashboard at `http://kegsense.local`
- Browser-based OTA firmware updates
- Optional Cloudflare Worker, D1 database and installable Remote PWA
- Settings, calibration and history stored in ESP32 NVS

## Hardware

- ESP32 DevKit V1, 30-pin
- 2 x HX711 load-cell amplifier modules
- 8 x 3-wire half-bridge load cells
- 1 x DS18B20 temperature module
- Stable USB power supply and a data-capable USB cable
- Two mechanically independent scale platforms

### GPIO map

The current schematic and firmware on `v2-refactor` use the same assignments:

| Function | ESP32 pin | Module pin |
| --- | --- | --- |
| Scale 1 data | GPIO 32 | HX711 DT/DOUT |
| Scale 1 clock | GPIO 33 | HX711 SCK |
| Scale 2 data | GPIO 16 | HX711 DT/DOUT |
| Scale 2 clock | GPIO 17 | HX711 SCK |
| Temperature data | GPIO 13 | DS18B20 DATA |
| Sensor supply | 3V3 | HX711/DS18B20 VCC |
| Common ground | GND | HX711/DS18B20 GND |

Disconnect USB power before changing wiring. Check for a short between 3.3 V
and GND before powering the board.

## Documentation

Complete illustrated guides are available in both languages:

- [English project guide (PDF)](docs/guides/KegSense-complete-project-guide-EN.pdf)
- [Norsk prosjektveiledning (PDF)](docs/guides/KegSense-komplett-veiledning-NO.pdf)

The guides cover mechanical assembly, wiring, flashing, calibration, Wi-Fi,
daily use, OTA updates, Cloudflare Remote and troubleshooting.

## Build and upload

The firmware is built with [PlatformIO](https://platformio.org/) using the
Arduino framework.

```bash
git clone https://github.com/Uthus98/KegSense.git
cd KegSense
git switch v2-refactor
pio run
```

Connect the ESP32, identify its serial port and upload the firmware:

```bash
pio run --target upload --upload-port COM3
pio device monitor --port COM3 --baud 115200
```

Replace `COM3` with the serial port used by your computer.

## First-time setup

1. Power the ESP32 and join the `KegSense-Setup` Wi-Fi network.
2. Wait for the captive portal or open `http://192.168.4.1`.
3. Select a 2.4 GHz Wi-Fi network and enter its password.
4. Choose one or two active kegs.
5. Enable temperature, history and Cloudflare Remote as required.
6. Save the configuration, reconnect to the local network and open
   `http://kegsense.local`.
7. Open **Settings** and calibrate each enabled scale using a known mass.

The dashboard also displays the device IP address, which can be used when mDNS
is not supported by the network.

## Local web interface

| Address | Purpose |
| --- | --- |
| `/` | Dashboard |
| `/settings` | Keg data, calibration and device controls |
| `/wifi` | Wi-Fi, keg count and optional features |
| `/daily-history` | Local consumption history |
| `/update` | OTA firmware update |

The development OTA credentials are `admin` / `kegsense`. Change
`OTA_USERNAME` and `OTA_PASSWORD` in `include/config.h` before permanent or
shared deployment.

## Cloudflare Remote

The optional service in [`cloud/`](cloud/) consists of a Cloudflare Worker, a
D1 database and a PWA. The ESP32 only makes outbound HTTPS requests.

```bash
cd cloud
npm install
npx wrangler login
npx wrangler d1 create kegsense-remote
npx wrangler secret put DEVICE_TOKEN
npx wrangler secret put APP_TOKEN
npm run db:remote
npm run deploy
```

Copy the generated D1 database ID into `cloud/wrangler.jsonc` before creating
the tables. Then open `/wifi` on KegSense and enter:

- A unique remote device ID
- The Worker telemetry URL ending in `/api/telemetry`
- The same `DEVICE_TOKEN` stored as the Worker secret

`APP_TOKEN` is used only by the Remote app. Cloudflare does not reveal secret
values after they have been stored; if a token is lost, create a new one and
update KegSense with the same value.

See [cloud/README.md](cloud/README.md) for the concise deployment checklist.

## OTA updates

Open `http://kegsense.local/update`, authenticate and upload the KegSense
application `.bin` file. Do not remove power while flash is being written. A
failed OTA upload retains the previous firmware; restart the ESP32 and inspect
the serial log before retrying.

## Project structure

```text
KegSense/
|-- include/          Firmware configuration and private-secret example
|-- src/              ESP32 firmware and embedded web interface
|-- cloud/            Cloudflare Worker, D1 schema and Remote PWA
|-- docs/guides/      Norwegian and English project guides
|-- platformio.ini    PlatformIO environment and dependencies
`-- release/          Locally generated release artifacts, when present
```

## Security notes

- Change the default OTA credentials before permanent deployment.
- Never commit real Wi-Fi passwords, `DEVICE_TOKEN` or `APP_TOKEN` values.
- Keep device secrets in `include/remote_secrets.h`; the repository contains
  `include/remote_secrets.example.h` as a template.
- Use separate, long values for `DEVICE_TOKEN` and `APP_TOKEN`.

## Troubleshooting

- **Setup portal does not appear:** restart the ESP32 and look for
  `KegSense-Setup`; open `http://192.168.4.1` manually.
- **`kegsense.local` does not resolve:** use the IP address shown on the
  dashboard or in the router.
- **Scale is offline or unstable:** verify HX711 power, DT/SCK pins, common
  ground, load-cell orientation and mechanical clearance.
- **Temperature is unavailable:** confirm the feature is enabled and check the
  DS18B20 connection on GPIO 13.
- **Remote does not update:** verify the HTTPS URL, device ID, `DEVICE_TOKEN`,
  Worker logs and HTTP response status.

For detailed diagnostic procedures, use the complete project guide in
[`docs/guides`](docs/guides/).
