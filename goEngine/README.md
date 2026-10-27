# Phonetic API

This Go service reads the uncompressed `IPhonetic.dat` binary index and provides
read-only HTTP endpoints for the phonetic entry count, indexed entries, and
phonetic search. The binary file is parsed as bytes; the pronunciation search
uses the same legacy ordering and marker handling as the Qt
`PhoneticSearchEngine`.

## Run

From this directory:

```powershell
go test ./...
go run . -data ..\bin\go\IPhonetic.dat -addr :8080
```

The data path can also be set with `PHONETIC_DATA`, and the listen address with
`PHONETIC_ADDR`. `PHONETIC_ALLOWED_ORIGINS` is a comma-separated list of exact
web origins allowed by CORS. If it is not set, cross-origin browser access is
not enabled.

## Endpoints

- `GET /api/phonetic/count` returns `{"count":144026}`.
- `GET /api/phonetic/entries/{index}` returns one entry. `index` is 1-based.
- `GET /api/phonetic/search?q={phonetic}` searches using legacy phonetic sort
  rules and returns the nearest entry with an `exact` flag.
- Add `includeSyllableMarks=true` to the search query to request strict
  syllable-marker matching.

An entry response includes `index`, `pronunciation`, and `word`. Search returns
the same fields plus `exact`. Search input must use ASCII phonetic-keyboard
characters; Unicode IPA conversion is not performed by this service.

The `word` value is the raw field after the first tab and before the record's
NUL terminator. It may contain additional tabs and should not be assumed to be
a single plain word token.
