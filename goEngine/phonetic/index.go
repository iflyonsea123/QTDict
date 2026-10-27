package phonetic

import (
	"bytes"
	"encoding/binary"
	"errors"
	"fmt"
	"os"
)

const (
	headerSize = 8
	maxVarint  = 4
)

var sortGroups = []string{
	"`5'",
	"79",
	":",
	"iI",
	"e",
	"A",
	"Ba",
	"CRo",
	"UuJ?",
	"EL\\",
	"Q",
	"Z",
	"p",
	"b",
	"t",
	"d",
	"k",
	"g^",
	"m",
	"n",
	"l",
	"f",
	"v",
	"WP",
	"T",
	"s",
	"z",
	"F",
	"h",
	"V",
	"r",
	"w",
	"j",
	"N",
}

type Entry struct {
	Index         int    `json:"index"`
	Pronunciation string `json:"pronunciation"`
	Word          string `json:"word"`
}

type SearchResult struct {
	Entry
	Exact bool `json:"exact"`
}

type record struct {
	pronunciationStart int
	pronunciationEnd   int
	wordStart          int
	wordEnd            int
}

type Index struct {
	data    []byte
	records []record
}

func LoadFile(path string) (*Index, error) {
	data, err := os.ReadFile(path)
	if err != nil {
		return nil, fmt.Errorf("read phonetic index: %w", err)
	}
	index, err := Parse(data)
	if err != nil {
		return nil, err
	}
	return index, nil
}

func Parse(data []byte) (*Index, error) {
	if len(data) < headerSize {
		return nil, errors.New("phonetic index header is incomplete")
	}
	data = bytes.Clone(data)

	declaredCount := uint64(binary.LittleEndian.Uint32(data[4:8]))
	if declaredCount > uint64((len(data)-headerSize)/5) {
		return nil, errors.New("phonetic index entry count exceeds file size")
	}

	records := make([]record, 0, int(declaredCount))
	offset := headerSize
	for offset < len(data) {
		wordEndRelative := bytes.IndexByte(data[offset:], 0)
		if wordEndRelative < 0 {
			return nil, fmt.Errorf("record %d has no NUL terminator", len(records)+1)
		}
		wordEnd := offset + wordEndRelative
		tabRelative := bytes.IndexByte(data[offset:wordEnd], '\t')
		if tabRelative < 0 {
			return nil, fmt.Errorf("record %d has an invalid pronunciation separator", len(records)+1)
		}
		tab := offset + tabRelative
		if tab <= offset || tab >= wordEnd {
			return nil, fmt.Errorf("record %d has an invalid pronunciation separator", len(records)+1)
		}

		encodedOffset := wordEnd + 1
		var decodedIndex uint64
		var placeValue uint64 = 1
		encodedBytes := 0
		for encodedOffset < len(data) && data[encodedOffset] != 0xff && encodedBytes < maxVarint {
			digit := data[encodedOffset]
			if digit == 0 {
				return nil, fmt.Errorf("record %d has an invalid entry number", len(records)+1)
			}
			decodedIndex += uint64(digit-1) * placeValue
			placeValue *= 254
			encodedOffset++
			encodedBytes++
		}
		if encodedBytes == 0 || encodedOffset >= len(data) || data[encodedOffset] != 0xff {
			return nil, fmt.Errorf("record %d has an invalid entry number encoding", len(records)+1)
		}
		if decodedIndex != uint64(len(records)) {
			return nil, fmt.Errorf("record %d has a nonsequential entry number", len(records)+1)
		}

		records = append(records, record{
			pronunciationStart: offset,
			pronunciationEnd:   tab,
			wordStart:          tab + 1,
			wordEnd:            wordEnd,
		})
		offset = encodedOffset + 1
	}

	if uint64(len(records)) != declaredCount {
		return nil, fmt.Errorf("phonetic index entry count mismatch: header declares %d, found %d",
			declaredCount, len(records))
	}

	return &Index{data: data, records: records}, nil
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
	record := index.records[row]
	return Entry{
		Index:         row + 1,
		Pronunciation: string(index.data[record.pronunciationStart:record.pronunciationEnd]),
		Word:          string(index.data[record.wordStart:record.wordEnd]),
	}
}

