package httpapi

import (
	"encoding/binary"
	"encoding/json"
	"net/http"
	"net/http/httptest"
	"testing"

	"phoneticengine/phonetic"
)

func newTestHandler(t *testing.T) http.Handler {
	t.Helper()
	data := make([]byte, 8, 64)
	binary.LittleEndian.PutUint32(data[4:8], 2)
	for i, pronunciation := range []string{":", "i"} {
		data = append(data, pronunciation...)
		data = append(data, '\t')
		data = append(data, "testword"...)
		data = append(data, 0, byte(i+1), 0xff)
	}
	index, err := phonetic.Parse(data)
	if err != nil {
		t.Fatalf("Parse() error = %v", err)
	}
	return NewHandler(index, []string{"https://example.test"})
}

func TestHTTPAPI(t *testing.T) {
	handler := newTestHandler(t)
	tests := []struct {
		name       string
		path       string
		wantStatus int
		wantBody   map[string]any
	}{
		{
			name:       "count",
			path:       "/api/phonetic/count",
			wantStatus: http.StatusOK,
			wantBody:   map[string]any{"count": float64(2)},
		},
		{
			name:       "entry",
			path:       "/api/phonetic/entries/2",
			wantStatus: http.StatusOK,
			wantBody: map[string]any{
				"index":         float64(2),
				"pronunciation": "i",
				"word":          "testword",
			},
		},
		{
			name:       "search",
			path:       "/api/phonetic/search?q=i",
			wantStatus: http.StatusOK,
			wantBody: map[string]any{
				"index":         float64(2),
				"pronunciation": "i",
				"word":          "testword",
				"exact":         true,
			},
		},
		{
			name:       "missing query",
			path:       "/api/phonetic/search",
			wantStatus: http.StatusBadRequest,
			wantBody:   map[string]any{"error": "query parameter q is required"},
		},
		{
			name:       "missing entry",
			path:       "/api/phonetic/entries/9",
			wantStatus: http.StatusNotFound,
			wantBody:   map[string]any{"error": "phonetic entry not found"},
		},
	}

	for _, test := range tests {
		t.Run(test.name, func(t *testing.T) {
			request := httptest.NewRequest(http.MethodGet, test.path, nil)
			response := httptest.NewRecorder()
			handler.ServeHTTP(response, request)
			if response.Code != test.wantStatus {
				t.Fatalf("status = %d, want %d; body: %s", response.Code, test.wantStatus, response.Body)
			}
			var body map[string]any
			if err := json.Unmarshal(response.Body.Bytes(), &body); err != nil {
				t.Fatalf("decode response: %v", err)
			}
			for key, want := range test.wantBody {
				if got := body[key]; got != want {
					t.Errorf("response[%q] = %#v, want %#v", key, got, want)
				}
			}
		})
	}
}

func TestCORSAllowsOnlyConfiguredOrigin(t *testing.T) {
	handler := newTestHandler(t)
	request := httptest.NewRequest(http.MethodGet, "/api/phonetic/count", nil)
	request.Header.Set("Origin", "https://example.test")
	response := httptest.NewRecorder()
	handler.ServeHTTP(response, request)
	if got := response.Header().Get("Access-Control-Allow-Origin"); got != "https://example.test" {
		t.Fatalf("allowed origin = %q", got)
	}

	request = httptest.NewRequest(http.MethodGet, "/api/phonetic/count", nil)
	request.Header.Set("Origin", "https://untrusted.test")
	response = httptest.NewRecorder()
	handler.ServeHTTP(response, request)
	if got := response.Header().Get("Access-Control-Allow-Origin"); got != "" {
		t.Fatalf("untrusted origin received CORS permission %q", got)
	}
}
