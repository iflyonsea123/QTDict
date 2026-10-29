package main

import (
	"errors"
	"flag"
	"log"
	"net/http"
	"os"
	"time"

	"phoneticengine/config"
	"phoneticengine/httpapi"
	"phoneticengine/phonetic"
	"phoneticengine/word"
)

func main() {
	configPath := flag.String("config", "", "path to config file (e.g. config.json)")
	addrFlag := flag.String("addr", "", "HTTP listen address override (e.g. :8080)")
	dataFlag := flag.String("data", "", "path to phonetic index override")
	flag.Parse()

	cfg, err := config.Load(*configPath)
	if err != nil {
		log.Fatalf("failed to load configuration: %v", err)
	}

	if *addrFlag != "" {
		cfg.Server.Addr = *addrFlag
	}
	if *dataFlag != "" {
		cfg.Data.PhoneticPath = *dataFlag
	}

	// 1. Load phonetic index
	var phoneticIndex *phonetic.Index
	if cfg.Data.PhoneticPath != "" {
		if _, err := os.Stat(cfg.Data.PhoneticPath); err == nil {
			phoneticIndex, err = phonetic.LoadFile(cfg.Data.PhoneticPath)
			if err != nil {
				log.Fatalf("load phonetic index: %v", err)
			}
			log.Printf("loaded %d phonetic entries from %s", phoneticIndex.Count(), cfg.Data.PhoneticPath)
		} else {
			log.Printf("phonetic index file not found at %s (skipped)", cfg.Data.PhoneticPath)
		}
	}

	// 2. Load word definitions and index
	var wordIndex *word.Index
	if cfg.Data.WordIndexPath != "" {
		if _, err := os.Stat(cfg.Data.WordIndexPath); err == nil {
			var defs *word.DefinitionStore
			if cfg.Data.SimpleMeaningPath != "" && cfg.Data.DetailMeaningPath != "" {
				defs, err = word.LoadDefinitions(cfg.Data.SimpleMeaningPath, cfg.Data.DetailMeaningPath)
				if err != nil {
					log.Printf("warning: load word definitions failed: %v", err)
				} else {
					log.Printf("loaded word definitions from %s and %s", cfg.Data.SimpleMeaningPath, cfg.Data.DetailMeaningPath)
				}
			}
			wordIndex, err = word.LoadFile(cfg.Data.WordIndexPath, defs)
			if err != nil {
				log.Fatalf("load word index: %v", err)
			}
			log.Printf("loaded %d word entries from %s", wordIndex.Count(), cfg.Data.WordIndexPath)
		} else {
			log.Printf("word index file not found at %s (skipped)", cfg.Data.WordIndexPath)
		}
	}

	if phoneticIndex == nil && wordIndex == nil {
		log.Fatal("neither phonetic nor word index could be loaded; check data paths in config.json")
	}

	server := &http.Server{
		Addr:              cfg.Server.Addr,
		Handler:           httpapi.NewHandler(phoneticIndex, wordIndex, cfg.Server.AllowedOrigins),
		ReadHeaderTimeout: 5 * time.Second,
	}

	log.Printf("dict server listening on %s", cfg.Server.Addr)
	if err := server.ListenAndServe(); err != nil && !errors.Is(err, http.ErrServerClosed) {
		log.Fatalf("start server: %v", err)
	}
}