func (index *Index) Search(query string, includeSyllableMarks bool) (SearchResult, bool) {
	if len(index.records) == 0 || query == "" {
		return SearchResult{}, false
	}
	queryBytes := []byte(query)

	first, last := 0, len(index.records)
	for first < last {
		middle := first + (last-first)/2
		record := index.records[middle]
		pronunciation := index.data[record.pronunciationStart:record.pronunciationEnd]
		if comparePronunciations(queryBytes, pronunciation, false) > 0 {
			first = middle + 1
		} else {
			last = middle
		}
	}

	hasFuzzyMatch := first < len(index.records) &&
		comparePronunciations(
			queryBytes,
			index.data[index.records[first].pronunciationStart:index.records[first].pronunciationEnd],
			false,
		) == 0
	if hasFuzzyMatch {
		if includeSyllableMarks {
			for row := first; row < len(index.records); row++ {
				record := index.records[row]
				pronunciation := index.data[record.pronunciationStart:record.pronunciationEnd]
				if comparePronunciations(queryBytes, pronunciation, false) != 0 {
					break
				}
				if comparePronunciations(queryBytes, pronunciation, true) == 0 {
					return SearchResult{Entry: index.entryAtRow(row), Exact: true}, true
				}
			}
		} else {
			return SearchResult{Entry: index.entryAtRow(first), Exact: true}, true
		}
	}

	if first >= len(index.records) {
		first = len(index.records) - 1
	}
	return SearchResult{Entry: index.entryAtRow(first), Exact: false}, true
}

func comparePronunciations(left, right []byte, includeSyllableMarks bool) int {
	leftOffset, rightOffset := 0, 0
	for {
		leftOffset = skipMarkers(left, leftOffset, includeSyllableMarks)
		rightOffset = skipMarkers(right, rightOffset, includeSyllableMarks)

		leftEnd, rightEnd := leftOffset >= len(left), rightOffset >= len(right)
		if leftEnd || rightEnd {
			if leftEnd && rightEnd {
				return 0
			}
			if includeSyllableMarks {
				if leftEnd {
					return 1
				}
				return -1
			}
			if leftEnd {
				return -1
			}
			return 1
		}

		leftChar, rightChar := left[leftOffset], right[rightOffset]
		leftOffset++
		rightOffset++
		if leftChar == rightChar {
			continue
		}

		leftRank := phoneticRank(leftChar, includeSyllableMarks)
		rightRank := phoneticRank(rightChar, includeSyllableMarks)
		if leftRank >= 0 && rightRank >= 0 {
			if leftRank == rightRank {
				continue
			}
			if leftRank < rightRank {
				return -1
			}
			return 1
		}
		if leftRank >= 0 || rightRank >= 0 {
			if leftRank >= 0 {
				return -1
			}
			return 1
		}
		if leftChar < rightChar {
			return -1
		}
		return 1
	}
}

func skipMarkers(value []byte, offset int, includeSyllableMarks bool) int {
	for offset < len(value) {
		if value[offset] == '(' {
			depth := 1
			offset++
			for offset < len(value) && depth > 0 {
				switch value[offset] {
				case '(':
					depth++
				case ')':
					depth--
				}
				offset++
			}
			continue
		}
		char := value[offset]
		if char == '-' || char == '.' || char == ' ' {
			offset++
			continue
		}
		if !includeSyllableMarks &&
			(char == '5' || char == '7' || char == '`' || char == '\'' || char == '9') {
			offset++
			continue
		}
		break
	}
	return offset
}

func phoneticRank(value byte, includeSyllableMarks bool) int {
	firstGroup := 2
	if includeSyllableMarks {
		firstGroup = 0
	}
	for group := firstGroup; group < len(sortGroups); group++ {
		for i := 0; i < len(sortGroups[group]); i++ {
			if sortGroups[group][i] == value {
				return group - firstGroup
			}
		}
	}
	return -1
}
