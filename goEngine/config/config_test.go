package config_test

import (
	"os"
	"path/filepath"
	"testing"

	"phoneticengine/config"
)

func TestLoadDefaultAndEnv(t *testing.T) {
	cfg, err := config.Load("")
	if err != nil {
		t.Fatalf("Load default config failed: %v", err)
	}

	if cfg.Server.Addr != ":8080" {
		t.Errorf("expected default addr :8080, got %s", cfg.Server.Addr)
	}

	// Test env override
	os.Setenv("PHONETIC_ADDR", ":9999")
	defer os.Unsetenv("PHONETIC_ADDR")

	cfgEnv, err := config.Load("")
	if err != nil {
		t.Fatalf("Load with env failed: %v", err)
	}
	if cfgEnv.Server.Addr != ":9999" {
		t.Errorf("expected env addr :9999, got %s", cfgEnv.Server.Addr)
	}
}

func TestLoadConfigFile(t *testing.T) {
	tmpDir := t.TempDir()
	configFilePath := filepath.Join(tmpDir, "custom.json")
	content := []byte(`{
		"server": {
			"addr": ":8888",
			"allowed_origins": ["http://localhost:8080"]
		},
		"data": {
			"word_index_path": "test_word.dat"
		}
	}`)
	if err := os.WriteFile(configFilePath, content, 0644); err != nil {
		t.Fatalf("write temp config: %v", err)
	}

	cfg, err := config.Load(configFilePath)
	if err != nil {
		t.Fatalf("Load custom config failed: %v", err)
	}
	if cfg.Server.Addr != ":8888" {
		t.Errorf("expected addr :8888, got %s", cfg.Server.Addr)
	}
	if len(cfg.Server.AllowedOrigins) != 1 || cfg.Server.AllowedOrigins[0] != "http://localhost:8080" {
		t.Errorf("unexpected allowed_origins: %v", cfg.Server.AllowedOrigins)
	}
	if cfg.Data.WordIndexPath != "test_word.dat" {
		t.Errorf("expected word_index_path test_word.dat, got %s", cfg.Data.WordIndexPath)
	}
}
