package config

import (
	"encoding/json"
	"fmt"
	"os"
	"path/filepath"
	"strings"
)

type ServerConfig struct {
	Addr           string   `json:"addr"`
	AllowedOrigins []string `json:"allowed_origins"`
}

type DataConfig struct {
	PhoneticPath      string `json:"phonetic_path"`
	WordIndexPath     string `json:"word_index_path"`
	SimpleMeaningPath string `json:"simple_meaning_path"`
	DetailMeaningPath string `json:"detail_meaning_path"`
}

type Config struct {
	Server ServerConfig `json:"server"`
	Data   DataConfig   `json:"data"`
}

func DefaultConfig() *Config {
	return &Config{
		Server: ServerConfig{
			Addr:           ":8080",
			AllowedOrigins: []string{},
		},
		Data: DataConfig{
			PhoneticPath:      filepath.Join("..", "bin", "go", "IPhonetic.dat"),
			WordIndexPath:     filepath.Join("..", "bin", "go", "iword.dat"),
			SimpleMeaningPath: filepath.Join("..", "bin", "go", "data1.dat"),
			DetailMeaningPath: filepath.Join("..", "bin", "go", "data2.dat"),
		},
	}
}

func Load(configPath string) (*Config, error) {
	cfg := DefaultConfig()

	// If a config file path is explicitly given or default file exists, load it.
	targetPath := configPath
	if targetPath == "" {
		if _, err := os.Stat("config.json"); err == nil {
			targetPath = "config.json"
		}
	}

	if targetPath != "" {
		content, err := os.ReadFile(targetPath)
		if err != nil {
			return nil, fmt.Errorf("read config file %s: %w", targetPath, err)
		}
		if err := json.Unmarshal(content, cfg); err != nil {
			return nil, fmt.Errorf("parse config file %s: %w", targetPath, err)
		}
	}

	// Environment variable overrides for compatibility
	if envAddr := os.Getenv("PHONETIC_ADDR"); envAddr != "" {
		cfg.Server.Addr = envAddr
	}
	if envOrigins := os.Getenv("PHONETIC_ALLOWED_ORIGINS"); envOrigins != "" {
		for _, o := range strings.Split(envOrigins, ",") {
			trimmed := strings.TrimSpace(o)
			if trimmed != "" {
				cfg.Server.AllowedOrigins = append(cfg.Server.AllowedOrigins, trimmed)
			}
		}
	}
	if envPhonetic := os.Getenv("PHONETIC_DATA"); envPhonetic != "" {
		cfg.Data.PhoneticPath = envPhonetic
	}
	if envWord := os.Getenv("WORD_INDEX_DATA"); envWord != "" {
		cfg.Data.WordIndexPath = envWord
	}
	if envSimple := os.Getenv("SIMPLE_MEANING_DATA"); envSimple != "" {
		cfg.Data.SimpleMeaningPath = envSimple
	}
	if envDetail := os.Getenv("DETAIL_MEANING_DATA"); envDetail != "" {
		cfg.Data.DetailMeaningPath = envDetail
	}

	return cfg, nil
}
