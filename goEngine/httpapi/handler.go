package httpapi

import (
	"encoding/json"
	"net/http"
	"strconv"
	"strings"

	"phoneticengine/phonetic"
)

type Handler struct {
	index          *phonetic.Index
	allowedOrigins map[string]struct{}
}

func NewHandler(index *phonetic.Index, allowedOrigins []string) *Handler {
	origins := make(map[string]struct{}, len(allowedOrigins))
	for _, origin := range allowedOrigins {
		origin = strings.TrimSpace(origin)
		if origin != "" {
			origins[origin] = struct{}{}
		}
	}
	return &Handler{index: index, allowedOrigins: origins}
}

func (handler *Handler) ServeHTTP(writer http.ResponseWriter, request *http.Request) {
	handler.setCORS(writer, request)
	if request.Method == http.MethodOptions {
		writer.WriteHeader(http.StatusNoContent)
		return
	}
	if request.Method != http.MethodGet {
		writer.Header().Set("Allow", "GET, OPTIONS")
		writeError(writer, http.StatusMethodNotAllowed, "method not allowed")
		return
	}

	switch {
	case request.URL.Path == "/api/phonetic/count":
		writeJSON(writer, http.StatusOK, struct {
			Count int `json:"count"`
		}{Count: handler.index.Count()})
	case request.URL.Path == "/api/phonetic/search":
		handler.search(writer, request)
	case strings.HasPrefix(request.URL.Path, "/api/phonetic/entries/"):
		handler.entry(writer, request.URL.Path)
	default:
		writeError(writer, http.StatusNotFound, "endpoint not found")
	}
}

func (handler *Handler) search(writer http.ResponseWriter, request *http.Request) {
	query := request.URL.Query().Get("q")
	if query == "" {
		writeError(writer, http.StatusBadRequest, "query parameter q is required")
		return
	}
	if len(query) > 256 {
		writeError(writer, http.StatusBadRequest, "query parameter q is too long")
		return
	}
	for i := 0; i < len(query); i++ {
		if query[i] > 0x7f {
			writeError(writer, http.StatusBadRequest, "query must use ASCII phonetic-keyboard characters")
			return
		}
	}

	includeSyllableMarks := false
	if values, exists := request.URL.Query()["includeSyllableMarks"]; exists {
		if len(values) != 1 {
			writeError(writer, http.StatusBadRequest, "includeSyllableMarks must be specified once")
			return
		}
		raw := values[0]
		parsed, err := strconv.ParseBool(raw)
		if err != nil {
			writeError(writer, http.StatusBadRequest, "includeSyllableMarks must be true or false")
			return
		}
		includeSyllableMarks = parsed
	}

	result, ok := handler.index.Search(query, includeSyllableMarks)
	if !ok {
		writeError(writer, http.StatusNotFound, "no phonetic entries are available")
		return
	}
	writeJSON(writer, http.StatusOK, result)
}

func (handler *Handler) entry(writer http.ResponseWriter, path string) {
	rawIndex := strings.TrimPrefix(path, "/api/phonetic/entries/")
	if rawIndex == "" || strings.Contains(rawIndex, "/") {
		writeError(writer, http.StatusBadRequest, "entry index must be a positive integer")
		return
	}
	entryNumber, err := strconv.Atoi(rawIndex)
	if err != nil || entryNumber < 1 {
		writeError(writer, http.StatusBadRequest, "entry index must be a positive integer")
		return
	}
	entry, ok := handler.index.EntryAt(entryNumber)
	if !ok {
		writeError(writer, http.StatusNotFound, "phonetic entry not found")
		return
	}
	writeJSON(writer, http.StatusOK, entry)
}

func (handler *Handler) setCORS(writer http.ResponseWriter, request *http.Request) {
	origin := request.Header.Get("Origin")
	if origin == "" {
		return
	}
	writer.Header().Add("Vary", "Origin")
	if _, ok := handler.allowedOrigins[origin]; !ok {
		return
	}
	writer.Header().Set("Access-Control-Allow-Origin", origin)
	writer.Header().Set("Access-Control-Allow-Methods", "GET, OPTIONS")
	writer.Header().Set("Access-Control-Allow-Headers", "Content-Type")
}

func writeJSON(writer http.ResponseWriter, status int, value any) {
	writer.Header().Set("Content-Type", "application/json; charset=utf-8")
	writer.WriteHeader(status)
	_ = json.NewEncoder(writer).Encode(value)
}

func writeError(writer http.ResponseWriter, status int, message string) {
	writeJSON(writer, status, struct {
		Error string `json:"error"`
	}{Error: message})
}
