package phonetic

import (
	"encoding/binary"
	"testing"
)

func testIndexData(pronunciations []string) []byte {
	data := make([]byte, headerSize, headerSize+len(pronunciations)*8)
	binary.LittleEndian.PutUint32(data[4:8], uint32(len(pronunciations)))
	for i, pronunciation := range pronunciations {
		data = append(data, pronunciation...)
		data = append(data, '\t')
		data = append(data, "word"...)
		data = append(data, 0)

		value := i
		for {
			data = append(data, byte(value%254+1))
			value /= 254
			if value == 0 {
				break
			}
		}
		data = append(data, 0xff)
	}
	return data
}

func TestParseAndEntryAt(t *testing.T) {
	index, err := Parse(testIndexData([]string{":", "i", "e"}))
	if err != nil {
		t.Fatalf("Parse() error = %v", err)
	}
	if got := index.Count(); got != 3 {
		t.Fatalf("Count() = %d, want 3", got)
	}
	entry, ok := index.EntryAt(2)
	if !ok {
		t.Fatal("EntryAt(2) returned false")
	}
	if entry.Index != 2 || entry.Pronunciation != "i" || entry.Word != "word" {
		t.Fatalf("EntryAt(2) = %+v", entry)
	}
	if _, ok := index.EntryAt(0); ok {
		t.Fatal("EntryAt(0) should be invalid")
	}
}

func TestSearchExactNearestAndSyllableMarks(t *testing.T) {
	index, err := Parse(testIndexData([]string{":", "i'", "i", "e"}))
	if err != nil {
		t.Fatalf("Parse() error = %v", err)
	}

	result, ok := index.Search("i", false)
	if !ok || !result.Exact || result.Index != 2 {
		t.Fatalf("Search(i) = %+v, %v; want first fuzzy match at entry 2", result, ok)
	}

	result, ok = index.Search("i", true)
	if !ok || !result.Exact || result.Index != 3 {
		t.Fatalf("Search(i, include marks) = %+v, %v; want exact entry 3", result, ok)
	}

	result, ok = index.Search("i'", true)
	if !ok || !result.Exact || result.Index != 2 {
		t.Fatalf("Search(i', include marks) = %+v, %v; want exact entry 2", result, ok)
	}

	result, ok = index.Search("d", false)
	if !ok || result.Exact {
		t.Fatalf("Search(d) = %+v, %v; want nearest non-exact entry", result, ok)
	}
}

func TestParseRejectsCorruptRecords(t *testing.T) {
	data := testIndexData([]string{"i"})
	data[4] = 2
	if _, err := Parse(data); err == nil {
		t.Fatal("Parse() accepted a mismatched declared count")
	}

	data = testIndexData([]string{"i"})
	data[len(data)-1] = 0
	if _, err := Parse(data); err == nil {
		t.Fatal("Parse() accepted a missing record terminator")
	}
}
