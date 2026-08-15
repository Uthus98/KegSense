# KegSense Remote

Cloudflare Worker + D1 + installerbar PWA. ESP32 sender kun utgående HTTPS-data.

## Oppsett

1. Installer Node.js og kjør `npm install` i denne mappen.
2. Kjør `npx wrangler login`.
3. Opprett databasen: `npx wrangler d1 create kegsense-remote`.
4. Lim database-ID-en inn i `wrangler.jsonc`.
5. Opprett to lange, forskjellige nøkler:
   - `npx wrangler secret put DEVICE_TOKEN`
   - `npx wrangler secret put APP_TOKEN`
6. Opprett tabellene: `npm run db:remote`.
7. Publiser appen: `npm run deploy`.
8. Lim Worker-adressen og `DEVICE_TOKEN` inn i `include/config.h`, og sett `REMOTE_ENABLED = true`.
9. Bygg og installer ny OTA-firmware.

`APP_TOKEN` brukes bare i mobilappen. `DEVICE_TOKEN` skal bare ligge på ESP32 og som Worker-secret.
