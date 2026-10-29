package word_test

import (
	"path/filepath"
	"strings"
	"testing"

	"phoneticengine/word"
)

func TestWordIndexAndDefinitions(t *testing.T) {
	wordPath := filepath.Join("..", "..", "bin", "go", "iword.dat")
	simplePath := filepath.Join("..", "..", "bin", "go", "data1.dat")
	detailPath := filepath.Join("..", "..", "bin", "go", "data2.dat")

	defs, err := word.LoadDefinitions(simplePath, detailPath)
	if err != nil {
		t.Fatalf("load definitions: %v", err)
	}

	idx, err := word.LoadFile(wordPath, defs)
	if err != nil {
		t.Fatalf("load word index: %v", err)
	}

	const expectedCount = 493062
	if idx.Count() != expectedCount {
		t.Fatalf("expected count %d, got %d", expectedCount, idx.Count())
	}

	// Test Search "apple"
	result, ok := idx.Search("apple")
	if !ok {
		t.Fatal("expected search apple to succeed")
	}
	if !result.Exact {
		t.Fatal("expected search apple to be exact match")
	}
	if !strings.EqualFold(result.Word, "apple") {
		t.Fatalf("expected word apple, got %s", result.Word)
	}
	if result.SimpleMeaning == "" {
		t.Fatal("expected apple to have simple meaning")
	}
	if !strings.Contains(result.SimpleMeaning, "苹果") {
		t.Fatalf("expected simple meaning to contain '苹果', got: %s", result.SimpleMeaning)
	}
	if result.DetailMeaning == "" {
		t.Fatal("expected apple to have detail meaning")
	}
	if !strings.Contains(result.DetailMeaning, "苹果") {
		t.Fatalf("expected detail meaning to contain '苹果', got: %s", result.DetailMeaning)
	}

	// Test case-insensitive search "Apple"
	resultCase, ok := idx.Search("Apple")
	if !ok || !resultCase.Exact || resultCase.Index != result.Index {
		t.Fatalf("expected search Apple to match same index as apple, got %+v", resultCase)
	}

	// Test approximate search
	resultApprox, ok := idx.Search("zzzzzzzz")
	if !ok {
		t.Fatal("expected approximate search to succeed")
	}
	if resultApprox.Exact {
		t.Fatal("expected zzzzzzzz to not be exact match")
	}
	if resultApprox.Index != expectedCount {
		t.Fatalf("expected approximate search to land at last word %d, got %d", expectedCount, resultApprox.Index)
	}

	// Test EntryAt bounds
	first, ok := idx.EntryAt(1)
	if !ok || first.Index != 1 || first.Word == "" {
		t.Fatalf("failed to fetch first entry: %+v", first)
	}
	last, ok := idx.EntryAt(expectedCount)
	if !ok || last.Index != expectedCount || last.Word == "" {
		t.Fatalf("failed to fetch last entry: %+v", last)
	}
	if _, ok := idx.EntryAt(0); ok {
		t.Fatal("entry 0 should be invalid")
	}
	if _, ok := idx.EntryAt(expectedCount + 1); ok {
		t.Fatal("entry expectedCount+1 should be invalid")
	}
}
