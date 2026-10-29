# Dictionary API (Phonetic & Word)

This Go service reads uncompressed binary dictionaries (`IPhonetic.dat`, `iword.dat`, `data1.dat`, `data2.dat`) and provides
read-only HTTP endpoints for:
1. Phonetic lookup, indexing, and search with legacy sorting rules.
2. English word search (493,062 entries) with definitions (simple meaning from `data1.dat` and detailed meaning from `data2.dat`), decoded to UTF-8.

## Configuration

The service can be configured via `config.json` (placed in the working directory or specified via `-config`):

```json
{
  "server": {
    "addr": ":8080",
    "allowed_origins": [
      "http://localhost:5173",
      "http://localhost:3000"
    ]
  },
  "data": {
    "phonetic_path": "../bin/go/IPhonetic.dat",
    "word_index_path": "../bin/go/iword.dat",
    "simple_meaning_path": "../bin/go/data1.dat",
    "detail_meaning_path": "../bin/go/data2.dat"
  }
}
```

Command-line flags `-config`, `-addr`, and `-data` can override these options.

## Run

From this directory:

```powershell
go test ./...
go run . -config config.json
```

## Endpoints

### Phonetic Endpoints
- `GET /api/phonetic/count` returns `{"count":144026}`.
- `GET /api/phonetic/entries/{index}` returns one entry (1-based index).
- `GET /api/phonetic/search?q={phonetic}` searches using legacy phonetic sort rules and returns the nearest entry with an `exact` flag.
  - Add `includeSyllableMarks=true` for strict syllable-marker matching.

### Word Endpoints
- `GET /api/word/count` returns `{"count":493062}`.
- `GET /api/word/entries/{index}` returns one word entry with simple and detail definitions (1-based index).
- `GET /api/word/search?q={word}` searches using legacy word dictionary sort rules and returns the nearest entry with `exact`, `simple_meaning`, and `detail_meaning`.
