package word

import (
	"bytes"
	"encoding/binary"
	"errors"
	"fmt"
	"os"
	"strings"
)

const (
	wordIndexHeaderSize    = 8
	alphabetIndexEntrySize = 8
	alphabetIndexCount     = 28
	firstWordOffset        = wordIndexHeaderSize + alphabetIndexEntrySize*alphabetIndexCount // 232
)

type WordSuggestion struct {
	Index   int    `json:"index"`
	Word    string `json:"word"`
	Meaning string `json:"meaning,omitempty"`
}

type Entry struct {
	Index         int    `json:"index"`
	Word          string `json:"word"`
	SimpleMeaning string `json:"simple_meaning,omitempty"`
	DetailMeaning string `json:"detail_meaning,omitempty"`
}

type SearchResult struct {
	Entry
	Exact bool `json:"exact"`
}

type record struct {
	wordStart    int
	wordEnd      int
	simpleOffset int
	detailOffset int
}

type Index struct {
	data        []byte
	records     []record
	definitions *DefinitionStore
}

func LoadFile(path string, definitions *DefinitionStore) (*Index, error) {
	data, err := os.ReadFile(path)
	if err != nil {
		return nil, fmt.Errorf("read word index %s: %w", path, err)
	}
	return Parse(data, definitions)
}

func decodeEncodedInteger(data []byte, offset *int, limit int, terminator byte) (int, bool) {
	var decoded uint64
	var place uint64 = 1
	p := *offset
	for digitCount := 0; digitCount < 4 && p < limit; digitCount++ {
		b := data[p]
		if b == terminator {
			*offset = p
			if terminator == 0xff {
				*offset = p + 1
			}
			return int(decoded), true
		}
		if b == 0 || b == 0xff {
			return 0, false
		}
		decoded += uint64(b-1) * place
		place *= 254
		p++
	}
	return 0, false
}

func Parse(data []byte, definitions *DefinitionStore) (*Index, error) {
	if len(data) < firstWordOffset {
		return nil, errors.New("word index header is incomplete")
	}

	declaredCount := int(binary.LittleEndian.Uint32(data[4:8]))
	if declaredCount <= 0 || declaredCount > len(data)/5 {
		return nil, errors.New("word index entry count is invalid or exceeds file size")
	}

	records := make([]record, 0, declaredCount)
	recordOffset := firstWordOffset
	endMarker := []byte{0, 0}

	for recordOffset < len(data) {
		wordEndRel := bytes.IndexByte(data[recordOffset:], 0)
		if wordEndRel <= 0 {
			return nil, fmt.Errorf("word entry %d has empty or missing NUL terminator", len(records)+1)
		}
		wordEnd := recordOffset + wordEndRel

		recordEndRel := bytes.Index(data[wordEnd+1:], endMarker)
		if recordEndRel < 0 {
			return nil, fmt.Errorf("word entry %d missing double-NUL end marker", len(records)+1)
		}
		recordEnd := wordEnd + 1 + recordEndRel

		metaOffset := wordEnd + 1
		flags := data[metaOffset]
		metaOffset++

		var subIndex int
		var ok bool
		if subIndex, ok = decodeEncodedInteger(data, &metaOffset, recordEnd, 0xff); !ok {
			return nil, fmt.Errorf("word entry %d has invalid sub-index encoding", len(records)+1)
		}
		_ = subIndex

		hasSimple := (flags & 0x01) != 0
		hasDetail := (flags & 0x02) != 0
		defCount := 0
		if hasSimple {
			defCount++
		}
		if hasDetail {
			defCount++
		}

		simpleOffset := -1
		detailOffset := -1
		for i := 0; i < defCount; i++ {
			isLast := (i + 1 == defCount)
			term := byte(0x00)
			if !isLast {
				term = 0xff
			}
			off, ok := decodeEncodedInteger(data, &metaOffset, recordEnd+1, term)
			if !ok {
				return nil, fmt.Errorf("word entry %d has invalid definition offset encoding", len(records)+1)
			}
			if hasSimple && (i == 0 || !hasDetail) {
				simpleOffset = off
			} else {
				detailOffset = off
			}
		}

		records = append(records, record{
			wordStart:    recordOffset,
			wordEnd:      wordEnd,
			simpleOffset: simpleOffset,
			detailOffset: detailOffset,
		})

		recordOffset = recordEnd + 2
	}

	if len(records) != declaredCount {
		return nil, fmt.Errorf("word index entry count mismatch: declared %d, parsed %d", declaredCount, len(records))
	}

	return &Index{
		data:        data,
		records:     records,
		definitions: definitions,
	}, nil
}

func (index *Index) Count() int {
	return len(index.records)
}

func (index *Index) EntryAt(entryNumber int) (Entry, bool) {
	if entryNumber < 1 || entryNumber > len(index.records) {
		return Entry{}, false
	}
	return index.entryAtRow(entryNumber - 1), true
}

