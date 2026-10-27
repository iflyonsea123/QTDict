package main

import (
	"errors"
	"flag"
	"log"
	"net/http"
	"os"
	"path/filepath"
	"strings"
	"time"

	"phoneticengine/httpapi"
	"phoneticengine/phonetic"
)

func main() {
	defaultDataPath := os.Getenv("PHONETIC_DATA")
	if defaultDataPath == "" {
		defaultDataPath = filepath.Join("..", "bin", "go", "IPhonetic.dat")
	}
	defaultAddr := os.Getenv("PHONETIC_ADDR")
	if defaultAddr == "" {
		defaultAddr = ":8080"
	}

	dataPath := flag.String("data", defaultDataPath, "path to the uncompressed phonetic index")
	addr := flag.String("addr", defaultAddr, "HTTP listen address")
	flag.Parse()

	index, err := phonetic.LoadFile(*dataPath)
	if err != nil {
		log.Fatalf("load phonetic index: %v", err)
	}
	log.Printf("loaded %d phonetic entries from %s", index.Count(), *dataPath)

	allowedOrigins := strings.Split(os.Getenv("PHONETIC_ALLOWED_ORIGINS"), ",")
	server := &http.Server{
		Addr:              *addr,
		Handler:           httpapi.NewHandler(index, allowedOrigins),
		ReadHeaderTimeout: 5 * time.Second,
	}
	log.Printf("phonetic API listening on %s", *addr)
	if err := server.ListenAndServe(); err != nil && !errors.Is(err, http.ErrServerClosed) {
		log.Fatalf("start phonetic API: %v", err)
	}
}
