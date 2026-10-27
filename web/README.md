# Okeyen Dictionary Web

The first web client for the Go phonetic dictionary service. The UI follows the
workspace, navigation, and responsive-card patterns used by the Figma-generated
`devkit` project while keeping this application independent and ready for more
dictionary sections.

## Start locally

1. Start the Go service from `goEngine`:

   ```powershell
   go run . -data ..\bin\go\IPhonetic.dat -addr :8080
   ```

2. From this `web` directory, install dependencies and start Vite:

   ```powershell
   npm install
   npm run dev
   ```

Vite proxies `/api` requests to `http://127.0.0.1:8080`. To use a different Go
server address, set `VITE_API_PROXY_TARGET` in a local `.env` file (see
`.env.example`).

## Implemented

- Reads the phonetic entry count from the Go API.
- Searches phonetic entries and displays exact versus nearest matches.
- Displays the raw word field and supports previous/next index navigation.
- Includes an on-screen phonetic keyboard and maps physical keys using the
  legacy Qt `PhoneticLineEdit` layout.
- Shows loading and API error states.

The dictionary's word field is displayed unchanged because it may contain tabs.
The `Okeyen Phonetic` font is referenced by the UI but not bundled; install a
licensed copy locally or provide an appropriately licensed web font before
deploying if the legacy glyphs are required.