func (index *Index) entryAtRow(row int) Entry {
	rec := index.records[row]
	entry := Entry{
		Index: row + 1,
		Word:  string(index.data[rec.wordStart:rec.wordEnd]),
	}
	if index.definitions != nil {
		if rec.simpleOffset >= 0 {
			entry.SimpleMeaning = index.definitions.SimpleMeaning(rec.simpleOffset)
		}
		if rec.detailOffset >= 0 {
			entry.DetailMeaning = index.definitions.DetailMeaning(rec.detailOffset)
		}
	}
	return entry
}

func (index *Index) Search(query string) (SearchResult, bool) {
	if len(index.records) == 0 || query == "" {
		return SearchResult{}, false
	}
	for i := 0; i < len(query); i++ {
		if query[i] > 0x7f {
			return SearchResult{}, false
		}
	}

	queryBytes := []byte(query)
	first, last := 0, len(index.records)
	for first < last {
		middle := first + (last-first)/2
		rec := index.records[middle]
		wordBytes := index.data[rec.wordStart:rec.wordEnd]
		if compareWords(wordBytes, queryBytes) < 0 {
			first = middle + 1
		} else {
			last = middle
		}
	}

	row := first
	if row >= len(index.records) {
		row = len(index.records) - 1
	}

	exact := first < len(index.records) &&
		compareWords(index.data[index.records[first].wordStart:index.records[first].wordEnd], queryBytes) == 0

	return SearchResult{
		Entry: index.entryAtRow(row),
		Exact: exact,
	}, true
}

func (index *Index) Suggest(query string, limit int) []WordSuggestion {
	if len(index.records) == 0 || query == "" || limit <= 0 {
		return nil
	}
	if limit > 50 {
		limit = 50
	}
	for i := 0; i < len(query); i++ {
		if query[i] > 0x7f {
			return nil
		}
	}

	queryBytes := []byte(query)
	first, last := 0, len(index.records)
	for first < last {
		middle := first + (last-first)/2
		rec := index.records[middle]
		wordBytes := index.data[rec.wordStart:rec.wordEnd]
		if compareWords(wordBytes, queryBytes) < 0 {
			first = middle + 1
		} else {
			last = middle
		}
	}

	lowerQuery := strings.ToLower(query)
	results := make([]WordSuggestion, 0, limit)

	// Collect prefix matches starting from first
	row := first
	for row < len(index.records) && len(results) < limit {
		rec := index.records[row]
		word := string(index.data[rec.wordStart:rec.wordEnd])
		if !strings.HasPrefix(strings.ToLower(word), lowerQuery) {
			break
		}
		meaning := ""
		if index.definitions != nil && rec.simpleOffset >= 0 {
			meaning = index.definitions.SimplePreview(rec.simpleOffset)
		}
		results = append(results, WordSuggestion{
			Index:   row + 1,
			Word:    word,
			Meaning: meaning,
		})
		row++
	}

	// If no exact prefix matches found, fall back to nearest candidates
	if len(results) == 0 {
		startRow := first
		if startRow >= len(index.records) {
			startRow = len(index.records) - 1
		}
		for r := startRow; r < len(index.records) && len(results) < limit; r++ {
			rec := index.records[r]
			word := string(index.data[rec.wordStart:rec.wordEnd])
			meaning := ""
			if index.definitions != nil && rec.simpleOffset >= 0 {
				meaning = index.definitions.SimplePreview(rec.simpleOffset)
			}
			results = append(results, WordSuggestion{
				Index:   r + 1,
				Word:    word,
				Meaning: meaning,
			})
		}
	}

	return results
}

func compareWords(left, right []byte) int {
	offset := 0
	for offset < len(left) && offset < len(right) {
		leftChar := left[offset]
		rightChar := right[offset]
		lowerLeft := leftChar
		if lowerLeft >= 'A' && lowerLeft <= 'Z' {
			lowerLeft += 'a' - 'A'
		}
		lowerRight := rightChar
		if lowerRight >= 'A' && lowerRight <= 'Z' {
			lowerRight += 'a' - 'A'
		}

		if lowerLeft == lowerRight {
			offset++
			continue
		}
		if lowerLeft == ' ' && (lowerRight == '-' || lowerRight == '\'') {
			return 1
		}
		if lowerRight == ' ' && (lowerLeft == '-' || lowerLeft == '\'') {
			return -1
		}
		if lowerLeft < lowerRight {
			return -1
		}
		return 1
	}

	var leftChar byte
	if offset < len(left) {
		leftChar = left[offset]
	}
	var rightChar byte
	if offset < len(right) {
		rightChar = right[offset]
	}

	lowerLeft := leftChar
	if lowerLeft >= 'A' && lowerLeft <= 'Z' {
		lowerLeft += 'a' - 'A'
	}
	lowerRight := rightChar
	if lowerRight >= 'A' && lowerRight <= 'Z' {
		lowerRight += 'a' - 'A'
	}

	if lowerLeft == lowerRight {
		return 0
	}
	if lowerLeft == ' ' && (lowerRight == '-' || lowerRight == '\'') {
		return 1
	}
	if lowerRight == ' ' && (lowerLeft == '-' || lowerLeft == '\'') {
		return -1
	}
	if lowerLeft < lowerRight {
		return -1
	}
	return 1
}
